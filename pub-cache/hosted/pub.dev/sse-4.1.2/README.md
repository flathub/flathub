[![Dart CI](https://github.com/dart-lang/sse/actions/workflows/test-package.yml/badge.svg)](https://github.com/dart-lang/sse/actions/workflows/test-package.yml)
[![pub package](https://img.shields.io/pub/v/sse.svg)](https://pub.dev/packages/sse)
[![package publisher](https://img.shields.io/pub/publisher/sse.svg)](https://pub.dev/packages/sse/publisher)

This package provides support for bi-directional communication through Server
Sent Events and corresponding POST requests.

This package is not intended to be a general purpose SSE package, but instead is
a bidirectional protocol for use when Websockets are unavailable. That is, both
the client and the server expose a `sink` and `stream` on which to send and
receive messages respectively.

Both the server and client have implicit assumptions on each other and therefore
a client from this package must be paired with a server from this package.
