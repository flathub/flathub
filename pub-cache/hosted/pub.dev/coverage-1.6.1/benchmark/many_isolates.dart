// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:isolate';
import 'dart:math';

Future<void> main(List<String> args, dynamic message) async {
  if (message == null) {
    // If there is no message, it means this instance was created by
    // run_benchmarks.dart. In that case, this is the parent instance that
    // spawns all the others.
    int sum = 0;
    for (int i = 0; i < 10; ++i) {
      final port = ReceivePort();
      final isolate =
          Isolate.spawnUri(Uri.file('many_isolates.dart'), [], port.sendPort);
      sum += await port.first as int;
      await isolate;
    }
    print('sum = $sum');
  } else {
    // If there is a message, it means this instance is one of the child
    // instances. The message is the port that this instance replies on.
    (message as SendPort).send(Random().nextInt(1000));
  }
}
