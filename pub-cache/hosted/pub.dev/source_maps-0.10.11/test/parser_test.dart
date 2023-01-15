// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:convert';

import 'package:source_maps/source_maps.dart';
import 'package:source_span/source_span.dart';
import 'package:test/test.dart';

import 'common.dart';

const Map<String, dynamic> _mapWithNoSourceLocation = {
  'version': 3,
  'sourceRoot': '',
  'sources': ['input.dart'],
  'names': [],
  'mappings': 'A',
  'file': 'output.dart'
};

const Map<String, dynamic> _mapWithSourceLocation = {
  'version': 3,
  'sourceRoot': '',
  'sources': ['input.dart'],
  'names': [],
  'mappings': 'AAAA',
  'file': 'output.dart'
};

const Map<String, dynamic> _mapWithSourceLocationAndMissingNames = {
  'version': 3,
  'sourceRoot': '',
  'sources': ['input.dart'],
  'mappings': 'AAAA',
  'file': 'output.dart'
};

const Map<String, dynamic> _mapWithSourceLocationAndName = {
  'version': 3,
  'sourceRoot': '',
  'sources': ['input.dart'],
  'names': ['var'],
  'mappings': 'AAAAA',
  'file': 'output.dart'
};

const Map<String, dynamic> _mapWithSourceLocationAndName1 = {
  'version': 3,
  'sourceRoot': 'pkg/',
  'sources': ['input1.dart'],
  'names': ['var1'],
  'mappings': 'AAAAA',
  'file': 'output.dart'
};

const Map<String, dynamic> _mapWithSourceLocationAndName2 = {
  'version': 3,
  'sourceRoot': 'pkg/',
  'sources': ['input2.dart'],
  'names': ['var2'],
  'mappings': 'AAAAA',
  'file': 'output2.dart'
};

const Map<String, dynamic> _mapWithSourceLocationAndName3 = {
  'version': 3,
  'sourceRoot': 'pkg/',
  'sources': ['input3.dart'],
  'names': ['var3'],
  'mappings': 'AAAAA',
  'file': '3/output.dart'
};

const _sourceMapBundle = [
  _mapWithSourceLocationAndName1,
  _mapWithSourceLocationAndName2,
  _mapWithSourceLocationAndName3,
];

void main() {
  test('parse', () {
    var mapping = parseJson(expectedMap);
    check(outputVar1, mapping, inputVar1, false);
    check(outputVar2, mapping, inputVar2, false);
    check(outputFunction, mapping, inputFunction, false);
    check(outputExpr, mapping, inputExpr, false);
  });

  test('parse + json', () {
    var mapping = parse(jsonEncode(expectedMap));
    check(outputVar1, mapping, inputVar1, false);
    check(outputVar2, mapping, inputVar2, false);
    check(outputFunction, mapping, inputFunction, false);
    check(outputExpr, mapping, inputExpr, false);
  });

  test('parse with file', () {
    var mapping = parseJson(expectedMap);
    check(outputVar1, mapping, inputVar1, true);
    check(outputVar2, mapping, inputVar2, true);
    check(outputFunction, mapping, inputFunction, true);
    check(outputExpr, mapping, inputExpr, true);
  });

  test('parse with no source location', () {
    var map = parse(jsonEncode(_mapWithNoSourceLocation)) as SingleMapping;
    expect(map.lines.length, 1);
    expect(map.lines.first.entries.length, 1);
    var entry = map.lines.first.entries.first;

    expect(entry.column, 0);
    expect(entry.sourceUrlId, null);
    expect(entry.sourceColumn, null);
    expect(entry.sourceLine, null);
    expect(entry.sourceNameId, null);
  });

  test('parse with source location and no name', () {
    var map = parse(jsonEncode(_mapWithSourceLocation)) as SingleMapping;
    expect(map.lines.length, 1);
    expect(map.lines.first.entries.length, 1);
    var entry = map.lines.first.entries.first;

    expect(entry.column, 0);
    expect(entry.sourceUrlId, 0);
    expect(entry.sourceColumn, 0);
    expect(entry.sourceLine, 0);
    expect(entry.sourceNameId, null);
  });

  test('parse with source location and missing names entry', () {
    var map = parse(jsonEncode(_mapWithSourceLocationAndMissingNames))
        as SingleMapping;
    expect(map.lines.length, 1);
    expect(map.lines.first.entries.length, 1);
    var entry = map.lines.first.entries.first;

    expect(entry.column, 0);
    expect(entry.sourceUrlId, 0);
    expect(entry.sourceColumn, 0);
    expect(entry.sourceLine, 0);
    expect(entry.sourceNameId, null);
  });

  test('parse with source location and name', () {
    var map = parse(jsonEncode(_mapWithSourceLocationAndName)) as SingleMapping;
    expect(map.lines.length, 1);
    expect(map.lines.first.entries.length, 1);
    var entry = map.lines.first.entries.first;

    expect(entry.sourceUrlId, 0);
    expect(entry.sourceUrlId, 0);
    expect(entry.sourceColumn, 0);
    expect(entry.sourceLine, 0);
    expect(entry.sourceNameId, 0);
  });

  test('parse with source root', () {
    var inputMap = Map.from(_mapWithSourceLocation);
    inputMap['sourceRoot'] = '/pkg/';
    var mapping = parseJson(inputMap) as SingleMapping;
    expect(mapping.spanFor(0, 0)?.sourceUrl, Uri.parse('/pkg/input.dart'));
    expect(
        mapping
            .spanForLocation(
                SourceLocation(0, sourceUrl: Uri.parse('ignored.dart')))
            ?.sourceUrl,
        Uri.parse('/pkg/input.dart'));

    var newSourceRoot = '/new/';

    mapping.sourceRoot = newSourceRoot;
    inputMap['sourceRoot'] = newSourceRoot;

    expect(mapping.toJson(), equals(inputMap));
  });

  test('parse with map URL', () {
    var inputMap = Map.from(_mapWithSourceLocation);
    inputMap['sourceRoot'] = 'pkg/';
    var mapping = parseJson(inputMap, mapUrl: 'file:///path/to/map');
    expect(mapping.spanFor(0, 0)?.sourceUrl,
        Uri.parse('file:///path/to/pkg/input.dart'));
  });

  group('parse with bundle', () {
    var mapping =
        parseJsonExtended(_sourceMapBundle, mapUrl: 'file:///path/to/map');

    test('simple', () {
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.file('/path/to/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.file('/path/to/output2.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.file('/path/to/3/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));

      expect(
          mapping.spanFor(0, 0, uri: 'file:///path/to/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(
          mapping.spanFor(0, 0, uri: 'file:///path/to/output2.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(
          mapping
              .spanFor(0, 0, uri: 'file:///path/to/3/output.dart')
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });

    test('package uris', () {
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('package:1/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('package:2/output2.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('package:3/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));

      expect(mapping.spanFor(0, 0, uri: 'package:1/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(mapping.spanFor(0, 0, uri: 'package:2/output2.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(mapping.spanFor(0, 0, uri: 'package:3/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });

    test('unmapped path', () {
      var span = mapping.spanFor(0, 0, uri: 'unmapped_output.dart')!;
      expect(span.sourceUrl, Uri.parse('unmapped_output.dart'));
      expect(span.start.line, equals(0));
      expect(span.start.column, equals(0));

      span = mapping.spanFor(10, 5, uri: 'unmapped_output.dart')!;
      expect(span.sourceUrl, Uri.parse('unmapped_output.dart'));
      expect(span.start.line, equals(10));
      expect(span.start.column, equals(5));
    });

    test('missing path', () {
      expect(() => mapping.spanFor(0, 0), throwsA(anything));
    });

    test('incomplete paths', () {
      expect(mapping.spanFor(0, 0, uri: 'output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(mapping.spanFor(0, 0, uri: 'output2.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(mapping.spanFor(0, 0, uri: '3/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });

    test('parseExtended', () {
      var mapping = parseExtended(jsonEncode(_sourceMapBundle),
          mapUrl: 'file:///path/to/map');

      expect(mapping.spanFor(0, 0, uri: 'output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(mapping.spanFor(0, 0, uri: 'output2.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(mapping.spanFor(0, 0, uri: '3/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });

    test('build bundle incrementally', () {
      var mapping = MappingBundle();

      mapping.addMapping(parseJson(_mapWithSourceLocationAndName1,
          mapUrl: 'file:///path/to/map') as SingleMapping);
      expect(mapping.spanFor(0, 0, uri: 'output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));

      expect(mapping.containsMapping('output2.dart'), isFalse);
      mapping.addMapping(parseJson(_mapWithSourceLocationAndName2,
          mapUrl: 'file:///path/to/map') as SingleMapping);
      expect(mapping.containsMapping('output2.dart'), isTrue);
      expect(mapping.spanFor(0, 0, uri: 'output2.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));

      expect(mapping.containsMapping('3/output.dart'), isFalse);
      mapping.addMapping(parseJson(_mapWithSourceLocationAndName3,
          mapUrl: 'file:///path/to/map') as SingleMapping);
      expect(mapping.containsMapping('3/output.dart'), isTrue);
      expect(mapping.spanFor(0, 0, uri: '3/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });

    // Test that the source map can handle cases where the uri passed in is
    // not from the expected host but it is still unambiguous which source
    // map should be used.
    test('different paths', () {
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('http://localhost/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('http://localhost/output2.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(
          mapping
              .spanForLocation(SourceLocation(0,
                  sourceUrl: Uri.parse('http://localhost/3/output.dart')))
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));

      expect(
          mapping.spanFor(0, 0, uri: 'http://localhost/output.dart')?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input1.dart'));
      expect(
          mapping
              .spanFor(0, 0, uri: 'http://localhost/output2.dart')
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input2.dart'));
      expect(
          mapping
              .spanFor(0, 0, uri: 'http://localhost/3/output.dart')
              ?.sourceUrl,
          Uri.parse('file:///path/to/pkg/input3.dart'));
    });
  });

  test('parse and re-emit', () {
    for (var expected in [
      expectedMap,
      _mapWithNoSourceLocation,
      _mapWithSourceLocation,
      _mapWithSourceLocationAndName
    ]) {
      var mapping = parseJson(expected) as SingleMapping;
      expect(mapping.toJson(), equals(expected));

      mapping = parseJsonExtended(expected) as SingleMapping;
      expect(mapping.toJson(), equals(expected));
    }

    var mapping = parseJsonExtended(_sourceMapBundle) as MappingBundle;
    expect(mapping.toJson(), equals(_sourceMapBundle));
  });

  test('parse extensions', () {
    var map = Map.from(expectedMap);
    map['x_foo'] = 'a';
    map['x_bar'] = [3];
    var mapping = parseJson(map) as SingleMapping;
    expect(mapping.toJson(), equals(map));
    expect(mapping.extensions['x_foo'], equals('a'));
    expect(mapping.extensions['x_bar'].first, equals(3));
  });

  group('source files', () {
    group('from fromEntries()', () {
      test('are null for non-FileLocations', () {
        var mapping = SingleMapping.fromEntries([
          Entry(SourceLocation(10, line: 1, column: 8), outputVar1.start, null)
        ]);
        expect(mapping.files, equals([null]));
      });

      test("use a file location's file", () {
        var mapping = SingleMapping.fromEntries(
            [Entry(inputVar1.start, outputVar1.start, null)]);
        expect(mapping.files, equals([input]));
      });
    });

    group('from parse()', () {
      group('are null', () {
        test('with no sourcesContent field', () {
          var mapping = parseJson(expectedMap) as SingleMapping;
          expect(mapping.files, equals([null]));
        });

        test('with null sourcesContent values', () {
          var map = Map.from(expectedMap);
          map['sourcesContent'] = [null];
          var mapping = parseJson(map) as SingleMapping;
          expect(mapping.files, equals([null]));
        });

        test('with a too-short sourcesContent', () {
          var map = Map.from(expectedMap);
          map['sourcesContent'] = [];
          var mapping = parseJson(map) as SingleMapping;
          expect(mapping.files, equals([null]));
        });
      });

      test('are parsed from sourcesContent', () {
        var map = Map.from(expectedMap);
        map['sourcesContent'] = ['hello, world!'];
        var mapping = parseJson(map) as SingleMapping;

        var file = mapping.files[0]!;
        expect(file.url, equals(Uri.parse('input.dart')));
        expect(file.getText(0), equals('hello, world!'));
      });
    });
  });
}
