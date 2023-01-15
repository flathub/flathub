// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

@TestOn('vm')
import 'dart:io';

import 'package:dwds/src/version.dart';
import 'package:pub_semver/pub_semver.dart';
import 'package:pubspec_parse/pubspec_parse.dart';
import 'package:test/test.dart';

void main() {
  test('dwds lib/src/version.dart matches the pubspec version', () {
    final pubspec = Pubspec.parse(File('pubspec.yaml').readAsStringSync());
    expect(Version.parse(packageVersion), pubspec.version,
        reason: 'Please run `pub run build_runner build '
            '--build-filter=lib/src/version.dart` to update the version.');
  });
}
