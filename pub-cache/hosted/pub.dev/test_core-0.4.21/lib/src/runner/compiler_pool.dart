// Copyright (c) 2015, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';

import 'package:async/async.dart';
import 'package:meta/meta.dart';
import 'package:pool/pool.dart';

import 'configuration.dart';
import 'suite.dart';

/// A pool of compiler instances.
///
/// This limits the number of compiler instances running concurrently.
abstract class CompilerPool {
  /// The test runner configuration.
  final config = Configuration.current;

  /// The internal pool that controls the number of process running at once.
  final Pool _pool;

  /// Whether [close] has been called.
  bool get closed => _closeMemo.hasRun;

  /// The memoizer for running [close] exactly once.
  final _closeMemo = AsyncMemoizer();

  /// Creates a compiler pool that multiple instances of a compiler at once.
  CompilerPool() : _pool = Pool(Configuration.current.concurrency);

  /// Compiles [code] to [path] using [_pool] and [compileInternal].
  ///
  /// Should not be overridden.
  Future<void> compile(
          String code, String path, SuiteConfiguration suiteConfig) =>
      _pool.withResource(() {
        if (closed) return null;
        return compileInternal(code, path, suiteConfig);
      });

  /// The actual function a given compiler pool should implement to compile a
  /// suite.
  @protected
  Future<void> compileInternal(
      String code, String path, SuiteConfiguration suiteConfig);

  /// Shuts down the compiler pool, invoking `closeInternal` exactly once.
  ///
  /// Should not be overridden.
  Future<void> close() => _closeMemo.runOnce(closeInternal);

  /// The actual function to shut down the compiler pool, invoked exactly once.
  @protected
  Future<void> closeInternal();
}
