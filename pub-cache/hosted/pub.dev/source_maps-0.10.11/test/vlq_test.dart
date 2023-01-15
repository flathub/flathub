// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

library test.vlq_test;

import 'dart:math';
import 'package:test/test.dart';
import 'package:source_maps/src/vlq.dart';

void main() {
  test('encode and decode - simple values', () {
    expect(encodeVlq(1).join(''), 'C');
    expect(encodeVlq(2).join(''), 'E');
    expect(encodeVlq(3).join(''), 'G');
    expect(encodeVlq(100).join(''), 'oG');
    expect(decodeVlq('C'.split('').iterator), 1);
    expect(decodeVlq('E'.split('').iterator), 2);
    expect(decodeVlq('G'.split('').iterator), 3);
    expect(decodeVlq('oG'.split('').iterator), 100);
  });

  test('encode and decode', () {
    for (var i = -10000; i < 10000; i++) {
      _checkEncodeDecode(i);
    }
  });

  test('only 32-bit ints allowed', () {
    var maxInt = (pow(2, 31) as int) - 1;
    var minInt = -(pow(2, 31) as int);
    _checkEncodeDecode(maxInt - 1);
    _checkEncodeDecode(minInt + 1);
    _checkEncodeDecode(maxInt);
    _checkEncodeDecode(minInt);

    expect(encodeVlq(minInt).join(''), 'hgggggE');
    expect(decodeVlq('hgggggE'.split('').iterator), minInt);

    expect(() => encodeVlq(maxInt + 1), throwsA(anything));
    expect(() => encodeVlq(maxInt + 2), throwsA(anything));
    expect(() => encodeVlq(minInt - 1), throwsA(anything));
    expect(() => encodeVlq(minInt - 2), throwsA(anything));

    // if we allowed more than 32 bits, these would be the expected encodings
    // for the large numbers above.
    expect(() => decodeVlq('ggggggE'.split('').iterator), throwsA(anything));
    expect(() => decodeVlq('igggggE'.split('').iterator), throwsA(anything));
    expect(() => decodeVlq('jgggggE'.split('').iterator), throwsA(anything));
    expect(() => decodeVlq('lgggggE'.split('').iterator), throwsA(anything));
  },
      // This test uses integers so large they overflow in JS.
      testOn: 'dart-vm');
}

void _checkEncodeDecode(int value) {
  var encoded = encodeVlq(value);
  expect(decodeVlq(encoded.iterator), value);
  expect(decodeVlq(encoded.join('').split('').iterator), value);
}
