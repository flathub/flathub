// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
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

final context = TestContext(
    directory: p.join('..', 'fixtures', '_testPackage'),
    entry: p.join('..', 'fixtures', '_testPackage', 'web', 'main.dart'),
    path: 'index.html',
    pathToServe: 'web');

ChromeProxyService get service =>
    fetchChromeProxyService(context.debugConnection);
WipConnection get tabConnection => context.tabConnection;

void main() {
  group('shared context', () {
    setUpAll(() async {
      await context.setUp();
    });

    tearDownAll(() async {
      await context.tearDown();
    });

    group('breakpoint', () {
      VM vm;
      late Isolate isolate;
      late String isolateId;
      ScriptList scripts;
      late ScriptRef mainScript;
      late String mainScriptUri;
      late Stream<Event> stream;

      setUp(() async {
        vm = await service.getVM();
        isolate = await service.getIsolate(vm.isolates!.first.id!);
        isolateId = isolate.id!;
        scripts = await service.getScripts(isolateId);

        await service.streamListen('Debug');
        stream = service.onEvent('Debug');

        mainScript = scripts.scripts!
            .firstWhere((each) => each.uri!.contains('main.dart'));
        mainScriptUri = mainScript.uri!;
      });

      tearDown(() async {
        await service.resume(isolateId);
      });

      test('set breakpoint', () async {
        final line = await context.findBreakpointLine(
            'printLocal', isolateId, mainScript);
        final bp = await service.addBreakpointWithScriptUri(
            isolateId, mainScriptUri, line);

        await stream.firstWhere(
            (Event event) => event.kind == EventKind.kPauseBreakpoint);

        expect(bp, isNotNull);

        // Remove breakpoint so it doesn't impact other tests.
        await service.removeBreakpoint(isolateId, bp.id!);
      });

      test('set breakpoint again', () async {
        final line = await context.findBreakpointLine(
            'printLocal', isolateId, mainScript);
        final bp = await service.addBreakpointWithScriptUri(
            isolateId, mainScriptUri, line);

        await stream.firstWhere(
            (Event event) => event.kind == EventKind.kPauseBreakpoint);

        expect(bp, isNotNull);

        // Remove breakpoint so it doesn't impact other tests.
        await service.removeBreakpoint(isolateId, bp.id!);
      });

      test('set existing breakpoint succeeds', () async {
        final line = await context.findBreakpointLine(
            'printLocal', isolateId, mainScript);
        final bp1 = await service.addBreakpointWithScriptUri(
            isolateId, mainScriptUri, line);
        final bp2 = await service.addBreakpointWithScriptUri(
            isolateId, mainScriptUri, line);

        expect(bp1, equals(bp2));
        expect(bp1, isNotNull);

        await stream.firstWhere(
            (Event event) => event.kind == EventKind.kPauseBreakpoint);

        var currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, containsAll([bp1]));

        // Remove breakpoints so they don't impact other tests.
        await service.removeBreakpoint(isolateId, bp1.id!);

        currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, isEmpty);
      });

      test('set breakpoints at the same line simultaneously succeeds',
          () async {
        final line = await context.findBreakpointLine(
            'printLocal', isolateId, mainScript);
        final futures = [
          service.addBreakpointWithScriptUri(isolateId, mainScriptUri, line),
          service.addBreakpointWithScriptUri(isolateId, mainScriptUri, line),
        ];

        final breakpoints = await Future.wait(futures);
        expect(breakpoints[0], equals(breakpoints[1]));
        expect(breakpoints[0], isNotNull);

        await stream.firstWhere(
            (Event event) => event.kind == EventKind.kPauseBreakpoint);

        var currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, containsAll([breakpoints[0]]));

        // Remove breakpoints so they don't impact other tests.
        await service.removeBreakpoint(isolateId, breakpoints[0].id!);

        currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, isEmpty);
      });

      test('remove non-existing breakpoint fails', () async {
        final line = await context.findBreakpointLine(
            'printLocal', isolateId, mainScript);
        final bp = await service.addBreakpointWithScriptUri(
            isolateId, mainScriptUri, line);

        await stream.firstWhere(
            (Event event) => event.kind == EventKind.kPauseBreakpoint);

        var currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, containsAll([bp]));

        // Remove breakpoints so they don't impact other tests.
        await service.removeBreakpoint(isolateId, bp.id!);
        await expectLater(
            service.removeBreakpoint(isolateId, bp.id!), throwsRPCError);

        currentIsolate = await service.getIsolate(isolateId);
        expect(currentIsolate.breakpoints, isEmpty);
      });
    });
  });
}
