// Copyright (c) 2016, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@TestOn('vm')
import 'package:boolean_selector/boolean_selector.dart';
import 'package:test/test.dart';

import 'package:test_api/src/backend/platform_selector.dart';
import 'package:test_api/src/backend/runtime.dart';
import 'package:test_core/src/runner/runtime_selection.dart';

import '../../utils.dart';

void main() {
  group('merge', () {
    group('for most fields', () {
      test('if neither is defined, preserves the default', () {
        var merged = suiteConfiguration().merge(suiteConfiguration());
        expect(merged.jsTrace, isFalse);
        expect(merged.runSkipped, isFalse);
        expect(merged.precompiledPath, isNull);
        expect(merged.runtimes, equals([Runtime.vm.identifier]));
      });

      test("if only the old configuration's is defined, uses it", () {
        var merged = suiteConfiguration(
                jsTrace: true,
                runSkipped: true,
                precompiledPath: '/tmp/js',
                runtimes: [RuntimeSelection(Runtime.chrome.identifier)])
            .merge(suiteConfiguration());

        expect(merged.jsTrace, isTrue);
        expect(merged.runSkipped, isTrue);
        expect(merged.precompiledPath, equals('/tmp/js'));
        expect(merged.runtimes, equals([Runtime.chrome.identifier]));
      });

      test("if only the configuration's is defined, uses it", () {
        var merged = suiteConfiguration().merge(suiteConfiguration(
            jsTrace: true,
            runSkipped: true,
            precompiledPath: '/tmp/js',
            runtimes: [RuntimeSelection(Runtime.chrome.identifier)]));

        expect(merged.jsTrace, isTrue);
        expect(merged.runSkipped, isTrue);
        expect(merged.precompiledPath, equals('/tmp/js'));
        expect(merged.runtimes, equals([Runtime.chrome.identifier]));
      });

      test(
          "if the two configurations conflict, uses the configuration's "
          'values', () {
        var older = suiteConfiguration(
            jsTrace: false,
            runSkipped: true,
            precompiledPath: '/tmp/js',
            runtimes: [RuntimeSelection(Runtime.chrome.identifier)]);
        var newer = suiteConfiguration(
            jsTrace: true,
            runSkipped: false,
            precompiledPath: '../js',
            runtimes: [RuntimeSelection(Runtime.firefox.identifier)]);
        var merged = older.merge(newer);

        expect(merged.jsTrace, isTrue);
        expect(merged.runSkipped, isFalse);
        expect(merged.precompiledPath, equals('../js'));
        expect(merged.runtimes, equals([Runtime.firefox.identifier]));
      });
    });

    group('for dart2jsArgs', () {
      test('if neither is defined, preserves the default', () {
        var merged = suiteConfiguration().merge(suiteConfiguration());
        expect(merged.dart2jsArgs, isEmpty);
      });

      test("if only the old configuration's is defined, uses it", () {
        var merged = suiteConfiguration(dart2jsArgs: ['--foo', '--bar'])
            .merge(suiteConfiguration());
        expect(merged.dart2jsArgs, equals(['--foo', '--bar']));
      });

      test("if only the configuration's is defined, uses it", () {
        var merged = suiteConfiguration()
            .merge(suiteConfiguration(dart2jsArgs: ['--foo', '--bar']));
        expect(merged.dart2jsArgs, equals(['--foo', '--bar']));
      });

      test('if both are defined, concatenates them', () {
        var older = suiteConfiguration(dart2jsArgs: ['--foo', '--bar']);
        var newer = suiteConfiguration(dart2jsArgs: ['--baz']);
        var merged = older.merge(newer);
        expect(merged.dart2jsArgs, equals(['--foo', '--bar', '--baz']));
      });
    });

    group('for config maps', () {
      test('merges each nested configuration', () {
        var merged = suiteConfiguration(tags: {
          BooleanSelector.parse('foo'):
              suiteConfiguration(precompiledPath: 'path/'),
          BooleanSelector.parse('bar'): suiteConfiguration(jsTrace: true)
        }, onPlatform: {
          PlatformSelector.parse('vm'):
              suiteConfiguration(precompiledPath: 'path/'),
          PlatformSelector.parse('chrome'): suiteConfiguration(jsTrace: true)
        }).merge(suiteConfiguration(tags: {
          BooleanSelector.parse('bar'): suiteConfiguration(jsTrace: false),
          BooleanSelector.parse('baz'): suiteConfiguration(runSkipped: true)
        }, onPlatform: {
          PlatformSelector.parse('chrome'): suiteConfiguration(jsTrace: false),
          PlatformSelector.parse('firefox'):
              suiteConfiguration(runSkipped: true)
        }));

        expect(merged.tags[BooleanSelector.parse('foo')]!.precompiledPath,
            equals('path/'));
        expect(merged.tags[BooleanSelector.parse('bar')]!.jsTrace, isFalse);
        expect(merged.tags[BooleanSelector.parse('baz')]!.runSkipped, isTrue);

        expect(merged.onPlatform[PlatformSelector.parse('vm')]!.precompiledPath,
            'path/');
        expect(merged.onPlatform[PlatformSelector.parse('chrome')]!.jsTrace,
            isFalse);
        expect(merged.onPlatform[PlatformSelector.parse('firefox')]!.runSkipped,
            isTrue);
      });
    });
  });
}
