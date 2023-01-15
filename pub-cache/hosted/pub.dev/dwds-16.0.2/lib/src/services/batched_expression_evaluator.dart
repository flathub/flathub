// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';

import 'package:collection/collection.dart';
import 'package:dwds/src/utilities/domain.dart';
import 'package:logging/logging.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import '../debugging/debugger.dart';
import '../debugging/location.dart';
import '../debugging/modules.dart';
import '../utilities/batched_stream.dart';
import 'expression_compiler.dart';
import 'expression_evaluator.dart';

class EvaluateRequest {
  final String isolateId;
  final String? libraryUri;
  final String expression;
  final Map<String, String>? scope;
  final completer = Completer<RemoteObject>();

  EvaluateRequest(this.isolateId, this.libraryUri, this.expression, this.scope);
}

class BatchedExpressionEvaluator extends ExpressionEvaluator {
  final _logger = Logger('BatchedExpressionEvaluator');
  final Debugger _debugger;
  final _requestController =
      BatchedStreamController<EvaluateRequest>(delay: 200);

  BatchedExpressionEvaluator(
    String entrypoint,
    AppInspectorInterface inspector,
    this._debugger,
    Locations locations,
    Modules modules,
    ExpressionCompiler compiler,
  ) : super(entrypoint, inspector, _debugger, locations, modules, compiler) {
    _requestController.stream.listen(_processRequest);
  }

  @override
  void close() {
    _logger.fine('Closed');
    _requestController.close();
  }

  @override
  Future<RemoteObject> evaluateExpression(
    String isolateId,
    String? libraryUri,
    String expression,
    Map<String, String>? scope,
  ) {
    final request = EvaluateRequest(isolateId, libraryUri, expression, scope);
    _requestController.sink.add(request);
    return request.completer.future;
  }

  void _processRequest(List<EvaluateRequest> requests) async {
    String? libraryUri;
    String? isolateId;
    Map<String, String>? scope;
    List<EvaluateRequest> currentRequests = [];

    for (var request in requests) {
      libraryUri ??= request.libraryUri;
      isolateId ??= request.isolateId;
      scope ??= request.scope;

      if (libraryUri != request.libraryUri ||
          isolateId != request.isolateId ||
          !MapEquality().equals(scope, request.scope)) {
        _logger.fine('New batch due to');
        if (libraryUri != request.libraryUri) {
          _logger.fine(' - library uri: $libraryUri != ${request.libraryUri}');
        }
        if (isolateId != request.isolateId) {
          _logger.fine(' - isolateId: $isolateId != ${request.isolateId}');
        }
        if (!MapEquality().equals(scope, request.scope)) {
          _logger.fine(' - scope: $scope != ${request.scope}');
        }

        unawaited(_evaluateBatch(currentRequests));
        currentRequests = [];
        libraryUri = request.libraryUri;
        isolateId = request.isolateId;
        scope = request.scope;
      }
      currentRequests.add(request);
    }
    unawaited(_evaluateBatch(currentRequests));
  }

  Future<void> _evaluateBatch(List<EvaluateRequest> requests) async {
    if (requests.isEmpty) return;

    final first = requests.first;
    if (requests.length == 1) {
      if (first.completer.isCompleted) return;
      return super
          .evaluateExpression(
              first.isolateId, first.libraryUri, first.expression, first.scope)
          .then(requests.first.completer.complete);
    }

    final expressions = requests.map((r) => r.expression).join(', ');
    final batchedExpression = '[ $expressions ]';

    _logger.fine('Evaluating batch of expressions $batchedExpression');

    final RemoteObject list = await super.evaluateExpression(
        first.isolateId, first.libraryUri, batchedExpression, first.scope);

    for (var i = 0; i < requests.length; i++) {
      final request = requests[i];
      if (request.completer.isCompleted) continue;
      _logger.fine('Getting result out of a batch for ${request.expression}');
      _debugger
          .getProperties(list.objectId!,
              offset: i, count: 1, length: requests.length)
          .then((v) {
        final result = v.first.value;
        _logger.fine(
            'Got result out of a batch for ${request.expression}: $result');
        request.completer.complete(result);
      });
    }
  }
}
