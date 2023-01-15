// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../util/io.dart';
import '../util/package_config.dart';
import 'compiler_pool.dart';
import 'suite.dart';

/// A pool of `dart2wasm` compiler instances.
///
/// This limits the number of compiler instances running concurrently.
class WasmCompilerPool extends CompilerPool {
  /// The currently-active dart2wasm processes.
  final _processes = <Process>{};

  /// Compiles [code] to [path].
  ///
  /// This wraps the Dart code in the standard browser-testing wrapper.
  ///
  /// The returned [Future] will complete once the `dart2wasm` process completes
  /// *and* all its output has been printed to the command line.
  @override
  Future compileInternal(
      String code, String path, SuiteConfiguration suiteConfig) {
    return withTempDir((dir) async {
      var wrapperPath = p.join(dir, 'main.dart');
      File(wrapperPath).writeAsStringSync(code);
      var outWasmPath = '$path.wasm';
      var outJSPath = p.join(p.dirname(path), 'dart2wasm_runtime.mjs');
      var dartBinPath = Platform.resolvedExecutable;
      var sdkRoot = p.join(p.dirname(dartBinPath), '../');
      var jsRuntimePath = p.join(sdkRoot, 'bin', 'dart2wasm_runtime.mjs');
      File(jsRuntimePath).copy(outJSPath);
      var platformDill =
          p.join(sdkRoot, 'lib', '_internal', 'dart2wasm_platform.dill');
      var dartPrecompiledRuntimePath = p.join(sdkRoot, 'bin', 'dartaotruntime');
      var dart2wasmSnapshotPath =
          p.join(sdkRoot, 'bin/snapshots', 'dart2wasm_product.snapshot');
      var process = await Process.start(dartPrecompiledRuntimePath, [
        dart2wasmSnapshotPath,
        '--dart-sdk=$sdkRoot',
        '--platform=$platformDill',
        '--packages=${(await packageConfigUri).path}',
        wrapperPath,
        outWasmPath,
      ]);
      if (closed) {
        process.kill();
        return;
      }

      _processes.add(process);

      /// Wait until the process is entirely done to print out any output.
      /// This can produce a little extra time for users to wait with no
      /// update, but it also avoids some really nasty-looking interleaved
      /// output. Write both stdout and stderr to the same buffer in case
      /// they're intended to be printed in order.
      var buffer = StringBuffer();

      await Future.wait([
        process.stdout.transform(utf8.decoder).forEach(buffer.write),
        process.stderr.transform(utf8.decoder).forEach(buffer.write),
      ]);

      var exitCode = await process.exitCode;
      _processes.remove(process);
      if (closed) return;

      var output = buffer.toString();
      if (output.isNotEmpty) print(output);

      if (exitCode != 0) throw StateError('dart2wasm failed.');
    });
  }

  /// Closes the compiler pool.
  ///
  /// This kills all currently-running compilers and ensures that no more will
  /// be started. It returns a [Future] that completes once all the compilers
  /// have been killed and all resources released.
  @override
  Future<void> closeInternal() async {
    await Future.wait(_processes.map((process) async {
      process.kill();
      await process.exitCode;
    }));
  }
}
