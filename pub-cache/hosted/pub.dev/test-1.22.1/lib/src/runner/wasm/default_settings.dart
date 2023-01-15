// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:collection';

import 'package:test_api/src/backend/runtime.dart'; // ignore: implementation_imports
import '../executable_settings.dart';

/// Default settings for starting browser executables with the wasm runtime.
final defaultSettings = UnmodifiableMapView({
  Runtime.experimentalChromeWasm: ExecutableSettings(
      linuxExecutable: 'google-chrome-beta',
      macOSExecutable: null,
      windowsExecutable: null,
      arguments: [
        '--js-flags=--experimental-wasm-gc --wasm-gc-js-interop '
            '--experimental-wasm-stack-switching '
            '--experimental-wasm-type-reflection'
      ]),
});
