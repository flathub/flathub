// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@JS()
library background;

import 'package:js/js.dart';

import 'chrome_api.dart';
import 'messaging.dart';

void main() {
  _registerListeners();
}

void _registerListeners() {
  chrome.runtime.onMessage.addListener(allowInterop(_handleRuntimeMessages));
}

void _handleRuntimeMessages(
    dynamic jsRequest, MessageSender sender, Function sendResponse) async {
  if (jsRequest is! String) return;

  interceptMessage(
      message: jsRequest,
      expectedSender: Script.detector,
      expectedRecipient: Script.background,
      expectedType: MessageType.dartAppReady,
      messageHandler: (_) {
        // Update the icon to show that a Dart app has been detected:
        chrome.action.setIcon(IconInfo(path: 'dart.png'), /*callback*/ null);
      });
}
