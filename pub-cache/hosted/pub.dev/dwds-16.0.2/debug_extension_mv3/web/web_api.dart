// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'package:js/js.dart';

@JS()
external Console get console;

@JS()
@anonymous
class Console {
  external void log(String header,
      [String style1, String style2, String style3]);

  external void warn(String header,
      [String style1, String style2, String style3]);
}
