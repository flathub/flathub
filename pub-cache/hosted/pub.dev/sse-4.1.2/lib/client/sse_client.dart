// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:html';

import 'package:js/js.dart';
import 'package:logging/logging.dart';
import 'package:pool/pool.dart';
import 'package:stream_channel/stream_channel.dart';

import '../src/util/uuid.dart';

/// Limit for the number of concurrent outgoing requests.
///
/// Chrome drops outgoing requests on the floor after some threshold. To prevent
/// these errors we buffer outgoing requests with a pool.
///
/// Note Chrome's limit is 6000. So this gives us plenty of headroom.
final _requestPool = Pool(1000);

/// A client for bi-directional sse communcation.
///
/// The client can send any JSON-encodable messages to the server by adding
/// them to the [sink] and listen to messages from the server on the [stream].
class SseClient extends StreamChannelMixin<String?> {
  final String _clientId;

  final _incomingController = StreamController<String>();

  final _outgoingController = StreamController<String>();

  final _logger = Logger('SseClient');

  final _onConnected = Completer();

  int _lastMessageId = -1;

  late EventSource _eventSource;

  late String _serverUrl;

  Timer? _errorTimer;

  /// [serverUrl] is the URL under which the server is listening for
  /// incoming bi-directional SSE connections. [debugKey] is an optional key
  /// that can be used to identify the SSE connection.
  SseClient(String serverUrl, {String? debugKey})
      : _clientId = debugKey == null
            ? generateUuidV4()
            : '$debugKey-${generateUuidV4()}' {
    _serverUrl = '$serverUrl?sseClientId=$_clientId';
    _eventSource = EventSource(_serverUrl, withCredentials: true);
    _eventSource.onOpen.first.whenComplete(() {
      _onConnected.complete();
      _outgoingController.stream
          .listen(_onOutgoingMessage, onDone: _onOutgoingDone);
    });
    _eventSource.addEventListener('message', _onIncomingMessage);
    _eventSource.addEventListener('control', _onIncomingControlMessage);

    _eventSource.onOpen.listen((_) {
      _errorTimer?.cancel();
    });
    _eventSource.onError.listen((error) {
      if (!(_errorTimer?.isActive ?? false)) {
        // By default the SSE client uses keep-alive.
        // Allow for a retry to connect before giving up.
        _errorTimer = Timer(const Duration(seconds: 5), () {
          _closeWithError(error);
        });
      }
    });
  }

  @Deprecated('Use onConnected instead.')
  Stream<Event> get onOpen => _eventSource.onOpen;

  Future<void> get onConnected => _onConnected.future;

  /// Add messages to this [StreamSink] to send them to the server.
  ///
  /// The message added to the sink has to be JSON encodable. Messages that fail
  /// to encode will be logged through a [Logger].
  @override
  StreamSink<String> get sink => _outgoingController.sink;

  /// [Stream] of messages sent from the server to this client.
  ///
  /// A message is a decoded JSON object.
  @override
  Stream<String> get stream => _incomingController.stream;

  void close() {
    _eventSource.close();
    // If the initial connection was never established. Add a listener so close
    // adds a done event to [sink].
    if (!_onConnected.isCompleted) _outgoingController.stream.drain();
    _incomingController.close();
    _outgoingController.close();
  }

  void _closeWithError(Object error) {
    _incomingController.addError(error);
    close();
    if (!_onConnected.isCompleted) {
      // This call must happen after the call to close() which checks
      // whether the completer was completed earlier.
      _onConnected.completeError(error);
    }
  }

  void _onIncomingControlMessage(Event message) {
    var data = (message as MessageEvent).data;
    if (data == 'close') {
      close();
    } else {
      throw UnsupportedError('[$_clientId] Illegal Control Message "$data"');
    }
  }

  void _onIncomingMessage(Event message) {
    var decoded =
        jsonDecode((message as MessageEvent).data as String) as String;
    _incomingController.add(decoded);
  }

  void _onOutgoingDone() {
    close();
  }

  void _onOutgoingMessage(String? message) async {
    String? encodedMessage;
    await _requestPool.withResource(() async {
      try {
        encodedMessage = jsonEncode(message);
      } on JsonUnsupportedObjectError catch (e) {
        _logger.warning('[$_clientId] Unable to encode outgoing message: $e');
      } on ArgumentError catch (e) {
        _logger.warning('[$_clientId] Invalid argument: $e');
      }
      try {
        final url = '$_serverUrl&messageId=${++_lastMessageId}';
        await _fetch(
            url,
            _FetchOptions(
                method: 'POST', body: encodedMessage, credentials: 'include'));
      } catch (error) {
        final augmentedError =
            '[$_clientId] SSE client failed to send $message:\n $error';
        _logger.severe(augmentedError);
        _closeWithError(augmentedError);
      }
    });
  }
}

// Custom implementation of Fetch API until Dart supports GET vs. POST,
// credentials, etc. See https://github.com/dart-lang/http/issues/595.
@JS('fetch')
external Object _nativeJsFetch(String resourceUrl, _FetchOptions options);

Future<dynamic> _fetch(String resourceUrl, _FetchOptions options) =>
    promiseToFuture(_nativeJsFetch(resourceUrl, options));

@JS()
@anonymous
class _FetchOptions {
  external factory _FetchOptions({
    required String method, // e.g., 'GET', 'POST'
    required String credentials, // e.g., 'omit', 'same-origin', 'include'
    required String? body,
  });
}
