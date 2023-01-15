// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@TestOn('vm')

import 'package:dwds/dwds.dart';
import 'package:file/local.dart';
import 'package:path/path.dart' as p;
import 'package:test/test.dart';

void main() {
  for (final useDebuggerModuleNames in [true, false]) {
    group(
        'Package uri mapper with debugger module names: '
        ' $useDebuggerModuleNames |', () {
      final fileSystem = LocalFileSystem();

      final packageUri =
          Uri(scheme: 'package', path: '_test_package/test_library.dart');

      final serverPath = useDebuggerModuleNames
          ? 'packages/_testPackage/lib/test_library.dart'
          : '/packages/_test_package/test_library.dart';

      final resolvedPath =
          '/webdev/fixtures/_testPackage/lib/test_library.dart';

      final packageConfigFile = Uri.file(p.normalize(p.absolute(p.join(
        '..',
        'fixtures',
        '_testPackage',
        '.dart_tool',
        'package_config.json',
      ))));

      late final PackageUriMapper packageUriMapper;
      setUpAll(() async {
        packageUriMapper = await PackageUriMapper.create(
          fileSystem,
          packageConfigFile,
          useDebuggerModuleNames: useDebuggerModuleNames,
        );
      });

      test('Can convert package urls to server paths', () {
        expect(packageUriMapper.packageUriToServerPath(packageUri), serverPath);
      });

      test('Can convert server paths to file paths', () {
        expect(
            packageUriMapper.serverPathToResolvedUri(serverPath),
            isA<Uri>()
                .having((uri) => uri.scheme, 'scheme', 'file')
                .having((uri) => uri.path, 'path', endsWith(resolvedPath)));
      });
    });
  }
}
