// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'package:coverage/coverage.dart';
import 'package:mockito/annotations.dart';
import 'package:mockito/mockito.dart';
import 'package:test/test.dart';
import 'package:vm_service/vm_service.dart';

import 'collect_coverage_mock_test.mocks.dart';

@GenerateMocks([VmService])
SourceReportRange _range(int scriptIndex, SourceReportCoverage coverage) =>
    SourceReportRange(scriptIndex: scriptIndex, coverage: coverage);

Script _script(List<List<int>> tokenPosTable) =>
    Script(id: 'script', tokenPosTable: tokenPosTable);

IsolateRef _isoRef(String id, String isoGroupId) =>
    IsolateRef(id: id, isolateGroupId: isoGroupId);

IsolateGroupRef _isoGroupRef(String id) => IsolateGroupRef(id: id);

IsolateGroup _isoGroup(String id, List<IsolateRef> isolates) =>
    IsolateGroup(id: id, isolates: isolates);

class FakeSentinelException implements SentinelException {
  @override
  dynamic noSuchMethod(Invocation invocation) {}
}

MockVmService _mockService(
  int majorVersion,
  int minorVersion, {
  Map<String, List<String>> isolateGroups = const {
    'isolateGroup': ['isolate'],
  },
}) {
  final service = MockVmService();
  final isoRefs = <IsolateRef>[];
  final isoGroupRefs = <IsolateGroupRef>[];
  final isoGroups = <IsolateGroup>[];
  for (final group in isolateGroups.entries) {
    isoGroupRefs.add(_isoGroupRef(group.key));
    final isosOfGroup = <IsolateRef>[];
    for (final isoId in group.value) {
      isosOfGroup.add(_isoRef(isoId, group.key));
    }
    isoGroups.add(_isoGroup(group.key, isosOfGroup));
    isoRefs.addAll(isosOfGroup);
  }
  when(service.getVM()).thenAnswer(
      (_) async => VM(isolates: isoRefs, isolateGroups: isoGroupRefs));
  for (final group in isoGroups) {
    when(service.getIsolateGroup(group.id)).thenAnswer((_) async => group);
  }
  when(service.getVersion()).thenAnswer(
      (_) async => Version(major: majorVersion, minor: minorVersion));
  return service;
}

void main() {
  group('Mock VM Service', () {
    test('Collect coverage', () async {
      final service = _mockService(3, 0);
      when(service.getSourceReport('isolate', ['Coverage'], forceCompile: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [15],
                      misses: [32],
                    ),
                  ),
                  _range(
                    1,
                    SourceReportCoverage(
                      hits: [75],
                      misses: [34],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                  ScriptRef(
                    uri: 'package:bar/bar.dart',
                    id: 'bar',
                  ),
                ],
              ));
      when(service.getObject('isolate', 'foo'))
          .thenAnswer((_) async => _script([
                [12, 15, 7],
                [47, 32, 19],
              ]));
      when(service.getObject('isolate', 'bar'))
          .thenAnswer((_) async => _script([
                [52, 34, 10],
                [95, 75, 3],
              ]));

      final jsonResult = await collect(Uri(), false, false, false, null,
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 2);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
      expect(result['package:bar/bar.dart']?.lineHits, {95: 1, 52: 0});
    });

    test('Collect coverage, report lines', () async {
      final service = _mockService(3, 51);
      when(service.getSourceReport('isolate', ['Coverage'],
              forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [12],
                      misses: [47],
                    ),
                  ),
                  _range(
                    1,
                    SourceReportCoverage(
                      hits: [95],
                      misses: [52],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                  ScriptRef(
                    uri: 'package:bar/bar.dart',
                    id: 'bar',
                  ),
                ],
              ));

      final jsonResult = await collect(Uri(), false, false, false, null,
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 2);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
      expect(result['package:bar/bar.dart']?.lineHits, {95: 1, 52: 0});
    });

    test('Collect coverage, scoped output, no library filters', () async {
      final service = _mockService(3, 0);
      when(service.getScripts('isolate')).thenAnswer((_) async => ScriptList(
            scripts: [
              ScriptRef(
                uri: 'package:foo/foo.dart',
                id: 'foo',
              ),
              ScriptRef(
                uri: 'package:bar/bar.dart',
                id: 'bar',
              ),
            ],
          ));
      when(service.getSourceReport('isolate', ['Coverage'],
              scriptId: 'foo', forceCompile: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [15],
                      misses: [32],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                ],
              ));
      when(service.getObject('isolate', 'foo'))
          .thenAnswer((_) async => _script([
                [12, 15, 7],
                [47, 32, 19],
              ]));

      final jsonResult = await collect(Uri(), false, false, false, {'foo'},
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 1);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
    });

    test('Collect coverage, scoped output, library filters', () async {
      final service = _mockService(3, 57);
      when(service.getSourceReport('isolate', ['Coverage'],
              forceCompile: true,
              reportLines: true,
              libraryFilters: ['package:foo/']))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [12],
                      misses: [47],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                ],
              ));

      final jsonResult = await collect(Uri(), false, false, false, {'foo'},
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 1);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
    });

    test('Collect coverage, old isolate group deduping', () async {
      final service = _mockService(3, 60, isolateGroups: {
        'isolateGroupA': ['isolate1', 'isolate2'],
        'isolateGroupB': ['isolate3'],
      });
      when(service.getSourceReport('isolate1', ['Coverage'],
              forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [12],
                      misses: [47],
                    ),
                  ),
                  _range(
                    1,
                    SourceReportCoverage(
                      hits: [95],
                      misses: [52],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                  ScriptRef(
                    uri: 'package:bar/bar.dart',
                    id: 'bar',
                  ),
                ],
              ));
      when(service.getSourceReport('isolate3', ['Coverage'],
              forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [34],
                      misses: [61],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:baz/baz.dart',
                    id: 'baz',
                  ),
                ],
              ));

      final jsonResult = await collect(Uri(), false, false, false, null,
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 3);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
      expect(result['package:bar/bar.dart']?.lineHits, {95: 1, 52: 0});
      expect(result['package:baz/baz.dart']?.lineHits, {34: 1, 61: 0});
      verifyNever(service.getSourceReport('isolate2', ['Coverage'],
          forceCompile: true, reportLines: true));
      verify(service.getIsolateGroup('isolateGroupA'));
      verify(service.getIsolateGroup('isolateGroupB'));
    });

    test('Collect coverage, fast isolate group deduping', () async {
      final service = _mockService(3, 61, isolateGroups: {
        'isolateGroupA': ['isolate1', 'isolate2'],
        'isolateGroupB': ['isolate3'],
      });
      when(service.getSourceReport('isolate1', ['Coverage'],
              forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [12],
                      misses: [47],
                    ),
                  ),
                  _range(
                    1,
                    SourceReportCoverage(
                      hits: [95],
                      misses: [52],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:foo/foo.dart',
                    id: 'foo',
                  ),
                  ScriptRef(
                    uri: 'package:bar/bar.dart',
                    id: 'bar',
                  ),
                ],
              ));
      when(service.getSourceReport('isolate3', ['Coverage'],
              forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [34],
                      misses: [61],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:baz/baz.dart',
                    id: 'baz',
                  ),
                ],
              ));

      final jsonResult = await collect(Uri(), false, false, false, null,
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 3);
      expect(result['package:foo/foo.dart']?.lineHits, {12: 1, 47: 0});
      expect(result['package:bar/bar.dart']?.lineHits, {95: 1, 52: 0});
      expect(result['package:baz/baz.dart']?.lineHits, {34: 1, 61: 0});
      verifyNever(service.getSourceReport('isolate2', ['Coverage'],
          forceCompile: true, reportLines: true));
      verifyNever(service.getIsolateGroup('isolateGroupA'));
      verifyNever(service.getIsolateGroup('isolateGroupB'));
    });

    test(
        'Collect coverage, scoped output, no library filters, '
        'handles SentinelException from getScripts', () async {
      final service = _mockService(3, 0);
      when(service.getScripts('isolate')).thenThrow(FakeSentinelException());

      final jsonResult = await collect(
          Uri(), false, false, false, {'foo', 'bar'},
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 0);
    });

    test(
        'Collect coverage, scoped output, no library filters, '
        'handles SentinelException from getSourceReport', () async {
      final service = _mockService(3, 51);
      when(service.getScripts('isolate')).thenAnswer((_) async => ScriptList(
            scripts: [
              ScriptRef(
                uri: 'package:foo/foo.dart',
                id: 'foo',
              ),
              ScriptRef(
                uri: 'package:bar/bar.dart',
                id: 'bar',
              ),
            ],
          ));
      when(service.getSourceReport('isolate', ['Coverage'],
              scriptId: 'foo', forceCompile: true, reportLines: true))
          .thenThrow(FakeSentinelException());
      when(service.getSourceReport('isolate', ['Coverage'],
              scriptId: 'bar', forceCompile: true, reportLines: true))
          .thenAnswer((_) async => SourceReport(
                ranges: [
                  _range(
                    0,
                    SourceReportCoverage(
                      hits: [95],
                      misses: [52],
                    ),
                  ),
                ],
                scripts: [
                  ScriptRef(
                    uri: 'package:bar/bar.dart',
                    id: 'bar',
                  ),
                ],
              ));

      final jsonResult = await collect(
          Uri(), false, false, false, {'foo', 'bar'},
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);
      expect(result.length, 1);
      expect(result['package:bar/bar.dart']?.lineHits, {95: 1, 52: 0});
    });

    test(
        'Collect coverage, no scoped output, '
        'handles SentinelException from getSourceReport', () async {
      final service = _mockService(3, 0);
      when(service.getSourceReport('isolate', ['Coverage'], forceCompile: true))
          .thenThrow(FakeSentinelException());

      final jsonResult = await collect(Uri(), false, false, false, null,
          serviceOverrideForTesting: service);
      final result = await HitMap.parseJson(
          jsonResult['coverage'] as List<Map<String, dynamic>>);

      expect(result.length, 0);
    });
  });
}
