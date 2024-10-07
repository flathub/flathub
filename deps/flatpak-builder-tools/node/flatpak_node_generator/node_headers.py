from typing import NamedTuple, Optional


class NodeHeaders(NamedTuple):
    target: str
    runtime: str
    disturl: str

    @classmethod
    def with_defaults(
        cls,
        target: str,
        runtime: Optional[str] = None,
        disturl: Optional[str] = None,
    ) -> 'NodeHeaders':
        if runtime is None:
            runtime = 'node'
        if disturl is None:
            if runtime == 'node':
                disturl = 'http://nodejs.org/dist'
            elif runtime == 'electron':
                disturl = 'https://www.electronjs.org/headers'
            else:
                raise ValueError(
                    f"Can't guess `disturl` for {runtime} version {target}"
                )
        return cls(target, runtime, disturl)

    @property
    def url(self) -> str:
        # TODO it may be better to retrieve urls from disturl/index.json
        return f'{self.disturl}/v{self.target}/node-v{self.target}-headers.tar.gz'

    @property
    def install_version(self) -> str:
        # FIXME not sure if this static value will always work
        return '9'
