// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';

import 'package:dwds/asset_reader.dart';
import 'package:dwds/expression_compiler.dart';
import 'package:dwds/src/debugging/classes.dart';
import 'package:dwds/src/debugging/execution_context.dart';
import 'package:dwds/src/debugging/inspector.dart';
import 'package:dwds/src/debugging/instance.dart';
import 'package:dwds/src/debugging/libraries.dart';
import 'package:dwds/src/debugging/metadata/provider.dart';
import 'package:dwds/src/debugging/modules.dart';
import 'package:dwds/src/debugging/remote_debugger.dart';
import 'package:dwds/src/debugging/webkit_debugger.dart';
import 'package:dwds/src/handlers/socket_connections.dart';
import 'package:dwds/src/loaders/require.dart';
import 'package:dwds/src/loaders/strategy.dart';
import 'package:shelf/shelf.dart' as shelf;
import 'package:vm_service/vm_service.dart';

/// A library of fake/stub implementations of our classes and their supporting
/// classes (e.g. WipConnection) for unit testing.
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import 'debugger_data.dart';

/// Constructs a trivial Isolate we can use when we need to provide one but
/// don't want go through initialization.
Isolate get simpleIsolate => Isolate(
      id: '1',
      number: '1',
      name: 'fake',
      libraries: [],
      exceptionPauseMode: 'abc',
      breakpoints: [],
      pauseOnExit: false,
      pauseEvent: null,
      startTime: 0,
      livePorts: 0,
      runnable: false,
      isSystemIsolate: false,
      isolateFlags: [],
    );

class FakeInspector implements AppInspector {
  FakeInspector({required this.fakeIsolate});

  Isolate fakeIsolate;

  @override
  Object noSuchMethod(Invocation invocation) {
    throw UnsupportedError('This is a fake');
  }

  @override
  Future<void> initialize(LibraryHelper libraryHelper, ClassHelper classHelper,
          InstanceHelper instanceHelper) async =>
      {};

  @override
  Future<InstanceRef?> instanceRefFor(Object value) async =>
      InstanceHelper.kNullInstanceRef;

  @override
  Future<Obj> getObject(String objectId, {int? offset, int? count}) async =>
      Obj.parse({})!;

  @override
  Future<ScriptList> getScripts() async => ScriptList(scripts: []);

  @override
  Future<ScriptRef> scriptRefFor(String uri) async =>
      ScriptRef(id: 'fake', uri: 'fake://uri');

  @override
  ScriptRef? scriptWithId(String? scriptId) => null;

  @override
  Isolate get isolate => fakeIsolate;

  @override
  IsolateRef get isolateRef => IsolateRef(
        id: fakeIsolate.id,
        number: fakeIsolate.number,
        name: fakeIsolate.name,
        isSystemIsolate: fakeIsolate.isSystemIsolate,
      );
}

class FakeSseConnection implements SseSocketConnection {
  /// A [StreamController] for incoming messages on SSE connection.
  final controllerIncoming = StreamController<String>();

  /// A [StreamController] for outgoing messages on SSE connection.
  final controllerOutgoing = StreamController<String>();

  @override
  bool get isInKeepAlivePeriod => false;

  @override
  StreamSink<String> get sink => controllerOutgoing.sink;

  @override
  Stream<String> get stream => controllerIncoming.stream;

  @override
  void shutdown() {}
}

class FakeModules implements Modules {
  @override
  void initialize(String entrypoint) {}

  @override
  Future<Uri> libraryForSource(String serverPath) {
    throw UnimplementedError();
  }

  @override
  Future<String> moduleForSource(String serverPath) {
    throw UnimplementedError();
  }

  @override
  Future<Map<String, String>> modules() {
    throw UnimplementedError();
  }

  @override
  Future<String> moduleForLibrary(String libraryUri) {
    throw UnimplementedError();
  }
}

class FakeWebkitDebugger implements WebkitDebugger {
  final Map<String, WipScript>? _scripts;
  @override
  Future disable() async => null;

  @override
  Future enable() async => null;

  FakeWebkitDebugger({Map<String, WipScript>? scripts}) : _scripts = scripts {
    globalLoadStrategy = RequireStrategy(
      ReloadConfiguration.none,
      (_) async => {},
      (_) async => {},
      (_, __) async => null,
      (MetadataProvider _, String __) async => '',
      (MetadataProvider _, String __) async => '',
      (String _) => '',
      (MetadataProvider _) async => <String, ModuleInfo>{},
      FakeAssetReader(),
    );
  }

  @override
  Stream<T> eventStream<T>(String method, WipEventTransformer<T> transformer) =>
      Stream.empty();

  @override
  Future<String> getScriptSource(String scriptId) async => '';

  Stream<WipDomain>? get onClosed => null;

  @override
  Stream<GlobalObjectClearedEvent> get onGlobalObjectCleared => Stream.empty();

  @override
  late Stream<DebuggerPausedEvent> onPaused;

  @override
  Stream<DebuggerResumedEvent> get onResumed => Stream.empty();

  @override
  Stream<ScriptParsedEvent> get onScriptParsed => Stream.empty();

  @override
  Stream<TargetCrashedEvent> get onTargetCrashed => Stream.empty();

  @override
  Future<WipResponse> pause() async => fakeWipResponse;

  @override
  Future<WipResponse> resume() async => fakeWipResponse;

  @override
  Map<String, WipScript> get scripts => _scripts!;

  List<WipResponse> results = variables1;
  int resultsReturned = 0;

  @override
  Future<WipResponse> sendCommand(String command,
      {Map<String, dynamic>? params}) async {
    // Force the results that we expect for looking up the variables.
    if (command == 'Runtime.getProperties') {
      return results[resultsReturned++];
    }
    return fakeWipResponse;
  }

  @override
  Future<WipResponse> setPauseOnExceptions(PauseState state) async =>
      fakeWipResponse;

  @override
  Future<WipResponse> removeBreakpoint(String breakpointId) async =>
      fakeWipResponse;

  @override
  Future<WipResponse> stepInto({Map<String, dynamic>? params}) async =>
      fakeWipResponse;

  @override
  Future<WipResponse> stepOut() async => fakeWipResponse;

  @override
  Future<WipResponse> stepOver({Map<String, dynamic>? params}) async =>
      fakeWipResponse;

  @override
  Stream<ConsoleAPIEvent> get onConsoleAPICalled => Stream.empty();

  @override
  Stream<ExceptionThrownEvent> get onExceptionThrown => Stream.empty();

  @override
  void close() {}

  @override
  Stream<WipConnection> get onClose => Stream.empty();

  @override
  Future<RemoteObject> evaluate(String expression,
          {bool? returnByValue, int? contextId}) async =>
      RemoteObject({});

  @override
  Future<RemoteObject> evaluateOnCallFrame(
      String callFrameId, String expression) async {
    return RemoteObject(<String, dynamic>{});
  }

  @override
  Future<List<WipBreakLocation>> getPossibleBreakpoints(
          WipLocation start) async =>
      [];

  @override
  Future<WipResponse> enablePage() async => fakeWipResponse;

  @override
  Future<WipResponse> pageReload() async => fakeWipResponse;
}

/// Fake execution context that is needed for id only
class FakeExecutionContext extends ExecutionContext {
  @override
  Future<int> get id async {
    return 0;
  }

  FakeExecutionContext();
}

class FakeStrategy implements LoadStrategy {
  @override
  Future<String> bootstrapFor(String entrypoint) async => 'dummy_bootstrap';

  @override
  shelf.Handler get handler =>
      (request) => (request.url.path == 'someDummyPath')
          ? shelf.Response.ok('some dummy response')
          : shelf.Response.notFound('someDummyPath');

  @override
  String get id => 'dummy-id';

  @override
  String get moduleFormat => 'dummy-format';

  @override
  String get loadLibrariesModule => '';

  @override
  String get loadLibrariesSnippet => '';

  @override
  String loadLibrarySnippet(String libraryUri) => '';

  @override
  String get loadModuleSnippet => '';

  @override
  ReloadConfiguration get reloadConfiguration => ReloadConfiguration.none;

  @override
  String loadClientSnippet(String clientScript) => 'dummy-load-client-snippet';

  @override
  Future<String?> moduleForServerPath(
          String entrypoint, String serverPath) async =>
      '';

  @override
  Future<String> serverPathForModule(String entrypoint, String module) async =>
      '';

  @override
  Future<String> sourceMapPathForModule(
          String entrypoint, String module) async =>
      '';

  @override
  String? serverPathForAppUri(String appUri) => '';

  @override
  MetadataProvider metadataProviderFor(String entrypoint) =>
      MetadataProvider(entrypoint, FakeAssetReader());

  @override
  void trackEntrypoint(String entrypoint) {}

  @override
  Future<Map<String, ModuleInfo>> moduleInfoForEntrypoint(String entrypoint) =>
      throw UnimplementedError();
}

class FakeAssetReader implements AssetReader {
  final String? _metadata;
  final String? _dartSource;
  final String? _sourceMap;
  FakeAssetReader({
    metadata,
    dartSource,
    sourceMap,
  })  : _metadata = metadata,
        _dartSource = dartSource,
        _sourceMap = sourceMap;

  @override
  Future<String> dartSourceContents(String serverPath) {
    return _throwUnimplementedOrReturnContents(_dartSource);
  }

  @override
  Future<String> metadataContents(String serverPath) {
    return _throwUnimplementedOrReturnContents(_metadata);
  }

  @override
  Future<String> sourceMapContents(String serverPath) {
    return _throwUnimplementedOrReturnContents(_sourceMap);
  }

  @override
  Future<void> close() async {}

  Future<String> _throwUnimplementedOrReturnContents(String? contents) async {
    if (contents == null) throw UnimplementedError();
    return contents;
  }
}

final fakeWipResponse = WipResponse({
  'id': 1,
  'result': {'fake': ''}
});
