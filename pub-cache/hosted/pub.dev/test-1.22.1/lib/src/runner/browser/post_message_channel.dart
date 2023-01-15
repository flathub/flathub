// Copyright (c) 2015, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

library test.src.runner.browser.post_message_channel;

import 'dart:js_util';

import 'package:js/js.dart';
import 'package:stream_channel/stream_channel.dart';

import 'dom.dart' as dom;

// Avoid using this from dart:html to work around dart-lang/sdk#32113.
@JS('window.parent.postMessage')
external void _postParentMessage(Object message, String targetOrigin);

/// Constructs a [StreamChannel] wrapping [MessageChannel] communication with
/// the host page.
StreamChannel<Object?> postMessageChannel() {
  var controller = StreamChannelController<Object?>(sync: true);

  // Listen for a message from the host that transfers a message port, then
  // cancel the subscription.  This is important to prevent multiple
  // subscriptions if the test is ever hot restarted.
  late final dom.Subscription subscription;
  subscription =
      dom.Subscription(dom.window, 'message', allowInterop((dom.Event event) {
    // A message on the Window can theoretically come from any website. It's
    // very unlikely that a malicious site would care about hacking someone's
    // unit tests, let alone be able to find the test server while it's
    // running, but it's good practice to check the origin anyway.
    final message = event as dom.MessageEvent;
    if (message.origin == dom.window.location.origin &&
        message.data == 'port') {
      subscription.cancel();
      var port = message.ports.first;
      port.start();
      var portSubscription =
          dom.Subscription(port, 'message', allowInterop((dom.Event event) {
        controller.local.sink.add((event as dom.MessageEvent).data);
      }));

      controller.local.stream.listen((data) {
        port.postMessage({'data': data});
      }, onDone: () {
        port.postMessage({'event': 'done'});
        portSubscription.cancel();
      });
    }
  }));

  // Send a ready message once we're listening so the host knows it's safe to
  // start sending events.
  // TODO(nweiz): Stop manually adding href here once issue 22554 is fixed.
  _postParentMessage(
      jsify({'href': dom.window.location.href, 'ready': true}) as Object,
      dom.window.location.origin);

  return controller.foreign;
}
