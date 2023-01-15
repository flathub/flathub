// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:io';

import 'package:benchmark_harness/benchmark_harness.dart';

import '../bin/collect_coverage.dart' as collect_coverage;
import '../bin/format_coverage.dart' as format_coverage;

// Runs a test script with various different coverage configurations.
class CoverageBenchmark extends AsyncBenchmarkBase {
  CoverageBenchmark(
    ScoreEmitter emitter,
    String name,
    this.script, {
    this.gatherCoverage = false,
    this.functionCoverage = false,
    this.branchCoverage = false,
  }) : super(name, emitter: emitter);

  final String script;
  final bool gatherCoverage;
  final bool functionCoverage;
  final bool branchCoverage;
  int iteration = 0;

  @override
  Future<void> run() async {
    print('Running $name...');
    final covFile = 'data/$name $iteration coverage.json';
    final lcovFile = 'data/$name $iteration lcov.info';
    ++iteration;

    await Process.start(
      Platform.executable,
      [
        if (branchCoverage) '--branch-coverage',
        'run',
        if (gatherCoverage) ...[
          '--pause-isolates-on-exit',
          '--disable-service-auth-codes',
          '--enable-vm-service=1234',
        ],
        script,
      ],
      mode: ProcessStartMode.detached,
    );
    if (gatherCoverage) {
      await collect_coverage.main([
        '--wait-paused',
        '--resume-isolates',
        '--uri=http://127.0.0.1:1234/',
        if (branchCoverage) '--branch-coverage',
        if (functionCoverage) '--function-coverage',
        '-o',
        covFile,
      ]);

      await format_coverage.main([
        '--lcov',
        '--check-ignore',
        '-i',
        covFile,
        '-o',
        lcovFile,
      ]);
    }
  }
}

// Emitter that just captures the value.
class CaptureEmitter implements ScoreEmitter {
  late double capturedValue;

  @override
  void emit(String testName, double value) {
    capturedValue = value;
  }
}

// Prints a JSON representation of the benchmark results, in a format compatible
// with the github benchmark action.
class JsonEmitter implements ScoreEmitter {
  JsonEmitter(this._baseline);

  final double _baseline;
  final _results = <String, double>{};

  @override
  void emit(String testName, double value) {
    _results[testName] = value;
  }

  String write() => '[${_results.entries.map((entry) => """{
  "name": "${entry.key}",
  "unit": "times slower",
  "value": ${(entry.value / _baseline).toStringAsFixed(2)}
}""").join(',\n')}]';
}

Future<void> runBenchmark(CoverageBenchmark benchmark) async {
  for (int i = 0; i < 3; ++i) {
    try {
      await benchmark.report().timeout(Duration(minutes: 2));
      return;
    } on TimeoutException {
      print('Timed out');
    }
  }
  print('Timed out too many times. Giving up.');
  exit(127);
}

Future<String> runBenchmarkSet(String name, String script) async {
  final captureEmitter = CaptureEmitter();
  await runBenchmark(
      CoverageBenchmark(captureEmitter, '$name - no coverage', script));
  final benchmarkBaseline = captureEmitter.capturedValue;

  final emitter = JsonEmitter(benchmarkBaseline);
  await runBenchmark(CoverageBenchmark(
      emitter, '$name - basic coverage', script,
      gatherCoverage: true));
  await runBenchmark(CoverageBenchmark(
      emitter, '$name - function coverage', script,
      gatherCoverage: true, functionCoverage: true));
  await runBenchmark(CoverageBenchmark(
      emitter, '$name - branch coverage', script,
      gatherCoverage: true, branchCoverage: true));
  return emitter.write();
}

Future<void> main() async {
  // Assume this script was started from the root coverage directory. Change to
  // the benchmark directory.
  Directory.current = 'benchmark';
  final result = await runBenchmarkSet('Many isolates', 'many_isolates.dart');
  await File('data/benchmark_result.json').writeAsString(result);
  exit(0);
}
