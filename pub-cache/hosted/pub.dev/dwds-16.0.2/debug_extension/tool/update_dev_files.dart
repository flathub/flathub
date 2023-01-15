// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:io';

void main() async {
  _updateManifestJson();
  _updateDevtoolsJs();
}

/// Adds the Googler extension key, updates the extension icon, and prefixes the
/// extension name with "[DEV]".
Future<void> _updateManifestJson() async {
  final manifestJson = File('dev_build/web/manifest.json');
  final extensionKeyTxt = File('extension_key.txt');
  final extensionKey = await extensionKeyTxt.exists()
      ? await extensionKeyTxt.readAsString()
      : null;
  _transformDevFile(manifestJson, (line) {
    if (_matchesKey(line: line, key: 'name')) {
      return [
        _newKeyValue(
          oldLine: line,
          newKey: 'name',
          newValue: '[DEV] Dart Debug Extension',
        ),
        if (extensionKey != null)
          _newKeyValue(
            oldLine: line,
            newKey: 'key',
            newValue: extensionKey,
          ),
      ];
    }
    if (_matchesKey(line: line, key: 'default_icon')) {
      return [
        _newKeyValue(
          oldLine: line,
          newKey: 'default_icon',
          newValue: 'dart_dev.png',
        )
      ];
    }
    if (_matchesValue(line: line, value: 'background.js')) {
      return [
        _newKeyValue(
          oldLine: line,
          newKey: null,
          newValue: 'background.dart.js',
        )
      ];
    } else {
      return [line];
    }
  });
}

/// Prefixes the names of the panels that are added to Chrome DevTools with
/// "[DEV]".
Future<void> _updateDevtoolsJs() async {
  final devtoolsJs = File('dev_build/web/devtools.js');
  return _transformDevFile(devtoolsJs, (line) {
    final originalDebuggerLine = "const DEBUGGER_PANEL_NAME = 'Dart Debugger';";
    final modifiedDebuggerLine =
        "const DEBUGGER_PANEL_NAME = '[DEV] Dart Debugger';";
    final originalInspectorLine =
        "const INSPECTOR_PANEL_NAME = 'Flutter Inspector';";
    final modifiedInspectorLine =
        "const INSPECTOR_PANEL_NAME = '[DEV] Flutter Inspector';";
    if (_matchesLine(line: line, match: originalDebuggerLine)) {
      return [_newLine(oldLine: line, newLine: modifiedDebuggerLine)];
    }
    if (_matchesLine(line: line, match: originalInspectorLine)) {
      return [_newLine(oldLine: line, newLine: modifiedInspectorLine)];
    }
    return [line];
  });
}

Future<void> _transformDevFile(
    File devFile, List<String> Function(String) transformLine) async {
  final lines = devFile.readAsLinesSync();
  final newLines = <String>[];
  for (final line in lines) {
    newLines.addAll(transformLine(line));
  }
  final content = newLines.joinWithNewLine();
  return devFile.writeAsStringSync(content);
}

bool _matchesKey({required String line, required String key}) {
  return line.trimLeft().startsWith('"$key":');
}

bool _matchesValue({required String line, required String value}) {
  return line.trimRight().endsWith('"$value"') ||
      line.trimRight().endsWith('"$value",');
}

bool _matchesLine({required String line, required String match}) {
  return line.trim() == match;
}

String _newKeyValue({
  required String oldLine,
  String? newKey,
  String? newValue,
}) {
  final lineStart = oldLine.leftPadding();
  final key = newKey != null ? '"$newKey": ' : '';
  final value = newValue != null ? '"$newValue"' : '';
  final lineEnd = oldLine.trim().endsWith(',') ? ',' : '';
  return '$lineStart$key$value$lineEnd';
}

String _newLine({
  required String oldLine,
  required String newLine,
}) {
  final lineStart = oldLine.leftPadding();
  return '$lineStart$newLine';
}

extension LeftPaddingExtension on String {
  String leftPadding() {
    String padding = '';
    int idx = 0;
    while (idx < length && this[idx] == ' ') {
      padding += ' ';
      idx++;
    }
    return padding;
  }
}

extension JoinExtension on List<String> {
  String joinWithNewLine() {
    return '${join('\n')}\n';
  }
}
