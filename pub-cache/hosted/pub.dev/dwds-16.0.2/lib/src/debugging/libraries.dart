// Copyright (c) 2019, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.import 'dart:async';

import 'package:collection/collection.dart';
import 'package:logging/logging.dart';
import 'package:vm_service/vm_service.dart';
import 'package:webkit_inspection_protocol/webkit_inspection_protocol.dart';

import '../loaders/strategy.dart';
import '../utilities/domain.dart';
import '../services/chrome_debug_exception.dart';
import 'metadata/class.dart';

/// Keeps track of Dart libraries available in the running application.
class LibraryHelper extends Domain {
  final Logger _logger = Logger('LibraryHelper');

  /// Map of library ID to [Library].
  final _librariesById = <String, Library>{};

  /// Map of libraryRef ID to [LibraryRef].
  final _libraryRefsById = <String, LibraryRef>{};

  LibraryRef? _rootLib;

  LibraryHelper(AppInspectorInterface appInspector) {
    inspector = appInspector;
  }

  Future<LibraryRef> get rootLib async {
    if (_rootLib != null) return _rootLib!;
    // TODO: read entrypoint from app metadata.
    // Issue: https://github.com/dart-lang/webdev/issues/1290
    final libraries = await libraryRefs;
    _rootLib = libraries
        .firstWhereOrNull((lib) => lib.name?.contains('org-dartlang') ?? false);
    _rootLib = _rootLib ??
        libraries
            .firstWhereOrNull((lib) => lib.name?.contains('main') ?? false);
    _rootLib = _rootLib ?? (libraries.isNotEmpty ? libraries.last : null);
    return _rootLib!;
  }

  /// Returns all libraryRefs in the app.
  ///
  /// Note this can return a cached result.
  Future<List<LibraryRef>> get libraryRefs async {
    if (_libraryRefsById.isNotEmpty) return _libraryRefsById.values.toList();
    final libraries = await globalLoadStrategy
        .metadataProviderFor(inspector.appConnection.request.entrypointPath)
        .libraries;
    for (var library in libraries) {
      _libraryRefsById[library] =
          LibraryRef(id: library, name: library, uri: library);
    }
    return _libraryRefsById.values.toList();
  }

  Future<Library?> libraryFor(LibraryRef libraryRef) async {
    final libraryId = libraryRef.id;
    if (libraryId == null) return null;
    final library =
        _librariesById[libraryId] ?? await _constructLibrary(libraryRef);
    if (library == null) return null;
    return _librariesById[libraryId] = library;
  }

  Future<LibraryRef?> libraryRefFor(String objectId) async {
    if (_libraryRefsById.isEmpty) await libraryRefs;
    return _libraryRefsById[objectId];
  }

  Future<Library?> _constructLibrary(LibraryRef libraryRef) async {
    final libraryId = libraryRef.id;
    final libraryUri = libraryRef.uri;
    if (libraryId == null || libraryUri == null) return null;

    // Fetch information about all the classes in this library.
    final expression = '''
    (function() {
      ${globalLoadStrategy.loadLibrarySnippet(libraryUri)}
      var result = {};
      var classes = Object.values(Object.getOwnPropertyDescriptors(library))
        .filter((p) => 'value' in p)
        .map((p) => p.value)
        .filter((l) => l && sdkUtils.isType(l));
      var classList = classes.map(function(clazz) {
        var descriptor = {
          'name': clazz.name,
          'dartName': sdkUtils.typeName(clazz)
        };
        return descriptor;
      });
      result['classes'] = classList;
      return result;
    })()
    ''';
    RemoteObject? result;
    try {
      result = await inspector.jsEvaluate(expression, returnByValue: true);
    } on ChromeDebugException catch (_) {
      // Unreferenced libraries are not loaded at runtime,
      // return empty library object for consistency among
      // VM Service implementations.
      // TODO: Collect library and class information from debug symbols.
      _logger.warning('Library ${libraryRef.uri} is not loaded. '
          'This can happen for unreferenced libraries.');
    }
    final classRefs = <ClassRef>[];
    if (result != null) {
      final jsonValues = result.value as Map<String, dynamic>;
      final classDescriptors =
          List<Map<String, dynamic>>.from(jsonValues['classes'] ?? []);
      for (final classDescriptor in classDescriptors) {
        final classMetaData = ClassMetaData(
          jsName: classDescriptor['name'] as Object?,
          libraryId: libraryRef.id,
          dartName: classDescriptor['dartName'] as Object?,
        );
        classRefs.add(classMetaData.classRef);
      }
    }
    return Library(
      name: libraryRef.name,
      uri: libraryRef.uri,
      debuggable: true,
      dependencies: [],
      scripts: await inspector.scriptRefsForLibrary(libraryId),
      variables: [],
      functions: [],
      classes: classRefs,
      id: libraryId,
    );
  }
}
