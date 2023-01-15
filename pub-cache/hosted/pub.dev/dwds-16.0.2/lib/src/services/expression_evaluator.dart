// Copyright (c) 2020, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';

import 'package:dwds/src/utilities/domain.dart';
import 'package:logging/logging.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import '../debugging/dart_scope.dart';
import '../debugging/debugger.dart';
import '../debugging/location.dart';
import '../debugging/modules.dart';
import '../loaders/strategy.dart';
import '../utilities/objects.dart' as chrome;
import 'expression_compiler.dart';

class ErrorKind {
  const ErrorKind._(this._kind);

  final String _kind;
  static const ErrorKind compilation = ErrorKind._('CompilationError');
  static const ErrorKind type = ErrorKind._('TypeError');
  static const ErrorKind reference = ErrorKind._('ReferenceError');
  static const ErrorKind internal = ErrorKind._('InternalError');
  static const ErrorKind invalidInput = ErrorKind._('InvalidInputError');
  static const ErrorKind loadModule = ErrorKind._('LoadModuleError');

  @override
  String toString() => _kind;
}

/// ExpressionEvaluator provides functionality to evaluate dart expressions
/// from text user input in the debugger, using chrome remote debugger to
/// collect context for evaluation (scope, types, modules), and using
/// ExpressionCompilerInterface to compile dart expressions to JavaScript.
class ExpressionEvaluator {
  final String _entrypoint;
  final AppInspectorInterface _inspector;
  final Debugger _debugger;
  final Locations _locations;
  final Modules _modules;
  final ExpressionCompiler _compiler;
  final _logger = Logger('ExpressionEvaluator');

  /// Strip synthetic library name from compiler error messages.
  static final _syntheticNameFilterRegex =
      RegExp('org-dartlang-debug:synthetic_debug_expression:.*:.*Error: ');

  /// Find module path from the XHR call network error message received from chrome.
  ///
  /// Example:
  /// NetworkError: Failed to load 'http://<hostname>.com/path/to/module.js?<cache_busting_token>'
  static final _loadModuleErrorRegex =
      RegExp(r".*Failed to load '.*\.com/(.*\.js).*");

  ExpressionEvaluator(this._entrypoint, this._inspector, this._debugger,
      this._locations, this._modules, this._compiler);

  RemoteObject _createError(ErrorKind severity, String message) {
    return RemoteObject(
        <String, String>{'type': '$severity', 'value': message});
  }

  void close() {}

  /// Evaluate dart expression inside a given library.
  ///
  /// Uses ExpressionCompiler interface to compile the expression to
  /// JavaScript and sends evaluate requests to chrome to calculate
  /// the final result.
  ///
  /// Returns remote object containing the result of evaluation or error.
  ///
  /// [isolateId] current isolate ID.
  /// [libraryUri] dart library to evaluate the expression in.
  /// [expression] dart expression to evaluate.
  Future<RemoteObject> evaluateExpression(
    String isolateId,
    String? libraryUri,
    String expression,
    Map<String, String>? scope,
  ) async {
    scope ??= {};

    if (expression.isEmpty) {
      return _createError(ErrorKind.invalidInput, expression);
    }

    if (libraryUri == null) {
      return _createError(ErrorKind.invalidInput, 'no library uri');
    }

    final module = await _modules.moduleForLibrary(libraryUri);
    if (module == null) {
      return _createError(ErrorKind.internal, 'no module for $libraryUri');
    }

    // Wrap the expression in a lambda so we can call it as a function.
    expression = _createDartLambda(expression, scope.keys);
    _logger.finest('Evaluating "$expression" at $module');

    // Compile expression using an expression compiler, such as
    // frontend server or expression compiler worker.
    final compilationResult = await _compiler.compileExpressionToJs(
        isolateId, libraryUri.toString(), 0, 0, {}, {}, module, expression);

    final isError = compilationResult.isError;
    final jsResult = compilationResult.result;
    if (isError) {
      return _formatCompilationError(jsResult);
    }

    // Strip try/catch incorrectly added by the expression compiler.
    var jsCode = _maybeStripTryCatch(jsResult);

    // Send JS expression to chrome to evaluate.
    jsCode = _createJsLambdaWithTryCatch(jsCode, scope.keys);
    var result = await _inspector.callFunction(jsCode, scope.values);
    result = await _formatEvaluationError(result);

    _logger.finest('Evaluated "$expression" to "$result"');
    return result;
  }

  /// Evaluate dart expression inside a given frame (function).
  ///
  /// Gets necessary context (types, scope, module names) data from chrome,
  /// uses ExpressionCompiler interface to compile the expression to
  /// JavaScript, and sends evaluate requests to chrome to calculate the
  /// final result.
  ///
  /// Returns remote object containing the result of evaluation or error.
  ///
  /// [isolateId] current isolate ID.
  /// [frameIndex] JavaScript frame to evaluate the expression in.
  /// [expression] dart expression to evaluate.
  Future<RemoteObject> evaluateExpressionInFrame(String isolateId,
      int frameIndex, String expression, Map<String, String>? scope) async {
    if (scope != null) {
      // TODO(annagrin): Implement scope support.
      // Issue: https://github.com/dart-lang/webdev/issues/1344
      return _createError(
          ErrorKind.internal,
          'Using scope for expression evaluation in frame '
          'is not supported.');
    }

    if (expression.isEmpty) {
      return _createError(ErrorKind.invalidInput, expression);
    }

    // Get JS scope and current JS location.
    final jsFrame = _debugger.jsFrameForIndex(frameIndex);
    if (jsFrame == null) {
      return _createError(
          ErrorKind.internal,
          'Expression evaluation in async frames '
          'is not supported. No frame with index $frameIndex.');
    }

    final functionName = jsFrame.functionName;
    final jsLine = jsFrame.location.lineNumber;
    final jsScriptId = jsFrame.location.scriptId;
    final jsColumn = jsFrame.location.columnNumber;
    final jsScope = await _collectLocalJsScope(jsFrame);

    // Find corresponding dart location and scope.
    final url = _debugger.urlForScriptId(jsScriptId);
    if (url == null) {
      return _createError(
          ErrorKind.internal, 'Cannot find url for JS script: $jsScriptId');
    }
    final locationMap = await _locations.locationForJs(url, jsLine, jsColumn);
    if (locationMap == null) {
      return _createError(
          ErrorKind.internal,
          'Cannot find Dart location for JS location: '
          'url: $url, '
          'function: $functionName, '
          'line: $jsLine, '
          'column: $jsColumn');
    }

    final dartLocation = locationMap.dartLocation;
    final dartSourcePath = dartLocation.uri.serverPath;
    final libraryUri = await _modules.libraryForSource(dartSourcePath);
    if (libraryUri == null) {
      return _createError(
          ErrorKind.internal, 'no libraryUri for $dartSourcePath');
    }

    final module = await _modules.moduleForLibrary(libraryUri.toString());
    if (module == null) {
      return _createError(
          ErrorKind.internal, 'no module for $libraryUri ($dartSourcePath)');
    }

    _logger.finest('Evaluating "$expression" at $module, '
        '$libraryUri:${dartLocation.line}:${dartLocation.column}');

    // Compile expression using an expression compiler, such as
    // frontend server or expression compiler worker.
    final compilationResult = await _compiler.compileExpressionToJs(
        isolateId,
        libraryUri.toString(),
        dartLocation.line,
        dartLocation.column,
        {},
        jsScope,
        module,
        expression);

    final isError = compilationResult.isError;
    final jsResult = compilationResult.result;
    if (isError) {
      return _formatCompilationError(jsResult);
    }

    // Strip try/catch incorrectly added by the expression compiler.
    var jsCode = _maybeStripTryCatch(jsResult);

    // Send JS expression to chrome to evaluate.
    jsCode = _createTryCatch(jsCode);

    // Send JS expression to chrome to evaluate.
    var result = await _debugger.evaluateJsOnCallFrameIndex(frameIndex, jsCode);
    result = await _formatEvaluationError(result);

    _logger.finest('Evaluated "$expression" to "$result"');
    return result;
  }

  RemoteObject _formatCompilationError(String error) {
    // Frontend currently gives a text message including library name
    // and function name on compilation error. Strip this information
    // since it shows synthetic names that are only used for temporary
    // debug library during expression evaluation.
    //
    // TODO(annagrin): modify frontend to avoid stripping dummy names
    // [issue 40449](https://github.com/dart-lang/sdk/issues/40449)
    if (error.startsWith('[')) {
      error = error.substring(1);
    }
    if (error.endsWith(']')) {
      error = error.substring(0, error.lastIndexOf(']'));
    }
    if (error.contains('InternalError: ')) {
      error = error.replaceAll('InternalError: ', '');
      return _createError(ErrorKind.internal, error);
    }
    error = error.replaceAll(_syntheticNameFilterRegex, '');
    return _createError(ErrorKind.compilation, error);
  }

  Future<RemoteObject> _formatEvaluationError(RemoteObject result) async {
    if (result.type == 'string') {
      var error = '${result.value}';
      if (error.startsWith('ReferenceError: ')) {
        error = error.replaceFirst('ReferenceError: ', '');
        return _createError(ErrorKind.reference, error);
      } else if (error.startsWith('TypeError: ')) {
        error = error.replaceFirst('TypeError: ', '');
        return _createError(ErrorKind.type, error);
      } else if (error.startsWith('NetworkError: ')) {
        var modulePath = _loadModuleErrorRegex.firstMatch(error)?.group(1);
        final module = modulePath != null
            ? await globalLoadStrategy.moduleForServerPath(
                _entrypoint, modulePath)
            : 'unknown';
        modulePath ??= 'unknown';
        error = 'Module is not loaded : $module (path: $modulePath). '
            'Accessing libraries that have not yet been used in the '
            'application is not supported during expression evaluation.';
        return _createError(ErrorKind.loadModule, error);
      }
    }
    return result;
  }

  Future<Map<String, String>> _collectLocalJsScope(WipCallFrame frame) async {
    final jsScope = <String, String>{};

    void collectVariables(
        String scopeType, Iterable<chrome.Property> variables) {
      for (var p in variables) {
        final name = p.name;
        final value = p.value;
        // TODO: null values represent variables optimized by v8.
        // Show that to the user.
        if (name != null && value != null && !_isUndefined(value)) {
          jsScope[name] = name;
        }
      }
    }

    // skip library and main scope
    final scopeChain = filterScopes(frame).reversed;
    for (var scope in scopeChain) {
      final objectId = scope.object.objectId;
      if (objectId != null) {
        final scopeProperties = await _debugger.getProperties(objectId);
        collectVariables(scope.scope, scopeProperties);
      }
    }

    return jsScope;
  }

  bool _isUndefined(RemoteObject value) => value.type == 'undefined';

  /// Strip try/catch incorrectly added by the expression compiler.
  /// TODO: remove adding try/catch block in expression compiler.
  /// https://github.com/dart-lang/webdev/issues/1341, then remove
  /// this stripping code.
  String _maybeStripTryCatch(String jsCode) {
    // Match the wrapping generated by the expression compiler exactly
    // so the matching does not succeed naturally after the wrapping is
    // removed:
    //
    // Expression compiler's wrapping:
    //
    // '\ntry {'
    // '\n  ($jsExpression('
    // '\n    $args'
    // '\n  ))'
    // '\n} catch (error) {'
    // '\n  error.name + ": " + error.message;'
    // '\n}';
    //
    final lines = jsCode.split('\n');
    if (lines.length > 5) {
      final tryLines = lines.getRange(0, 2).toList();
      final bodyLines = lines.getRange(2, lines.length - 3);
      final catchLines =
          lines.getRange(lines.length - 3, lines.length).toList();
      if (tryLines[0].isEmpty &&
          tryLines[1] == 'try {' &&
          catchLines[0] == '} catch (error) {' &&
          catchLines[1] == '  error.name + ": " + error.message;' &&
          catchLines[2] == '}') {
        return bodyLines.join('\n');
      }
    }
    return jsCode;
  }

  String _createJsLambdaWithTryCatch(
      String expression, Iterable<String> params) {
    final args = params.join(', ');
    return '  '
        '  function($args) {\n'
        '    try {\n'
        '      return $expression($args);\n'
        '    } catch (error) {\n'
        '      return error.name + ": " + error.message;\n'
        '    }\n'
        '} ';
  }

  String _createTryCatch(String expression) => '  '
      '  try {\n'
      '    $expression;\n'
      '  } catch (error) {\n'
      '    error.name + ": " + error.message;\n'
      '  }\n';

  String _createDartLambda(String expression, Iterable<String> params) =>
      '(${params.join(', ')}) => $expression';
}
