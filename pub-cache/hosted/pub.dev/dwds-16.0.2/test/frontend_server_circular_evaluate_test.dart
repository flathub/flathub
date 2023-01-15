// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@TestOn('vm')

import 'dart:io';

import 'package:pub_semver/pub_semver.dart';
import 'package:test/test.dart';

import 'fixtures/context.dart';
import 'evaluate_circular_common.dart';

void main() async {
  // Enable verbose logging for debugging.
  final debug = false;

  // TODO(annagrin): Remove when 2.19.0-150.0.dev is in stable.
  final debuggerModuleNamesSupported =
      Version.parse(Platform.version.split(' ').first) >=
          Version.parse('2.19.0-150.0.dev');

  group('Context with circular dependencies |', () {
    for (var nullSafety in NullSafety.values) {
      group('${nullSafety.name} null safety |', () {
        for (var indexBaseMode in IndexBaseMode.values) {
          group(
            'with ${indexBaseMode.name} |',
            () {
              testAll(
                compilationMode: CompilationMode.frontendServer,
                indexBaseMode: indexBaseMode,
                nullSafety: nullSafety,
                useDebuggerModuleNames: true,
                debug: debug,
              );
            },
            skip:
                // https://github.com/dart-lang/sdk/issues/49277
                indexBaseMode == IndexBaseMode.base && Platform.isWindows ||
                    // https://github.com/dart-lang/webdev/issues/1591);
                    nullSafety == NullSafety.sound ||
                    // Needs debugger module names change in the SDK to work.
                    !debuggerModuleNamesSupported,
          );
        }
      });
    }
  });
}
