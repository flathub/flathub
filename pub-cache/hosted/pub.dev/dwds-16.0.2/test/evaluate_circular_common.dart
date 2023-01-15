// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@TestOn('vm')
import 'dart:async';

import 'package:dwds/src/connections/debug_connection.dart';
import 'package:dwds/src/services/chrome_proxy_service.dart';
import 'package:path/path.dart' as p;
import 'package:test/test.dart';
import 'package:vm_service/vm_service.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import 'fixtures/context.dart';
import 'fixtures/logging.dart';

class TestSetup {
  static TestContext createContext(String index, String packageRoot) =>
      TestContext(
          directory: p.join('..', 'fixtures', packageRoot),
          entry: p.join('..', 'fixtures', packageRoot, 'web', 'main.dart'),
          path: index,
          pathToServe: 'web');

  static TestContext contextUnsound(String index) =>
      createContext(index, '_testCircular2');

  static TestContext contextSound(String index) =>
      createContext(index, '_testCircular2Sound');

  TestContext context;

  TestSetup.sound(IndexBaseMode baseMode)
      : context = contextSound(_index(baseMode));

  TestSetup.unsound(IndexBaseMode baseMode)
      : context = contextUnsound(_index(baseMode));

  factory TestSetup.create(NullSafety? nullSafety, IndexBaseMode baseMode) =>
      nullSafety == NullSafety.sound
          ? TestSetup.sound(baseMode)
          : TestSetup.unsound(baseMode);

  ChromeProxyService get service =>
      fetchChromeProxyService(context.debugConnection);
  WipConnection get tabConnection => context.tabConnection;

  static String _index(IndexBaseMode baseMode) =>
      baseMode == IndexBaseMode.base ? 'base_index.html' : 'index.html';
}

void testAll({
  CompilationMode compilationMode = CompilationMode.buildDaemon,
  IndexBaseMode indexBaseMode = IndexBaseMode.noBase,
  NullSafety nullSafety = NullSafety.sound,
  bool useDebuggerModuleNames = false,
  bool debug = false,
}) {
  if (compilationMode == CompilationMode.buildDaemon &&
      indexBaseMode == IndexBaseMode.base) {
    throw StateError(
        'build daemon scenario does not support non-empty base in index file');
  }
  final setup = TestSetup.create(nullSafety, indexBaseMode);
  final context = setup.context;

  Future<void> onBreakPoint(String isolate, ScriptRef script,
      String breakPointId, Future<void> Function() body) async {
    Breakpoint? bp;
    try {
      final line =
          await context.findBreakpointLine(breakPointId, isolate, script);
      bp = await setup.service
          .addBreakpointWithScriptUri(isolate, script.uri!, line);
      await body();
    } finally {
      // Remove breakpoint so it doesn't impact other tests or retries.
      if (bp != null) {
        await setup.service.removeBreakpoint(isolate, bp.id!);
      }
    }
  }

  group('shared context with evaluation', () {
    setUpAll(() async {
      setCurrentLogWriter(debug: debug);
      await context.setUp(
        compilationMode: compilationMode,
        nullSafety: nullSafety,
        enableExpressionEvaluation: true,
        useDebuggerModuleNames: useDebuggerModuleNames,
        verboseCompiler: debug,
      );
    });

    tearDownAll(() async {
      await context.tearDown();
    });

    setUp(() => setCurrentLogWriter(debug: debug));

    group('evaluateInFrame', () {
      VM vm;
      late Isolate isolate;
      late String isolateId;
      ScriptList scripts;
      late ScriptRef test1LibraryScript;
      late ScriptRef test2LibraryScript;
      late Stream<Event> stream;

      setUp(() async {
        setCurrentLogWriter(debug: debug);
        vm = await setup.service.getVM();
        isolate = await setup.service.getIsolate(vm.isolates!.first.id!);
        isolateId = isolate.id!;
        scripts = await setup.service.getScripts(isolateId);

        await setup.service.streamListen('Debug');
        stream = setup.service.onEvent('Debug');

        final soundNullSafety = nullSafety == NullSafety.sound;
        final test1 =
            soundNullSafety ? '_test_circular1_sound' : '_test_circular1';
        final test2 =
            soundNullSafety ? '_test_circular2_sound' : '_test_circular2';

        test1LibraryScript = scripts.scripts!.firstWhere(
            (each) => each.uri!.contains('package:$test1/library1.dart'));
        test2LibraryScript = scripts.scripts!.firstWhere(
            (each) => each.uri!.contains('package:$test2/library2.dart'));
      });

      tearDown(() async {
        await setup.service.resume(isolateId);
      });

      test('evaluate expression in _test_circular1/library', () async {
        await onBreakPoint(isolateId, test1LibraryScript, 'Concatenate',
            () async {
          final event = await stream
              .firstWhere((event) => event.kind == EventKind.kPauseBreakpoint);

          final result = await setup.service
              .evaluateInFrame(isolateId, event.topFrame!.index!, 'a');

          expect(
              result,
              isA<InstanceRef>().having(
                  (instance) => instance.valueAsString, 'valueAsString', 'a'));
        });
      });

      test('evaluate expression in _test_circular2/library', () async {
        await onBreakPoint(
            isolateId, test2LibraryScript, 'testCircularDependencies',
            () async {
          final event = await stream
              .firstWhere((event) => event.kind == EventKind.kPauseBreakpoint);

          final result = await setup.service
              .evaluateInFrame(isolateId, event.topFrame!.index!, 'true');

          expect(
              result,
              isA<InstanceRef>().having((instance) => instance.valueAsString,
                  'valueAsString', 'true'));
        });
      });
    });
  });
}
