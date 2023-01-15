// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:js_util' as js_util;

import 'package:js/js.dart';

@JS()
@staticInterop
class Window extends EventTarget {}

extension WindowExtension on Window {
  external Location get location;
  CSSStyleDeclaration? getComputedStyle(Element elt, [String? pseudoElt]) =>
      js_util.callMethod(this, 'getComputedStyle', <Object>[
        elt,
        if (pseudoElt != null) pseudoElt
      ]) as CSSStyleDeclaration?;
  external Navigator get navigator;
  void postMessage(Object message, String targetOrigin,
          [List<MessagePort>? messagePorts]) =>
      js_util.callMethod(this, 'postMessage', <Object?>[
        js_util.jsify(message),
        targetOrigin,
        if (messagePorts != null) js_util.jsify(messagePorts)
      ]);
}

@JS('window')
external Window get window;

@JS()
@staticInterop
class Document extends Node {}

extension DocumentExtension on Document {
  external Element? querySelector(String selectors);
  Element createElement(String name, [Object? options]) => js_util.callMethod(
          this, 'createElement', <Object>[name, if (options != null) options])
      as Element;
}

@JS()
@staticInterop
class HTMLDocument extends Document {}

extension HTMLDocumentExtension on HTMLDocument {
  external HTMLBodyElement? get body;
  external String? get title;
}

@JS('document')
external HTMLDocument get document;

@JS()
@staticInterop
class Navigator {}

extension NavigatorExtension on Navigator {
  external String get userAgent;
}

@JS()
@staticInterop
class Element extends Node {}

extension DomElementExtension on Element {
  external DomTokenList get classList;
}

@JS()
@staticInterop
class HTMLElement extends Element {}

@JS()
@staticInterop
class HTMLBodyElement extends HTMLElement {}

@JS()
@staticInterop
class Node extends EventTarget {}

extension NodeExtension on Node {
  external Node appendChild(Node node);
  void remove() {
    if (parentNode != null) {
      final Node parent = parentNode!;
      parent.removeChild(this);
    }
  }

  external Node removeChild(Node child);
  external Node? get parentNode;
}

@JS()
@staticInterop
class EventTarget {}

extension EventTargetExtension on EventTarget {
  void addEventListener(String type, EventListener? listener,
      [bool? useCapture]) {
    if (listener != null) {
      js_util.callMethod(this, 'addEventListener',
          <Object>[type, listener, if (useCapture != null) useCapture]);
    }
  }

  void removeEventListener(String type, EventListener? listener,
      [bool? useCapture]) {
    if (listener != null) {
      js_util.callMethod(this, 'removeEventListener',
          <Object>[type, listener, if (useCapture != null) useCapture]);
    }
  }
}

typedef EventListener = void Function(Event event);

@JS()
@staticInterop
class Event {}

extension EventExtension on Event {
  external void stopPropagation();
}

@JS()
@staticInterop
class MessageEvent extends Event {}

extension MessageEventExtension on MessageEvent {
  dynamic get data => js_util.dartify(js_util.getProperty(this, 'data'));
  external String get origin;
  List<MessagePort> get ports =>
      js_util.getProperty<List>(this, 'ports').cast<MessagePort>();
}

@JS()
@staticInterop
class Location {}

extension LocationExtension on Location {
  external String get href;
  external String get origin;
}

@JS()
@staticInterop
class MessagePort extends EventTarget {}

extension MessagePortExtension on MessagePort {
  void postMessage(Object? message) => js_util.callMethod(this, 'postMessage',
      <Object>[if (message != null) js_util.jsify(message) as Object]);
  external void start();
}

@JS()
@staticInterop
class CSSStyleDeclaration {}

@JS()
@staticInterop
class HTMLScriptElement extends HTMLElement {}

extension HTMLScriptElementExtension on HTMLScriptElement {
  external set src(String value);
}

HTMLScriptElement createHTMLScriptElement() =>
    document.createElement('script') as HTMLScriptElement;

@JS()
@staticInterop
class DomTokenList {}

extension DomTokenListExtension on DomTokenList {
  external void add(String value);
  external void remove(String value);
  external bool contains(String token);
}

@JS()
@staticInterop
class HTMLIFrameElement extends HTMLElement {}

extension HTMLIFrameElementExtension on HTMLIFrameElement {
  external String? get src;
  external set src(String? value);
  external Window get contentWindow;
}

HTMLIFrameElement createHTMLIFrameElement() =>
    document.createElement('iframe') as HTMLIFrameElement;

@JS()
@staticInterop
class WebSocket extends EventTarget {}

extension WebSocketExtension on WebSocket {
  external void send(Object? data);
}

WebSocket createWebSocket(String url) =>
    _callConstructor('WebSocket', <Object>[url])! as WebSocket;

@JS()
@staticInterop
class MessageChannel {}

extension MessageChannelExtension on MessageChannel {
  external MessagePort get port1;
  external MessagePort get port2;
}

MessageChannel createMessageChannel() =>
    _callConstructor('MessageChannel', <Object>[])! as MessageChannel;

Object? _findConstructor(String constructorName) =>
    js_util.getProperty(window, constructorName);

Object? _callConstructor(String constructorName, List<Object?> args) {
  final Object? constructor = _findConstructor(constructorName);
  if (constructor == null) {
    return null;
  }
  return js_util.callConstructor(constructor, args);
}

class Subscription {
  final String type;
  final EventTarget target;
  final EventListener listener;

  Subscription(this.target, this.type, this.listener) {
    target.addEventListener(type, listener);
  }

  void cancel() => target.removeEventListener(type, listener);
}
