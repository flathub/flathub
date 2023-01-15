// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

/// Utilities to encode and decode VLQ values used in source maps.
///
/// Sourcemaps are encoded with variable length numbers as base64 encoded
/// strings with the least significant digit coming first. Each base64 digit
/// encodes a 5-bit value (0-31) and a continuation bit. Signed values can be
/// represented by using the least significant bit of the value as the sign bit.
///
/// For more details see the source map [version 3 documentation](https://docs.google.com/document/d/1U1RGAehQwRypUTovF1KRlpiOFze0b-_2gc6fAH0KY0k/edit?usp=sharing).
library source_maps.src.vlq;

import 'dart:math';

const int vlqBaseShift = 5;

const int vlqBaseMask = (1 << 5) - 1;

const int vlqContinuationBit = 1 << 5;

const int vlqContinuationMask = 1 << 5;

const String base64Digits =
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

final Map<String, int> _digits = () {
  var map = <String, int>{};
  for (var i = 0; i < 64; i++) {
    map[base64Digits[i]] = i;
  }
  return map;
}();

final int maxInt32 = (pow(2, 31) as int) - 1;
final int minInt32 = -(pow(2, 31) as int);

/// Creates the VLQ encoding of [value] as a sequence of characters
Iterable<String> encodeVlq(int value) {
  if (value < minInt32 || value > maxInt32) {
    throw ArgumentError('expected 32 bit int, got: $value');
  }
  var res = <String>[];
  var signBit = 0;
  if (value < 0) {
    signBit = 1;
    value = -value;
  }
  value = (value << 1) | signBit;
  do {
    var digit = value & vlqBaseMask;
    value >>= vlqBaseShift;
    if (value > 0) {
      digit |= vlqContinuationBit;
    }
    res.add(base64Digits[digit]);
  } while (value > 0);
  return res;
}

/// Decodes a value written as a sequence of VLQ characters. The first input
/// character will be `chars.current` after calling `chars.moveNext` once. The
/// iterator is advanced until a stop character is found (a character without
/// the [vlqContinuationBit]).
int decodeVlq(Iterator<String> chars) {
  var result = 0;
  var stop = false;
  var shift = 0;
  while (!stop) {
    if (!chars.moveNext()) throw StateError('incomplete VLQ value');
    var char = chars.current;
    var digit = _digits[char];
    if (digit == null) {
      throw FormatException('invalid character in VLQ encoding: $char');
    }
    stop = (digit & vlqContinuationBit) == 0;
    digit &= vlqBaseMask;
    result += (digit << shift);
    shift += vlqBaseShift;
  }

  // Result uses the least significant bit as a sign bit. We convert it into a
  // two-complement value. For example,
  //   2 (10 binary) becomes 1
  //   3 (11 binary) becomes -1
  //   4 (100 binary) becomes 2
  //   5 (101 binary) becomes -2
  //   6 (110 binary) becomes 3
  //   7 (111 binary) becomes -3
  var negate = (result & 1) == 1;
  result = result >> 1;
  result = negate ? -result : result;

  // TODO(sigmund): can we detect this earlier?
  if (result < minInt32 || result > maxInt32) {
    throw FormatException(
        'expected an encoded 32 bit int, but we got: $result');
  }
  return result;
}
