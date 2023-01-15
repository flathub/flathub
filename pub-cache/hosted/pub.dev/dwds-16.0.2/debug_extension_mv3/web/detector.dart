// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@JS()
library detector;

import 'dart:html';
import 'package:js/js.dart';

import 'chrome_api.dart';
import 'messaging.dart';

void main() {
  _registerListeners();
}

void _registerListeners() {
  document.addEventListener('dart-app-ready', _onDartAppReadyEvent);
}

void _onDartAppReadyEvent(Event event) {
  _sendMessageToBackgroundScript(
    type: MessageType.dartAppReady,
    body: 'Dart app ready!',
  );
}

void _sendMessageToBackgroundScript({
  required MessageType type,
  required String body,
}) {
  final message = Message(
    to: Script.background,
    from: Script.detector,
    type: type,
    body: body,
  );
  chrome.runtime.sendMessage(
    /*id*/ null,
    message.toJSON(),
    /*options*/ null,
    /*callback*/ null,
  );
}
