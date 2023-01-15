// Copyright (c) 2020, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'package:dwds/src/loaders/strategy.dart';
import 'package:dwds/src/debugging/location.dart';
import 'package:dwds/src/debugging/skip_list.dart';
import 'package:dwds/src/utilities/dart_uri.dart';
import 'package:source_maps/parser.dart';
import 'package:test/test.dart';

import 'fixtures/fakes.dart';

class TestStrategy extends FakeStrategy {
  @override
  String serverPathForAppUri(String appUri) {
    return 'foo';
  }
}

void main() {
  globalLoadStrategy = TestStrategy();

  late SkipLists skipLists;
  final dartUri = DartUri('org-dartlang-app://web/main.dart');
  group('SkipLists', () {
    setUp(() {
      skipLists = SkipLists();
    });

    test('do not include known ranges', () async {
      final skipList = await skipLists.compute('123', {
        Location.from(
            'foo', TargetLineEntry(1, []), TargetEntry(2, 0, 0, 0), dartUri),
        Location.from(
            'foo', TargetLineEntry(10, []), TargetEntry(20, 0, 0, 0), dartUri),
      });
      expect(skipList.length, 3);
      _validateRange(skipList.first, 0, 0, 1, 1);
      _validateRange(skipList[1], 1, 3, 10, 19);
      _validateRange(skipList.last, 10, 21, maxValue, maxValue);
    });

    test('do not include start of the file', () async {
      final skipList = await skipLists.compute('123', {
        Location.from(
            'foo', TargetLineEntry(0, []), TargetEntry(0, 0, 0, 0), dartUri),
        Location.from(
            'foo', TargetLineEntry(10, []), TargetEntry(20, 0, 0, 0), dartUri),
      });
      expect(skipList.length, 2);
      _validateRange(skipList[0], 0, 1, 10, 19);
      _validateRange(skipList.last, 10, 21, maxValue, maxValue);
    });

    test('does not depend on order of locations', () async {
      final skipList = await skipLists.compute('123', {
        Location.from(
            'foo', TargetLineEntry(10, []), TargetEntry(20, 0, 0, 0), dartUri),
        Location.from(
            'foo', TargetLineEntry(0, []), TargetEntry(0, 0, 0, 0), dartUri),
      });
      expect(skipList.length, 2);
      _validateRange(skipList[0], 0, 1, 10, 19);
      _validateRange(skipList.last, 10, 21, maxValue, maxValue);
    });

    test('contains the provided id', () async {
      final id = '123';
      final skipList = await skipLists.compute(id, {});
      for (var range in skipList) {
        expect(range['scriptId'], id);
      }
    });

    test('ignores the whole file if provided no locations', () async {
      final skipList = await skipLists.compute('123', {});
      expect(skipList.length, 1);
      _validateRange(skipList.first, 0, 0, maxValue, maxValue);
    });
  });
}

void _validateRange(Map<String, dynamic> range, int startLine, int startColumn,
    int endLine, int endColumn) {
  final start = range['start'];
  expect(start['lineNumber'], startLine);
  expect(start['columnNumber'], startColumn);
  final end = range['end'];
  expect(end['lineNumber'], endLine);
  expect(end['columnNumber'], endColumn);
}
