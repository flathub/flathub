// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.import 'dart:async';

import 'package:async/async.dart';
import 'package:collection/collection.dart';
import 'package:logging/logging.dart';
import 'package:vm_service/vm_service.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import '../connections/app_connection.dart';
import '../loaders/strategy.dart';
import '../readers/asset_reader.dart';
import '../utilities/conversions.dart';
import '../utilities/dart_uri.dart';
import '../utilities/domain.dart';
import '../utilities/sdk_configuration.dart';
import '../utilities/shared.dart';
import 'classes.dart';
import 'debugger.dart';
import 'execution_context.dart';
import 'instance.dart';
import 'libraries.dart';
import 'location.dart';
import 'remote_debugger.dart';

/// An inspector for a running Dart application contained in the
/// [WipConnection].
///
/// Provides information about currently loaded scripts and objects and support
/// for eval.
class AppInspector implements AppInspectorInterface {
  final _scriptCacheMemoizer = AsyncMemoizer<List<ScriptRef>>();

  Future<List<ScriptRef>> get scriptRefs => _populateScriptCaches();

  final _logger = Logger('AppInspector');

  /// Map of scriptRef ID to [ScriptRef].
  final _scriptRefsById = <String, ScriptRef>{};

  /// Map of Dart server path to [ScriptRef].
  final _serverPathToScriptRef = <String, ScriptRef>{};

  /// Map of [ScriptRef] id to containing [LibraryRef] id.
  final _scriptIdToLibraryId = <String, String>{};

  /// Map of [Library] id to included [ScriptRef]s.
  final _libraryIdToScriptRefs = <String, List<ScriptRef>>{};

  @override
  RemoteDebugger get remoteDebugger => _remoteDebugger;
  final RemoteDebugger _remoteDebugger;

  @override
  Isolate get isolate => _isolate;
  final Isolate _isolate;

  @override
  IsolateRef get isolateRef => _isolateRef;
  final IsolateRef _isolateRef;

  @override
  AppConnection get appConnection => _appConnection;
  final AppConnection _appConnection;

  final ExecutionContext _executionContext;

  late final LibraryHelper _libraryHelper;
  late final ClassHelper _classHelper;
  late final InstanceHelper _instanceHelper;

  final AssetReader _assetReader;
  final Locations _locations;

  /// The root URI from which the application is served.
  final String _root;
  final SdkConfiguration _sdkConfiguration;

  /// JavaScript expression that evaluates to the Dart stack trace mapper.
  static const stackTraceMapperExpression = '\$dartStackTraceUtility.mapper';

  /// Regex used to extract the message from an exception description.
  static final exceptionMessageRegex = RegExp(r'^.*$', multiLine: true);

  AppInspector._(
    this._appConnection,
    this._isolate,
    this._remoteDebugger,
    this._assetReader,
    this._locations,
    this._root,
    this._executionContext,
    this._sdkConfiguration,
  ) : _isolateRef = _toIsolateRef(_isolate);

  Future<void> initialize(
    LibraryHelper libraryHelper,
    ClassHelper classHelper,
    InstanceHelper instanceHelper,
  ) async {
    _libraryHelper = libraryHelper;
    _classHelper = classHelper;
    _instanceHelper = instanceHelper;

    final libraries = await _libraryHelper.libraryRefs;
    isolate.rootLib = await _libraryHelper.rootLib;
    isolate.libraries?.addAll(libraries);

    final scripts = await scriptRefs;

    await DartUri.initialize(_sdkConfiguration);
    await DartUri.recordAbsoluteUris(
        libraries.map((lib) => lib.uri).whereNotNull());
    await DartUri.recordAbsoluteUris(
        scripts.map((script) => script.uri).whereNotNull());

    isolate.extensionRPCs?.addAll(await _getExtensionRpcs());
  }

  static IsolateRef _toIsolateRef(Isolate isolate) => IsolateRef(
        id: isolate.id,
        name: isolate.name,
        number: isolate.number,
        isSystemIsolate: isolate.isSystemIsolate,
      );

  static Future<AppInspector> create(
    AppConnection appConnection,
    RemoteDebugger remoteDebugger,
    AssetReader assetReader,
    Locations locations,
    String root,
    Debugger debugger,
    ExecutionContext executionContext,
    SdkConfiguration sdkConfiguration,
  ) async {
    final id = createId();
    final time = DateTime.now().millisecondsSinceEpoch;
    final name = 'main()';
    final isolate = Isolate(
        id: id,
        number: id,
        name: name,
        startTime: time,
        runnable: true,
        pauseOnExit: false,
        pauseEvent: Event(
            kind: EventKind.kPauseStart,
            timestamp: time,
            isolate: IsolateRef(
              id: id,
              name: name,
              number: id,
              isSystemIsolate: false,
            )),
        livePorts: 0,
        libraries: [],
        breakpoints: [],
        exceptionPauseMode: debugger.pauseState,
        isSystemIsolate: false,
        isolateFlags: [])
      ..extensionRPCs = [];
    final inspector = AppInspector._(
      appConnection,
      isolate,
      remoteDebugger,
      assetReader,
      locations,
      root,
      executionContext,
      sdkConfiguration,
    );

    debugger.updateInspector(inspector);

    final libraryHelper = LibraryHelper(inspector);
    final classHelper = ClassHelper(inspector);
    final instanceHelper = InstanceHelper(inspector, debugger);

    await inspector.initialize(
      libraryHelper,
      classHelper,
      instanceHelper,
    );
    return inspector;
  }

  /// Returns the ID for the execution context or null if not found.
  @override
  Future<int?> get contextId async {
    try {
      return await _executionContext.id;
    } catch (e, s) {
      _logger.severe('Missing execution context ID: ', e, s);
      return null;
    }
  }

  /// Get the value of the field named [fieldName] from [receiver].
  @override
  Future<RemoteObject> loadField(RemoteObject receiver, String fieldName) {
    final load = '''
        function() {
          return ${globalLoadStrategy.loadModuleSnippet}("dart_sdk").dart.dloadRepl(this, "$fieldName");
        }
        ''';
    return jsCallFunctionOn(receiver, load, []);
  }

  /// Call a method by name on [receiver], with arguments [positionalArgs] and
  /// [namedArgs].
  Future<RemoteObject> _invokeMethod(RemoteObject receiver, String methodName,
      [List<RemoteObject> positionalArgs = const [],
      Map namedArgs = const {}]) async {
    // TODO(alanknight): Support named arguments.
    if (namedArgs.isNotEmpty) {
      throw UnsupportedError('Named arguments are not yet supported');
    }
    // We use the JS pseudo-variable 'arguments' to get the list of all arguments.
    final send = '''
        function () {
          if (!(this.__proto__)) { return 'Instance of PlainJavaScriptObject';}
          return ${globalLoadStrategy.loadModuleSnippet}("dart_sdk").dart.dsendRepl(this, "$methodName", arguments);
        }
        ''';
    final remote = await jsCallFunctionOn(receiver, send, positionalArgs);
    return remote;
  }

  /// Calls Chrome's Runtime.callFunctionOn method.
  ///
  /// [evalExpression] should be a JS function definition that can accept
  /// [arguments].
  @override
  Future<RemoteObject> jsCallFunctionOn(RemoteObject receiver,
      String evalExpression, List<RemoteObject> arguments,
      {bool returnByValue = false}) async {
    final jsArguments = arguments.map(callArgumentFor).toList();
    final response =
        await remoteDebugger.sendCommand('Runtime.callFunctionOn', params: {
      'functionDeclaration': evalExpression,
      'arguments': jsArguments,
      'objectId': receiver.objectId,
      'returnByValue': returnByValue,
    });
    final result =
        getResultOrHandleError(response, evalContents: evalExpression);
    return RemoteObject(result);
  }

  /// Calls Chrome's Runtime.callFunctionOn method with a global function.
  ///
  /// [evalExpression] should be a JS function definition that can accept
  /// [arguments].
  Future<RemoteObject> _jsCallFunction(
      String evalExpression, List<Object> arguments,
      {bool returnByValue = false}) async {
    final jsArguments = arguments.map(callArgumentFor).toList();
    final response =
        await remoteDebugger.sendCommand('Runtime.callFunctionOn', params: {
      'functionDeclaration': evalExpression,
      'arguments': jsArguments,
      'executionContextId': await contextId,
      'returnByValue': returnByValue,
    });
    final result =
        getResultOrHandleError(response, evalContents: evalExpression);
    return RemoteObject(result);
  }

  /// Invoke the function named [selector] on the object identified by
  /// [targetId].
  ///
  /// The [targetId] can be the URL of a Dart library, in which case this means
  /// invoking a top-level function. The [arguments] are always strings that are
  /// Dart object Ids (which can also be Chrome RemoteObject objectIds that are
  /// for non-Dart JS objects.)
  @override
  Future<RemoteObject> invoke(
      String targetId, String selector, List<dynamic> arguments) async {
    final remoteArguments =
        arguments.cast<String>().map(remoteObjectFor).toList();
    // We special case the Dart library, where invokeMethod won't work because
    // it's not really a Dart object.
    if (isLibraryId(targetId)) {
      final library = await getObject(targetId) as Library;
      return await _invokeLibraryFunction(library, selector, remoteArguments);
    } else {
      return _invokeMethod(
          remoteObjectFor(targetId), selector, remoteArguments);
    }
  }

  /// Invoke the function named [selector] from [library] with [arguments].
  Future<RemoteObject> _invokeLibraryFunction(
      Library library, String selector, List<RemoteObject> arguments) {
    return _evaluateInLibrary(
        library,
        'function () { return this.$selector.apply(this, arguments);}',
        arguments);
  }

  /// Evaluate [expression] by calling Chrome's Runtime.evaluate.
  @override
  Future<RemoteObject> jsEvaluate(String expression,
      {bool returnByValue = false, bool awaitPromise = false}) async {
    // TODO(alanknight): Support a version with arguments if needed.
    final response =
        await remoteDebugger.sendCommand('Runtime.evaluate', params: {
      'expression': expression,
      'returnByValue': returnByValue,
      'awaitPromise': awaitPromise,
      'contextId': await contextId,
    });
    final result = getResultOrHandleError(response, evalContents: expression);
    return RemoteObject(result);
  }

  /// Evaluate the JS function with source [jsFunction] in the context of
  /// [library] with [arguments].
  Future<RemoteObject> _evaluateInLibrary(
      Library library, String jsFunction, List<RemoteObject> arguments) async {
    final libraryUri = library.uri;
    if (libraryUri == null) {
      throwInvalidParam('invoke', 'library uri is null');
    }
    final findLibrary = '''
(function() {
  ${globalLoadStrategy.loadLibrarySnippet(libraryUri)};
  return library;
})();
''';
    final remoteLibrary = await jsEvaluate(findLibrary);
    return jsCallFunctionOn(remoteLibrary, jsFunction, arguments);
  }

  /// Call [function] with objects referred by [argumentIds] as arguments.
  @override
  Future<RemoteObject> callFunction(
      String function, Iterable<String> argumentIds) async {
    final arguments = argumentIds.map(remoteObjectFor).toList();
    return _jsCallFunction(function, arguments);
  }

  @override
  Future<InstanceRef?> instanceRefFor(Object value) =>
      _instanceHelper.instanceRefFor(value);

  Future<Instance?> instanceFor(RemoteObject value) =>
      _instanceHelper.instanceFor(value);

  @override
  Future<LibraryRef?> libraryRefFor(String objectId) =>
      _libraryHelper.libraryRefFor(objectId);

  @override
  Future<Library?> getLibrary(String objectId) async {
    final libraryRef = await libraryRefFor(objectId);
    if (libraryRef == null) return null;
    return _libraryHelper.libraryFor(libraryRef);
  }

  @override
  Future<Obj> getObject(String objectId, {int? offset, int? count}) async {
    try {
      final library = await getLibrary(objectId);
      if (library != null) {
        return library;
      }
      final clazz = await _classHelper.forObjectId(objectId);
      if (clazz != null) {
        return clazz;
      }
      final scriptRef = _scriptRefsById[objectId];
      if (scriptRef != null) {
        return _getScript(scriptRef);
      }
      final instance = await _instanceHelper
          .instanceFor(remoteObjectFor(objectId), offset: offset, count: count);
      if (instance != null) {
        return instance;
      }
    } catch (e, s) {
      _logger.fine('getObject $objectId failed', e, s);
      rethrow;
    }
    throw UnsupportedError('Only libraries, instances, classes, and scripts '
        'are supported for getObject');
  }

  Future<Script> _getScript(ScriptRef scriptRef) async {
    final scriptId = scriptRef.id;
    final scriptUri = scriptRef.uri;
    if (scriptId == null || scriptUri == null) {
      throwInvalidParam('getObject', 'No script info for script $scriptRef');
    }
    final serverPath = DartUri(scriptUri, _root).serverPath;
    final source = await _assetReader.dartSourceContents(serverPath);
    if (source == null) {
      throwInvalidParam('getObject',
          'No source for $scriptRef  with serverPath: $serverPath');
    }
    final libraryId = _scriptIdToLibraryId[scriptId];
    if (libraryId == null) {
      throwInvalidParam('getObject', 'No library for script $scriptRef');
    }
    return Script(
        uri: scriptRef.uri,
        library: await libraryRefFor(libraryId),
        id: scriptId)
      ..tokenPosTable = await _locations.tokenPosTableFor(serverPath)
      ..source = source;
  }

  @override
  Future<MemoryUsage> getMemoryUsage() async {
    final response = await remoteDebugger.sendCommand('Runtime.getHeapUsage');
    final result = response.result;
    if (result == null) {
      throw RPCError('getMemoryUsage', RPCError.kInternalError,
          'Null result from chrome Devtools.');
    }
    final jsUsage = HeapUsage(result);
    final usage = MemoryUsage.parse({
      'heapUsage': jsUsage.usedSize,
      'heapCapacity': jsUsage.totalSize,
      'externalUsage': 0,
    });
    if (usage == null) {
      throw RPCError('getMemoryUsage', RPCError.kInternalError,
          'Failed to parse memory usage result.');
    }
    return usage;
  }

  /// Returns the [ScriptRef] for the provided Dart server path [uri].
  @override
  Future<ScriptRef?> scriptRefFor(String uri) async {
    await _populateScriptCaches();
    return _serverPathToScriptRef[uri];
  }

  /// Returns the [ScriptRef]s in the library with [libraryId].
  @override
  Future<List<ScriptRef>> scriptRefsForLibrary(String libraryId) async {
    await _populateScriptCaches();
    return _libraryIdToScriptRefs[libraryId] ?? [];
  }

  /// Return the VM SourceReport for the given parameters.
  ///
  /// Currently this implements the 'PossibleBreakpoints' report kind.
  @override
  Future<SourceReport> getSourceReport(
    List<String> reports, {
    String? scriptId,
    int? tokenPos,
    int? endTokenPos,
    bool? forceCompile,
    bool? reportLines,
    List<String>? libraryFilters,
  }) {
    if (reports.contains(SourceReportKind.kCoverage)) {
      throwInvalidParam('getSourceReport',
          'Source report kind ${SourceReportKind.kCoverage} not supported');
    }

    if (reports.isEmpty) {
      throwInvalidParam('getSourceReport',
          'Invalid parameter: no value for source report kind provided.');
    }

    if (reports.length > 1 ||
        reports.first != SourceReportKind.kPossibleBreakpoints) {
      throwInvalidParam('getSourceReport', 'Unsupported source report kind.');
    }

    return _getPossibleBreakpoints(scriptId);
  }

  Future<SourceReport> _getPossibleBreakpoints(String? scriptId) async {
    // TODO(devoncarew): Consider adding some caching for this method.

    final scriptRef = scriptWithId(scriptId);
    if (scriptRef == null) {
      throwInvalidParam('getSourceReport', 'scriptRef not found for $scriptId');
    }
    final scriptUri = scriptRef.uri;
    if (scriptUri == null) {
      throwInvalidParam('getSourceReport', 'scriptUri not found for $scriptId');
    }

    final dartUri = DartUri(scriptUri, _root);
    final mappedLocations =
        await _locations.locationsForDart(dartUri.serverPath);
    // Unlike the Dart VM, the token positions match exactly to the possible
    // breakpoints. This is because the token positions are derived from the
    // DDC source maps which Chrome also uses.
    final tokenPositions = <int>[
      for (var location in mappedLocations) location.tokenPos
    ];
    tokenPositions.sort();

    final range = SourceReportRange(
      scriptIndex: 0,
      startPos: tokenPositions.isEmpty ? -1 : tokenPositions.first,
      endPos: tokenPositions.isEmpty ? -1 : tokenPositions.last,
      compiled: true,
      possibleBreakpoints: tokenPositions,
    );

    final ranges = [range];
    return SourceReport(scripts: [scriptRef], ranges: ranges);
  }

  /// All the scripts in the isolate.
  @override
  Future<ScriptList> getScripts() async {
    return ScriptList(scripts: await scriptRefs);
  }

  /// Request and cache <ScriptRef>s for all the scripts in the application.
  ///
  /// This populates [_scriptRefsById], [_scriptIdToLibraryId],
  /// [_libraryIdToScriptRefs] and [_serverPathToScriptRef].
  ///
  /// It is a one-time operation, because if we do a
  /// reload the inspector will get re-created.
  ///
  /// Returns the list of scripts refs cached.
  Future<List<ScriptRef>> _populateScriptCaches() async {
    return _scriptCacheMemoizer.runOnce(() async {
      final libraryUris = [
        for (var library in isolate.libraries ?? []) library.uri
      ];
      final scripts = await globalLoadStrategy
          .metadataProviderFor(appConnection.request.entrypointPath)
          .scripts;
      // For all the non-dart: libraries, find their parts and create scriptRefs
      // for them.
      final userLibraries =
          libraryUris.where((uri) => !uri.startsWith('dart:'));
      for (var uri in userLibraries) {
        final parts = scripts[uri];
        final scriptRefs = [
          ScriptRef(uri: uri, id: createId()),
          for (var part in parts ?? []) ScriptRef(uri: part, id: createId())
        ];
        final libraryRef = await _libraryHelper.libraryRefFor(uri);
        final libraryId = libraryRef?.id;
        if (libraryId != null) {
          final libraryIdToScriptRefs = _libraryIdToScriptRefs.putIfAbsent(
              libraryId, () => <ScriptRef>[]);
          for (var scriptRef in scriptRefs) {
            final scriptId = scriptRef.id;
            final scriptUri = scriptRef.uri;
            if (scriptId != null && scriptUri != null) {
              _scriptRefsById[scriptId] = scriptRef;
              _scriptIdToLibraryId[scriptId] = libraryId;
              _serverPathToScriptRef[DartUri(scriptUri, _root).serverPath] =
                  scriptRef;
              libraryIdToScriptRefs.add(scriptRef);
            }
          }
        }
      }
      return _scriptRefsById.values.toList();
    });
  }

  /// Look up the script by id in an isolate.
  @override
  ScriptRef? scriptWithId(String? scriptId) =>
      scriptId == null ? null : _scriptRefsById[scriptId];

  /// Runs an eval on the page to compute all existing registered extensions.
  Future<List<String>> _getExtensionRpcs() async {
    final expression =
        "${globalLoadStrategy.loadModuleSnippet}('dart_sdk').developer._extensions.keys.toList();";
    final extensionRpcs = <String>[];
    final params = {
      'expression': expression,
      'returnByValue': true,
      'contextId': await contextId,
    };
    try {
      final response =
          await remoteDebugger.sendCommand('Runtime.evaluate', params: params);
      final result = getResultOrHandleError(response, evalContents: expression);
      extensionRpcs.addAll(List.from(result['value'] as List? ?? []));
    } catch (e, s) {
      _logger.severe(
          'Error calling Runtime.evaluate with params $params', e, s);
    }
    return extensionRpcs;
  }

  /// Convert a JS exception description into a description containing
  /// a Dart stack trace.
  @override
  Future<String> mapExceptionStackTrace(String description) async {
    RemoteObject mapperResult;
    try {
      mapperResult = await _jsCallFunction(
          stackTraceMapperExpression, <Object>[description]);
    } catch (_) {
      return description;
    }
    final mappedStack = mapperResult.value?.toString();
    if (mappedStack == null || mappedStack.isEmpty) {
      return description;
    }
    var message = exceptionMessageRegex.firstMatch(description)?.group(0);
    message = (message != null) ? '$message\n' : '';
    return '$message$mappedStack';
  }
}
