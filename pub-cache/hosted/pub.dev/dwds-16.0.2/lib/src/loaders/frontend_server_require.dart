// Copyright 2020 The Dart Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:path/path.dart' as p;

import '../debugging/metadata/provider.dart';
import '../loaders/strategy.dart';
import '../readers/asset_reader.dart';
import '../services/expression_compiler.dart';
import 'require.dart';

/// Provides a [RequireStrategy] suitable for use with Frontend Server.
class FrontendServerRequireStrategyProvider {
  final ReloadConfiguration _configuration;
  final AssetReader _assetReader;
  final PackageUriMapper _packageUriMapper;
  final Future<Map<String, String>> Function() _digestsProvider;
  final String _basePath;

  late final RequireStrategy _requireStrategy = RequireStrategy(
    _configuration,
    _moduleProvider,
    (_) => _digestsProvider(),
    _moduleForServerPath,
    _serverPathForModule,
    _sourceMapPathForModule,
    _serverPathForAppUri,
    _moduleInfoForProvider,
    _assetReader,
  );

  FrontendServerRequireStrategyProvider(
    this._configuration,
    this._assetReader,
    this._packageUriMapper,
    this._digestsProvider,
    this._basePath,
  );

  RequireStrategy get strategy => _requireStrategy;

  String _removeBasePath(String path) {
    if (_basePath.isEmpty) return path;

    final stripped = stripLeadingSlashes(path);
    return stripLeadingSlashes(stripped.substring(_basePath.length));
  }

  String _addBasePath(String serverPath) => _basePath.isEmpty
      ? stripLeadingSlashes(serverPath)
      : '$_basePath/${stripLeadingSlashes(serverPath)}';

  Future<Map<String, String>> _moduleProvider(
          MetadataProvider metadataProvider) async =>
      (await metadataProvider.moduleToModulePath).map((key, value) =>
          MapEntry(key, stripLeadingSlashes(removeJsExtension(value))));

  Future<String?> _moduleForServerPath(
      MetadataProvider metadataProvider, String serverPath) async {
    final modulePathToModule = await metadataProvider.modulePathToModule;
    final relativeServerPath = _removeBasePath(serverPath);
    return modulePathToModule[relativeServerPath];
  }

  Future<String> _serverPathForModule(
          MetadataProvider metadataProvider, String module) async =>
      _addBasePath((await metadataProvider.moduleToModulePath)[module] ?? '');

  Future<String> _sourceMapPathForModule(
          MetadataProvider metadataProvider, String module) async =>
      _addBasePath((await metadataProvider.moduleToSourceMap)[module] ?? '');

  String? _serverPathForAppUri(String appUrl) {
    final appUri = Uri.parse(appUrl);
    if (appUri.isScheme('org-dartlang-app')) {
      return _addBasePath(appUri.path);
    }
    if (appUri.isScheme('package')) {
      final resolved = _packageUriMapper.packageUriToServerPath(appUri);
      if (resolved != null) {
        return resolved;
      }
    }
    return null;
  }

  Future<Map<String, ModuleInfo>> _moduleInfoForProvider(
      MetadataProvider metadataProvider) async {
    final modules = await metadataProvider.moduleToModulePath;
    final result = <String, ModuleInfo>{};
    for (var module in modules.keys) {
      final modulePath = modules[module]!;
      result[module] = ModuleInfo(
          // TODO: Save locations of full kernel files in ddc metadata.
          // Issue: https://github.com/dart-lang/sdk/issues/43684
          p.setExtension(modulePath, '.full.dill'),
          p.setExtension(modulePath, '.dill'));
    }
    return result;
  }
}
