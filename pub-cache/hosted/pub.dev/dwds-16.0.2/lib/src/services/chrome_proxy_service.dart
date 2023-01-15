// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:logging/logging.dart' hide LogRecord;
import 'package:pub_semver/pub_semver.dart' as semver;
import 'package:vm_service/vm_service.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import '../../data/debug_event.dart';
import '../../data/register_event.dart';
import '../connections/app_connection.dart';
import '../debugging/debugger.dart';
import '../debugging/execution_context.dart';
import '../debugging/inspector.dart';
import '../debugging/instance.dart';
import '../debugging/location.dart';
import '../debugging/modules.dart';
import '../debugging/remote_debugger.dart';
import '../debugging/skip_list.dart';
import '../events.dart';
import '../loaders/strategy.dart';
import '../readers/asset_reader.dart';
import '../services/expression_compiler.dart';
import '../utilities/dart_uri.dart';
import '../utilities/sdk_configuration.dart';
import '../utilities/shared.dart';
import 'expression_evaluator.dart';
import 'batched_expression_evaluator.dart';

/// A proxy from the chrome debug protocol to the dart vm service protocol.
class ChromeProxyService implements VmServiceInterface {
  /// Cache of all existing StreamControllers.
  ///
  /// These are all created through [onEvent].
  final _streamControllers = <String, StreamController<Event>>{};

  /// The root `VM` instance. There can only be one of these, but its isolates
  /// are dynamic and roughly map to chrome tabs.
  final VM _vm;

  /// Signals when isolate is initialized.
  Future<void> get isInitialized => _initializedCompleter.future;
  Completer<void> _initializedCompleter = Completer<void>();

  /// Signals when isolate starts.
  Future<void> get isStarted => _startedCompleter.future;
  Completer<void> _startedCompleter = Completer<void>();

  /// Signals when expression compiler is ready to evaluate.
  Future<void> get isCompilerInitialized => _compilerCompleter.future;
  Completer<void> _compilerCompleter = Completer<void>();

  /// The root at which we're serving.
  final String root;

  final RemoteDebugger remoteDebugger;
  final ExecutionContext executionContext;

  final AssetReader _assetReader;

  final Locations _locations;

  final SkipLists _skipLists;

  final Modules _modules;

  /// Provides debugger-related functionality.
  Future<Debugger> get debuggerFuture => _debuggerCompleter.future;
  final _debuggerCompleter = Completer<Debugger>();

  /// Provides variable inspection functionality.
  AppInspector get inspector {
    if (_inspector == null) {
      throw StateError('No running isolate (inspector is not set).');
    }
    return _inspector!;
  }

  AppInspector? _inspector;

  /// Determines if there an isolate running currently.
  ///
  /// [_inspector] is `null` iff the isolate is not running,
  /// for example, before the first isolate starts or during
  /// a hot restart.
  bool get _isIsolateRunning => _inspector != null;

  StreamSubscription<ConsoleAPIEvent>? _consoleSubscription;

  final _disabledBreakpoints = <Breakpoint>{};
  final _previousBreakpoints = <Breakpoint>{};

  final _logger = Logger('ChromeProxyService');

  final ExpressionCompiler? _compiler;
  ExpressionEvaluator? _expressionEvaluator;

  final SdkConfigurationProvider _sdkConfigurationProvider;

  bool terminatingIsolates = false;

  ChromeProxyService._(
    this._vm,
    this.root,
    this._assetReader,
    this.remoteDebugger,
    this._modules,
    this._locations,
    this._skipLists,
    this.executionContext,
    this._compiler,
    this._sdkConfigurationProvider,
  ) {
    final debugger = Debugger.create(
      remoteDebugger,
      _streamNotify,
      _locations,
      _skipLists,
      root,
    );
    debugger.then((value) => _debuggerCompleter.complete(value));
  }

  static Future<ChromeProxyService> create(
    RemoteDebugger remoteDebugger,
    String root,
    AssetReader assetReader,
    LoadStrategy loadStrategy,
    AppConnection appConnection,
    ExecutionContext executionContext,
    ExpressionCompiler? expressionCompiler,
    SdkConfigurationProvider sdkConfigurationProvider,
  ) async {
    final vm = VM(
      name: 'ChromeDebugProxy',
      operatingSystem: Platform.operatingSystem,
      startTime: DateTime.now().millisecondsSinceEpoch,
      version: Platform.version,
      isolates: [],
      isolateGroups: [],
      systemIsolates: [],
      systemIsolateGroups: [],
      targetCPU: 'Web',
      hostCPU: 'DWDS',
      architectureBits: -1,
      pid: -1,
    );

    final modules = Modules(root);
    final locations = Locations(assetReader, modules, root);
    final skipLists = SkipLists();
    final service = ChromeProxyService._(
      vm,
      root,
      assetReader,
      remoteDebugger,
      modules,
      locations,
      skipLists,
      executionContext,
      expressionCompiler,
      sdkConfigurationProvider,
    );
    unawaited(service.createIsolate(appConnection));
    return service;
  }

  /// Initializes metadata in [Locations], [Modules], and [ExpressionCompiler].
  Future<void> _initializeEntrypoint(String entrypoint) async {
    _locations.initialize(entrypoint);
    _modules.initialize(entrypoint);
    _skipLists.initialize();
    // We do not need to wait for compiler dependencies to be updated as the
    // [ExpressionEvaluator] is robust to evaluation requests during updates.
    unawaited(_updateCompilerDependencies(entrypoint));
  }

  Future<void> _updateCompilerDependencies(String entrypoint) async {
    final metadataProvider = globalLoadStrategy.metadataProviderFor(entrypoint);
    final moduleFormat = globalLoadStrategy.moduleFormat;
    final soundNullSafety = await metadataProvider.soundNullSafety;

    _logger.info('Initializing expression compiler for $entrypoint '
        'with sound null safety: $soundNullSafety');

    final compiler = _compiler;
    if (compiler != null) {
      await compiler.initialize(
          moduleFormat: moduleFormat, soundNullSafety: soundNullSafety);
      final dependencies =
          await globalLoadStrategy.moduleInfoForEntrypoint(entrypoint);
      await captureElapsedTime(() async {
        final result = await compiler.updateDependencies(dependencies);
        // Expression evaluation is ready after dependencies are updated.
        if (!_compilerCompleter.isCompleted) _compilerCompleter.complete();
        return result;
      }, (result) => DwdsEvent.compilerUpdateDependencies(entrypoint));
    }
  }

  /// Creates a new isolate.
  ///
  /// Only one isolate at a time is supported, but they should be cleaned up
  /// with [destroyIsolate] and recreated with this method there is a hot
  /// restart or full page refresh.
  Future<void> createIsolate(AppConnection appConnection) async {
    // Inspector is null if the previous isolate is destroyed.
    if (_isIsolateRunning) {
      throw UnsupportedError(
          'Cannot create multiple isolates for the same app');
    }
    // Waiting for the debugger to be ready before initializing the entrypoint.
    //
    // Note: moving `await debugger` after the `_initializeEntryPoint` call
    // causes `getcwd` system calls to fail. Since that system call is used
    // in first `Uri.base` call in the expression compiler service isolate,
    // the expression compiler service will fail to start.
    // Issue: https://github.com/dart-lang/webdev/issues/1282
    final debugger = await debuggerFuture;
    final entrypoint = appConnection.request.entrypointPath;
    await _initializeEntrypoint(entrypoint);
    final sdkConfiguration = await _sdkConfigurationProvider.configuration;

    debugger.notifyPausedAtStart();
    _inspector = await AppInspector.create(
      appConnection,
      remoteDebugger,
      _assetReader,
      _locations,
      root,
      debugger,
      executionContext,
      sdkConfiguration,
    );
    debugger.updateInspector(inspector);

    final compiler = _compiler;
    _expressionEvaluator = compiler == null
        ? null
        : BatchedExpressionEvaluator(
            entrypoint,
            inspector,
            debugger,
            _locations,
            _modules,
            compiler,
          );

    await debugger.reestablishBreakpoints(
        _previousBreakpoints, _disabledBreakpoints);
    _disabledBreakpoints.clear();

    unawaited(appConnection.onStart.then((_) async {
      await debugger.resumeFromStart();
      if (!_startedCompleter.isCompleted) {
        _startedCompleter.complete();
      } else {
        // See https://github.com/flutter/flutter/issues/117676:
        _logger
            .warning('Unexpected state, debugging may not work as expected.');
      }
    }));

    unawaited(appConnection.onDone.then((_) => destroyIsolate()));

    final isolateRef = inspector.isolateRef;
    final timestamp = DateTime.now().millisecondsSinceEpoch;

    // Listen for `registerExtension` and `postEvent` calls.
    _setUpChromeConsoleListeners(isolateRef);

    _vm.isolates?.add(isolateRef);

    _streamNotify(
        'Isolate',
        Event(
            kind: EventKind.kIsolateStart,
            timestamp: timestamp,
            isolate: isolateRef));
    _streamNotify(
        'Isolate',
        Event(
            kind: EventKind.kIsolateRunnable,
            timestamp: timestamp,
            isolate: isolateRef));

    // TODO: We shouldn't need to fire these events since they exist on the
    // isolate, but devtools doesn't recognize extensions after a page refresh
    // otherwise.
    for (var extensionRpc in inspector.isolate.extensionRPCs ?? []) {
      _streamNotify(
          'Isolate',
          Event(
              kind: EventKind.kServiceExtensionAdded,
              timestamp: timestamp,
              isolate: isolateRef)
            ..extensionRPC = extensionRpc);
    }

    // The service is considered initialized when the first isolate is created.
    if (!_initializedCompleter.isCompleted) _initializedCompleter.complete();
  }

  /// Should be called when there is a hot restart or full page refresh.
  ///
  /// Clears out the [_inspector] and all related cached information.
  void destroyIsolate() {
    _logger.fine('Destroying isolate');
    if (!_isIsolateRunning) return;
    final isolate = inspector.isolate;
    final isolateRef = inspector.isolateRef;

    _initializedCompleter = Completer<void>();
    _startedCompleter = Completer<void>();
    _compilerCompleter = Completer<void>();
    _streamNotify(
        'Isolate',
        Event(
            kind: EventKind.kIsolateExit,
            timestamp: DateTime.now().millisecondsSinceEpoch,
            isolate: isolateRef));
    _vm.isolates?.removeWhere((ref) => ref.id == isolate.id);
    _inspector = null;
    _previousBreakpoints.clear();
    _previousBreakpoints.addAll(isolate.breakpoints ?? []);
    _expressionEvaluator?.close();
    _consoleSubscription?.cancel();
    _consoleSubscription = null;
  }

  Future<void> disableBreakpoints() async {
    _disabledBreakpoints.clear();
    if (!_isIsolateRunning) return;
    final isolate = inspector.isolate;

    _disabledBreakpoints.addAll(isolate.breakpoints ?? []);
    for (var breakpoint in isolate.breakpoints?.toList() ?? []) {
      await (await debuggerFuture).removeBreakpoint(breakpoint.id);
    }
  }

  @override
  Future<Breakpoint> addBreakpoint(String isolateId, String scriptId, int line,
      {int? column}) async {
    await isInitialized;
    _checkIsolate('addBreakpoint', isolateId);
    return (await debuggerFuture).addBreakpoint(scriptId, line, column: column);
  }

  @override
  Future<Breakpoint> addBreakpointAtEntry(String isolateId, String functionId) {
    return _rpcNotSupportedFuture('addBreakpointAtEntry');
  }

  @override
  Future<Breakpoint> addBreakpointWithScriptUri(
      String isolateId, String scriptUri, int line,
      {int? column}) async {
    await isInitialized;
    _checkIsolate('addBreakpointWithScriptUri', isolateId);
    if (Uri.parse(scriptUri).scheme == 'dart') {
      // TODO(annagrin): Support setting breakpoints in dart SDK locations.
      // Issue: https://github.com/dart-lang/webdev/issues/1584
      throw RPCError(
          'addBreakpoint',
          102,
          'The VM is unable to add a breakpoint '
              'at the specified line or function: $scriptUri:$line:$column: '
              'breakpoints in dart SDK locations are not supported yet.');
    }
    final dartUri = DartUri(scriptUri, root);
    final scriptRef = await inspector.scriptRefFor(dartUri.serverPath);
    final scriptId = scriptRef?.id;
    if (scriptId == null) {
      throw RPCError(
          'addBreakpoint',
          102,
          'The VM is unable to add a breakpoint '
              'at the specified line or function: $scriptUri:$line:$column: '
              'cannot find script ID for ${dartUri.serverPath}');
    }
    return (await debuggerFuture).addBreakpoint(scriptId, line, column: column);
  }

  @override
  Future<Response> callServiceExtension(String method,
      {String? isolateId, Map? args}) async {
    await isInitialized;
    isolateId ??= _inspector?.isolate.id;
    _checkIsolate('callServiceExtension', isolateId);
    args ??= <String, String>{};
    final stringArgs = args.map((k, v) => MapEntry(
        k is String ? k : jsonEncode(k), v is String ? v : jsonEncode(v)));
    final expression = '''
${globalLoadStrategy.loadModuleSnippet}("dart_sdk").developer.invokeExtension(
    "$method", JSON.stringify(${jsonEncode(stringArgs)}));
''';
    final result = await inspector.jsEvaluate(expression, awaitPromise: true);
    final decodedResponse =
        jsonDecode(result.value as String) as Map<String, dynamic>;
    if (decodedResponse.containsKey('code') &&
        decodedResponse.containsKey('message') &&
        decodedResponse.containsKey('data')) {
      // ignore: only_throw_errors
      throw RPCError(method, decodedResponse['code'] as int,
          decodedResponse['message'] as String, decodedResponse['data'] as Map);
    } else {
      return Response()..json = decodedResponse;
    }
  }

  @override
  Future<Success> clearVMTimeline() {
    return _rpcNotSupportedFuture('clearVMTimeline');
  }

  Future<Response> _getEvaluationResult(
      Future<RemoteObject> Function() evaluation, String expression) async {
    try {
      final result = await evaluation();
      // Handle compilation errors, internal errors,
      // and reference errors from JavaScript evaluation in chrome.
      if (result.type.contains('Error')) {
        if (!result.type.startsWith('CompilationError')) {
          _logger.warning('Failed to evaluate expression \'$expression\': '
              '${result.type}: ${result.value}.');

          _logger.info('Please follow instructions at '
              'https://github.com/dart-lang/webdev/issues/956 '
              'to file a bug.');
        }
        return ErrorRef(
          kind: 'error',
          message: '${result.type}: ${result.value}',
          id: createId(),
        );
      }
      return (await _instanceRef(result));
    } on RPCError catch (_) {
      rethrow;
    } catch (e, s) {
      // Handle errors that throw exceptions, such as invalid JavaScript
      // generated by the expression evaluator.
      _logger.warning('Failed to evaluate expression \'$expression\'. ');
      _logger.info('Please follow instructions at '
          'https://github.com/dart-lang/webdev/issues/956 '
          'to file a bug.');
      _logger.info('$e:$s');
      return ErrorRef(kind: 'error', message: '<unknown>', id: createId());
    }
  }

  @override
  Future<Response> evaluate(
    String isolateId,
    String targetId,
    String expression, {
    Map<String, String>? scope,
    bool? disableBreakpoints,
  }) async {
    // TODO(798) - respect disableBreakpoints.
    return captureElapsedTime(() async {
      await isInitialized;
      final evaluator = _expressionEvaluator;
      if (evaluator != null) {
        await isCompilerInitialized;
        _checkIsolate('evaluate', isolateId);

        final library = await inspector.getLibrary(targetId);
        return await _getEvaluationResult(
            () => evaluator.evaluateExpression(
                isolateId, library?.uri, expression, scope),
            expression);
      }
      throw RPCError('evaluateInFrame', RPCError.kInvalidRequest,
          'Expression evaluation is not supported for this configuration.');
    }, (result) => DwdsEvent.evaluate(expression, result));
  }

  @override
  Future<Response> evaluateInFrame(
      String isolateId, int frameIndex, String expression,
      {Map<String, String>? scope, bool? disableBreakpoints}) async {
    // TODO(798) - respect disableBreakpoints.

    return captureElapsedTime(() async {
      await isInitialized;
      final evaluator = _expressionEvaluator;
      if (evaluator != null) {
        await isCompilerInitialized;
        _checkIsolate('evaluateInFrame', isolateId);

        return await _getEvaluationResult(
            () => evaluator.evaluateExpressionInFrame(
                isolateId, frameIndex, expression, scope),
            expression);
      }
      throw RPCError('evaluateInFrame', RPCError.kInvalidRequest,
          'Expression evaluation is not supported for this configuration.');
    }, (result) => DwdsEvent.evaluateInFrame(expression, result));
  }

  @override
  Future<AllocationProfile> getAllocationProfile(String isolateId,
      {bool? gc, bool? reset}) {
    return _rpcNotSupportedFuture('getAllocationProfile');
  }

  @override
  Future<ClassList> getClassList(String isolateId) {
    // See dart-lang/webdev/issues/971.
    return _rpcNotSupportedFuture('getClassList');
  }

  @override
  Future<FlagList> getFlagList() async {
    // VM flags do not apply to web apps.
    return FlagList(flags: []);
  }

  @override
  Future<InstanceSet> getInstances(
      String isolateId, String classId, int limit) {
    return _rpcNotSupportedFuture('getInstances');
  }

  @override
  Future<Isolate> getIsolate(String isolateId) async {
    return captureElapsedTime(() async {
      await isInitialized;
      _checkIsolate('getIsolate', isolateId);
      return inspector.isolate;
    }, (result) => DwdsEvent.getIsolate());
  }

  @override
  Future<MemoryUsage> getMemoryUsage(String isolateId) async {
    await isInitialized;
    _checkIsolate('getMemoryUsage', isolateId);
    return inspector.getMemoryUsage();
  }

  @override
  Future<Obj> getObject(String isolateId, String objectId,
      {int? offset, int? count}) async {
    await isInitialized;
    _checkIsolate('getObject', isolateId);
    return inspector.getObject(objectId, offset: offset, count: count);
  }

  @override
  Future<ScriptList> getScripts(String isolateId) async {
    return await captureElapsedTime(() async {
      await isInitialized;
      _checkIsolate('getScripts', isolateId);
      return inspector.getScripts();
    }, (result) => DwdsEvent.getScripts());
  }

  @override
  Future<SourceReport> getSourceReport(
    String isolateId,
    List<String> reports, {
    String? scriptId,
    int? tokenPos,
    int? endTokenPos,
    bool? forceCompile,
    bool? reportLines,
    List<String>? libraryFilters,
  }) async {
    return await captureElapsedTime(() async {
      await isInitialized;
      _checkIsolate('getSourceReport', isolateId);
      return await inspector.getSourceReport(
        reports,
        scriptId: scriptId,
        tokenPos: tokenPos,
        endTokenPos: endTokenPos,
        forceCompile: forceCompile,
        reportLines: reportLines,
        libraryFilters: libraryFilters,
      );
    }, (result) => DwdsEvent.getSourceReport());
  }

  /// Returns the current stack.
  ///
  /// Throws RPCError the corresponding isolate is not paused.
  ///
  /// The returned stack will contain up to [limit] frames if provided.
  @override
  Future<Stack> getStack(String isolateId, {int? limit}) async {
    await isInitialized;
    await isStarted;
    _checkIsolate('getStack', isolateId);
    return (await debuggerFuture).getStack(limit: limit);
  }

  @override
  Future<VM> getVM() async {
    return captureElapsedTime(() async {
      await isInitialized;
      return _vm;
    }, (result) => DwdsEvent.getVM());
  }

  @override
  Future<Timeline> getVMTimeline(
      {int? timeOriginMicros, int? timeExtentMicros}) {
    return _rpcNotSupportedFuture('getVMTimeline');
  }

  @override
  Future<TimelineFlags> getVMTimelineFlags() {
    return _rpcNotSupportedFuture('getVMTimelineFlags');
  }

  @override
  Future<Version> getVersion() async {
    final version = semver.Version.parse(vmServiceVersion);
    return Version(major: version.major, minor: version.minor);
  }

  @override
  Future<Response> invoke(
      String isolateId, String targetId, String selector, List argumentIds,
      {bool? disableBreakpoints}) async {
    await isInitialized;
    _checkIsolate('invoke', isolateId);
    // TODO(798) - respect disableBreakpoints.
    final remote = await inspector.invoke(targetId, selector, argumentIds);
    return _instanceRef(remote);
  }

  @override
  Future<Success> kill(String isolateId) {
    return _rpcNotSupportedFuture('kill');
  }

  @override
  Stream<Event> onEvent(String streamId) {
    return _streamControllers.putIfAbsent(streamId, () {
      switch (streamId) {
        case EventStreams.kExtension:
          return StreamController<Event>.broadcast();
        case EventStreams.kIsolate:
          // TODO: right now we only support the `ServiceExtensionAdded` event
          // for the Isolate stream.
          return StreamController<Event>.broadcast();
        case EventStreams.kVM:
          return StreamController<Event>.broadcast();
        case EventStreams.kGC:
          return StreamController<Event>.broadcast();
        case EventStreams.kTimeline:
          return StreamController<Event>.broadcast();
        case EventStreams.kService:
          return StreamController<Event>.broadcast();
        case EventStreams.kDebug:
          return StreamController<Event>.broadcast();
        case EventStreams.kLogging:
          return StreamController<Event>.broadcast();
        case EventStreams.kStdout:
          return _chromeConsoleStreamController(
              (e) => _stdoutTypes.contains(e.type));
        case EventStreams.kStderr:
          return _chromeConsoleStreamController(
              (e) => _stderrTypes.contains(e.type),
              includeExceptions: true);
        default:
          throw RPCError(
            'streamListen',
            RPCError.kMethodNotFound,
            'The stream `$streamId` is not supported on web devices',
          );
      }
    }).stream;
  }

  @override
  Future<Success> pause(String isolateId) async {
    await isInitialized;
    _checkIsolate('pause', isolateId);
    return (await debuggerFuture).pause();
  }

  // Note: Ignore the optional local parameter, it is there to keep the method
  // signature consistent with the VM service interface.
  @override
  Future<UriList> lookupResolvedPackageUris(String isolateId, List<String> uris,
      {bool? local}) async {
    await isInitialized;
    _checkIsolate('lookupResolvedPackageUris', isolateId);
    return UriList(uris: uris.map(DartUri.toResolvedUri).toList());
  }

  @override
  Future<UriList> lookupPackageUris(String isolateId, List<String> uris) async {
    await isInitialized;
    _checkIsolate('lookupPackageUris', isolateId);
    return UriList(uris: uris.map(DartUri.toPackageUri).toList());
  }

  @override
  Future<Success> registerService(String service, String alias) async {
    return _rpcNotSupportedFuture('registerService');
  }

  @override
  Future<ReloadReport> reloadSources(String isolateId,
      {bool? force, bool? pause, String? rootLibUri, String? packagesUri}) {
    return Future.error(RPCError(
      'reloadSources',
      RPCError.kMethodNotFound,
      'Hot reload not supported on web devices',
    ));
  }

  @override
  Future<Success> removeBreakpoint(
      String isolateId, String breakpointId) async {
    await isInitialized;
    _checkIsolate('removeBreakpoint', isolateId);
    _disabledBreakpoints
        .removeWhere((breakpoint) => breakpoint.id == breakpointId);
    return (await debuggerFuture).removeBreakpoint(breakpointId);
  }

  @override
  Future<Success> resume(String isolateId,
      {String? step, int? frameIndex}) async {
    if (inspector.appConnection.isStarted) {
      return captureElapsedTime(() async {
        await isInitialized;
        await isStarted;
        _checkIsolate('resume', isolateId);
        return await (await debuggerFuture)
            .resume(step: step, frameIndex: frameIndex);
      }, (result) => DwdsEvent.resume(step));
    } else {
      inspector.appConnection.runMain();
      return Success();
    }
  }

  @override
  Future<Success> setIsolatePauseMode(String isolateId,
      {String? exceptionPauseMode, bool? shouldPauseOnExit}) async {
    // TODO(elliette): Is there a way to respect the shouldPauseOnExit parameter
    // in Chrome?
    return setExceptionPauseMode(
        isolateId, exceptionPauseMode ?? ExceptionPauseMode.kNone);
  }

  @override
  Future<Success> setExceptionPauseMode(String isolateId, String mode) async {
    await isInitialized;
    _checkIsolate('setExceptionPauseMode', isolateId);
    return (await debuggerFuture).setExceptionPauseMode(mode);
  }

  @override
  Future<Success> setFlag(String name, String value) {
    return _rpcNotSupportedFuture('setFlag');
  }

  @override
  Future<Success> setLibraryDebuggable(
      String isolateId, String libraryId, bool isDebuggable) {
    return _rpcNotSupportedFuture('setLibraryDebuggable');
  }

  @override
  Future<Success> setName(String isolateId, String name) async {
    await isInitialized;
    _checkIsolate('setName', isolateId);
    inspector.isolate.name = name;
    return Success();
  }

  @override
  Future<Success> setVMName(String name) async {
    _vm.name = name;
    _streamNotify(
        'VM',
        Event(
            kind: EventKind.kVMUpdate,
            timestamp: DateTime.now().millisecondsSinceEpoch,
            // We are not guaranteed to have an isolate at this point an time.
            isolate: null)
          ..vm = toVMRef(_vm));
    return Success();
  }

  @override
  Future<Success> setVMTimelineFlags(List<String> recordedStreams) {
    return _rpcNotSupportedFuture('setVMTimelineFlags');
  }

  @override
  Future<Success> streamCancel(String streamId) {
    // TODO: We should implement this (as we've already implemented
    // streamListen).
    return _rpcNotSupportedFuture('streamCancel');
  }

  @override
  Future<Success> streamListen(String streamId) async {
    // TODO: This should return an error if the stream is already being listened
    // to.
    onEvent(streamId);
    return Success();
  }

  @override
  Future<Success> clearCpuSamples(String isolateId) {
    return _rpcNotSupportedFuture('clearCpuSamples');
  }

  @override
  Future<CpuSamples> getCpuSamples(
      String isolateId, int timeOriginMicros, int timeExtentMicros) {
    return _rpcNotSupportedFuture('getCpuSamples');
  }

  /// Returns a streamController that listens for console logs from chrome and
  /// adds all events passing [filter] to the stream.
  StreamController<Event> _chromeConsoleStreamController(
      bool Function(ConsoleAPIEvent) filter,
      {bool includeExceptions = false}) {
    late StreamController<Event> controller;
    StreamSubscription? chromeConsoleSubscription;
    StreamSubscription? exceptionsSubscription;

    // This is an edge case for this lint apparently
    //
    // ignore: join_return_with_assignment
    controller = StreamController<Event>.broadcast(onCancel: () {
      chromeConsoleSubscription?.cancel();
      exceptionsSubscription?.cancel();
    }, onListen: () {
      chromeConsoleSubscription = remoteDebugger.onConsoleAPICalled.listen((e) {
        if (!_isIsolateRunning) return;
        final isolateRef = inspector.isolateRef;
        if (!filter(e)) return;
        final args = e.params?['args'] as List?;
        final item = args?[0] as Map?;
        final value = '${item?["value"]}\n';
        controller.add(Event(
            kind: EventKind.kWriteEvent,
            timestamp: DateTime.now().millisecondsSinceEpoch,
            isolate: isolateRef)
          ..bytes = base64.encode(utf8.encode(value))
          ..timestamp = e.timestamp.toInt());
      });
      if (includeExceptions) {
        exceptionsSubscription =
            remoteDebugger.onExceptionThrown.listen((e) async {
          if (!_isIsolateRunning) return;
          final isolateRef = inspector.isolateRef;
          var description = e.exceptionDetails.exception?.description;
          if (description != null) {
            description = await inspector.mapExceptionStackTrace(description);
          }
          controller.add(Event(
              kind: EventKind.kWriteEvent,
              timestamp: DateTime.now().millisecondsSinceEpoch,
              isolate: isolateRef)
            ..bytes = base64.encode(utf8.encode(description ?? '')));
        });
      }
    });
    return controller;
  }

  /// Parses the [BatchedDebugEvents] and emits corresponding Dart VM Service
  /// protocol [Event]s.
  Future<void> parseBatchedDebugEvents(BatchedDebugEvents debugEvents) async {
    for (var debugEvent in debugEvents.events) {
      await parseDebugEvent(debugEvent);
    }
  }

  /// Parses the [DebugEvent] and emits a corresponding Dart VM Service
  /// protocol [Event].
  Future<void> parseDebugEvent(DebugEvent debugEvent) async {
    if (terminatingIsolates) return;
    if (!_isIsolateRunning) return;
    final isolateRef = inspector.isolateRef;

    _streamNotify(
        EventStreams.kExtension,
        Event(
            kind: EventKind.kExtension,
            timestamp: DateTime.now().millisecondsSinceEpoch,
            isolate: isolateRef)
          ..extensionKind = debugEvent.kind
          ..extensionData = ExtensionData.parse(
              jsonDecode(debugEvent.eventData) as Map<String, dynamic>));
  }

  /// Parses the [RegisterEvent] and emits a corresponding Dart VM Service
  /// protocol [Event].
  Future<void> parseRegisterEvent(RegisterEvent registerEvent) async {
    if (terminatingIsolates) return;
    if (!_isIsolateRunning) return;

    final isolate = inspector.isolate;
    final isolateRef = inspector.isolateRef;
    final service = registerEvent.eventData;
    isolate.extensionRPCs?.add(service);

    _streamNotify(
        EventStreams.kIsolate,
        Event(
            kind: EventKind.kServiceExtensionAdded,
            timestamp: DateTime.now().millisecondsSinceEpoch,
            isolate: isolateRef)
          ..extensionRPC = service);
  }

  /// Listens for chrome console events and handles the ones we care about.
  void _setUpChromeConsoleListeners(IsolateRef isolateRef) {
    _consoleSubscription =
        remoteDebugger.onConsoleAPICalled.listen((event) async {
      if (terminatingIsolates) return;
      if (event.type != 'debug') return;
      if (!_isIsolateRunning) return;

      final isolate = inspector.isolate;
      if (isolateRef.id != isolate.id) return;

      final firstArgValue = event.args[0].value as String;
      // TODO(nshahan) - Migrate 'inspect' and 'log' events to the injected
      // client communication approach as well?
      switch (firstArgValue) {
        case 'dart.developer.inspect':
          // All inspected objects should be real objects.
          if (event.args[1].type != 'object') break;

          final inspectee = await _instanceRef(event.args[1]);
          _streamNotify(
              EventStreams.kDebug,
              Event(
                  kind: EventKind.kInspect,
                  timestamp: DateTime.now().millisecondsSinceEpoch,
                  isolate: isolateRef)
                ..inspectee = inspectee
                ..timestamp = event.timestamp.toInt());
          break;
        case 'dart.developer.log':
          _handleDeveloperLog(isolateRef, event);
          break;
        default:
          break;
      }
    });
  }

  void _streamNotify(String streamId, Event event) {
    final controller = _streamControllers[streamId];
    if (controller == null) return;
    controller.add(event);
  }

  void _handleDeveloperLog(IsolateRef isolateRef, ConsoleAPIEvent event) async {
    final logObject = event.params?['args'][1] as Map?;
    final logParams = <String, RemoteObject>{};
    for (dynamic obj in logObject?['preview']?['properties'] ?? {}) {
      if (obj['name'] != null && obj is Map<String, dynamic>) {
        logParams[obj['name'] as String] = RemoteObject(obj);
      }
    }

    final logRecord = LogRecord(
      message: await _instanceRef(logParams['message']),
      loggerName: await _instanceRef(logParams['name']),
      level: logParams['level'] != null
          ? int.tryParse(logParams['level']!.value.toString())
          : 0,
      error: await _instanceRef(logParams['error']),
      time: event.timestamp.toInt(),
      sequenceNumber: logParams['sequenceNumber'] != null
          ? int.tryParse(logParams['sequenceNumber']!.value.toString())
          : 0,
      stackTrace: await _instanceRef(logParams['stackTrace']),
      zone: await _instanceRef(logParams['zone']),
    );

    _streamNotify(
      EventStreams.kLogging,
      Event(
          kind: EventKind.kLogging,
          timestamp: DateTime.now().millisecondsSinceEpoch,
          isolate: isolateRef)
        ..logRecord = logRecord
        ..timestamp = event.timestamp.toInt(),
    );
  }

  @override
  Future<Timestamp> getVMTimelineMicros() {
    return _rpcNotSupportedFuture('getVMTimelineMicros');
  }

  @override
  Future<InboundReferences> getInboundReferences(
      String isolateId, String targetId, int limit) {
    return _rpcNotSupportedFuture('getInboundReferences');
  }

  @override
  Future<RetainingPath> getRetainingPath(
      String isolateId, String targetId, int limit) {
    return _rpcNotSupportedFuture('getRetainingPath');
  }

  @override
  Future<Success> requestHeapSnapshot(String isolateId) {
    return _rpcNotSupportedFuture('requestHeapSnapshot');
  }

  @override
  Future<IsolateGroup> getIsolateGroup(String isolateGroupId) {
    return _rpcNotSupportedFuture('getIsolateGroup');
  }

  @override
  Future<MemoryUsage> getIsolateGroupMemoryUsage(String isolateGroupId) {
    return _rpcNotSupportedFuture('getIsolateGroupMemoryUsage');
  }

  @override
  Future<ProtocolList> getSupportedProtocols() async {
    final version = semver.Version.parse(vmServiceVersion);
    return ProtocolList(protocols: [
      Protocol(
        protocolName: 'VM Service',
        major: version.major,
        minor: version.minor,
      )
    ]);
  }

  Future<InstanceRef> _instanceRef(RemoteObject? obj) async {
    final instance = obj == null ? null : await inspector.instanceRefFor(obj);
    return instance ?? InstanceHelper.kNullInstanceRef;
  }

  static RPCError _rpcNotSupported(String method) {
    return RPCError(method, RPCError.kMethodNotFound,
        '$method: Not supported on web devices');
  }

  static Future<T> _rpcNotSupportedFuture<T>(String method) {
    return Future.error(_rpcNotSupported(method));
  }

  @override
  Future<ProcessMemoryUsage> getProcessMemoryUsage() =>
      _rpcNotSupportedFuture('getProcessMemoryUsage');

  @override
  Future<PortList> getPorts(String isolateId) => throw UnimplementedError();

  @override
  Future<CpuSamples> getAllocationTraces(String isolateId,
          {int? timeOriginMicros, int? timeExtentMicros, String? classId}) =>
      throw UnimplementedError();

  @override
  Future<Success> setTraceClassAllocation(
          String isolateId, String classId, bool enable) =>
      throw UnimplementedError();

  @override
  Future<Breakpoint> setBreakpointState(
          String isolateId, String breakpointId, bool enable) =>
      throw UnimplementedError();

  @override
  Future<Success> streamCpuSamplesWithUserTag(List<String> userTags) =>
      throw UnimplementedError();

  /// Prevent DWDS from blocking Dart SDK rolls if changes in package:vm_service
  /// are unimplemented in DWDS.
  @override
  dynamic noSuchMethod(Invocation invocation) {
    return super.noSuchMethod(invocation);
  }

  /// Validate that isolateId matches the current isolate we're connected to and
  /// return that isolate.
  ///
  /// This is useful to call at the beginning of API methods that are passed an
  /// isolate id.
  Isolate _checkIsolate(String methodName, String? isolateId) {
    final currentIsolateId = inspector.isolate.id;
    if (currentIsolateId == null) {
      throw StateError('No running isolate ID');
    }
    if (isolateId != currentIsolateId) {
      _throwSentinel(methodName, SentinelKind.kCollected,
          'Unrecognized isolateId: $isolateId');
    }
    return inspector.isolate;
  }

  static Never _throwSentinel(String method, String kind, String message) {
    final data = <String, String>{'kind': kind, 'valueAsString': message};
    throw SentinelException.parse(method, data);
  }
}

/// The `type`s of [ConsoleAPIEvent]s that are treated as `stderr` logs.
const _stderrTypes = ['error'];

/// The `type`s of [ConsoleAPIEvent]s that are treated as `stdout` logs.
const _stdoutTypes = ['log', 'info', 'warning'];
