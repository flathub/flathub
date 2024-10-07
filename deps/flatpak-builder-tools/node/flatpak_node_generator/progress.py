from typing import Collection, ContextManager, Optional, Type

import asyncio
import shutil
import sys
import types

from .package import Package
from .providers import ModuleProvider


class GeneratorProgress(ContextManager['GeneratorProgress']):
    def __init__(
        self,
        packages: Collection[Package],
        module_provider: ModuleProvider,
        max_parallel: int,
    ) -> None:
        self.finished = 0
        self.packages = packages
        self.module_provider = module_provider
        self.parallel_limit = asyncio.Semaphore(max_parallel)
        self.previous_package: Optional[Package] = None
        self.current_package: Optional[Package] = None

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        tb: Optional[types.TracebackType],
    ) -> None:
        print()

    def _format_package(self, package: Package, max_width: int) -> str:
        result = f'{package.name} @ {package.version}'

        if len(result) > max_width:
            result = result[: max_width - 3] + '...'

        return result

    def _update(self) -> None:
        columns, _ = shutil.get_terminal_size()

        sys.stdout.write('\r' + ' ' * columns)

        prefix_string = f'\rGenerating packages [{self.finished}/{len(self.packages)}] '
        sys.stdout.write(prefix_string)
        max_package_width = columns - len(prefix_string)

        if self.current_package is not None:
            sys.stdout.write(
                self._format_package(self.current_package, max_package_width)
            )

        sys.stdout.flush()

    def _update_with_package(self, package: Package) -> None:
        self.previous_package, self.current_package = (
            self.current_package,
            package,
        )
        self._update()

    async def _generate(self, package: Package) -> None:
        async with self.parallel_limit:
            self._update_with_package(package)
            await self.module_provider.generate_package(package)
            self.finished += 1
            self._update_with_package(package)

    async def run(self) -> None:
        self._update()

        tasks = [asyncio.create_task(self._generate(pkg)) for pkg in self.packages]
        for coro in asyncio.as_completed(tasks):
            try:
                await coro
            except:
                # If an exception occurred, make sure to cancel all the other
                # tasks.
                for task in tasks:
                    task.cancel()

                raise
