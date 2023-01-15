// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@JS()
library messaging;

import 'dart:convert';

import 'package:js/js.dart';

import 'web_api.dart';

enum Script {
  background,
  detector;

  factory Script.fromString(String value) {
    return Script.values.byName(value);
  }
}

enum MessageType {
  dartAppReady;

  factory MessageType.fromString(String value) {
    return MessageType.values.byName(value);
  }
}

class Message {
  final Script to;
  final Script from;
  final MessageType type;
  final String body;
  final String? error;

  Message({
    required this.to,
    required this.from,
    required this.type,
    required this.body,
    this.error,
  });

  factory Message.fromJSON(String json) {
    final decoded = jsonDecode(json) as Map<String, dynamic>;

    return Message(
      to: Script.fromString(decoded['to'] as String),
      from: Script.fromString(decoded['from'] as String),
      type: MessageType.fromString(decoded['type'] as String),
      body: decoded['body'] as String,
      error: decoded['error'] as String?,
    );
  }

  String toJSON() {
    return jsonEncode({
      'type': type.name,
      'to': to.name,
      'from': from.name,
      'encodedBody': body,
      if (error != null) 'error': error,
    });
  }
}

void interceptMessage({
  required String? message,
  required MessageType expectedType,
  required Script expectedSender,
  required Script expectedRecipient,
  required void Function(String message) messageHandler,
}) {
  try {
    if (message == null) return;
    final decodedMessage = Message.fromJSON(message);
    if (decodedMessage.type != expectedType ||
        decodedMessage.to != expectedRecipient ||
        decodedMessage.from != expectedSender) {
      return;
    }
    messageHandler(decodedMessage.body);
  } catch (error) {
    console.warn(
        'Error intercepting $expectedType message from $expectedSender to $expectedRecipient: $error');
    return;
  }
}
