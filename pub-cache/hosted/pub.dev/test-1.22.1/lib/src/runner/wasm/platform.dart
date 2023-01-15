// Copyright (c) 2022, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:async/async.dart';
import 'package:http_multi_server/http_multi_server.dart';
import 'package:path/path.dart' as p;
import 'package:shelf/shelf.dart' as shelf;
import 'package:shelf/shelf_io.dart' as shelf_io;
import 'package:shelf_packages_handler/shelf_packages_handler.dart';
import 'package:shelf_static/shelf_static.dart';
import 'package:shelf_web_socket/shelf_web_socket.dart';
// ignore: deprecated_member_use
import 'package:test_api/backend.dart' show Runtime, SuitePlatform;
import 'package:test_core/src/runner/configuration.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/package_version.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/platform.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/plugin/customizable_platform.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/runner_suite.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/suite.dart'; // ignore: implementation_imports
import 'package:test_core/src/runner/wasm_compiler_pool.dart'; // ignore: implementation_imports
import 'package:test_core/src/util/io.dart'; // ignore: implementation_imports
import 'package:test_core/src/util/package_config.dart'; // ignore: implementation_imports
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:yaml/yaml.dart';

import '../../util/math.dart';
import '../../util/one_off_handler.dart';
import '../../util/path_handler.dart';
import '../browser/browser_manager.dart';
import '../executable_settings.dart';
import 'default_settings.dart';

class BrowserWasmPlatform extends PlatformPlugin
    implements CustomizablePlatform<ExecutableSettings> {
  /// Starts the server.
  ///
  /// [root] is the root directory that the server should serve. It defaults to
  /// the working directory.
  static Future<BrowserWasmPlatform> start({String? root}) async {
    print('WARNING: Running with the experimental wasm platform. This platform '
        'is temporary and will be removed in the future, without a breaking '
        'change release, in favor of a different method of running wasm tests.'
        'This platform should only be used for experimentation at this time.'
        '\n');
    var server = shelf_io.IOServer(await HttpMultiServer.loopback(0));
    var packageConfig = await currentPackageConfig;
    return BrowserWasmPlatform._(
        server,
        Configuration.current,
        p.fromUri(packageConfig.resolve(
            Uri.parse('package:test/src/runner/browser/static/favicon.ico'))),
        p.fromUri(packageConfig.resolve(
            Uri.parse('package:test/src/runner/wasm/static/default.html.tpl'))),
        p.fromUri(packageConfig.resolve(Uri.parse(
            'package:test/src/runner/wasm/static/run_wasm_chrome.js'))),
        root: root);
  }

  /// The test runner configuration.
  final Configuration _config;

  /// The underlying server.
  final shelf.Server _server;

  /// A randomly-generated secret.
  ///
  /// This is used to ensure that other users on the same system can't snoop
  /// on data being served through this server.
  final _secret = Uri.encodeComponent(randomBase64(24));

  /// The URL for this server.
  Uri get url => _server.url.resolve('$_secret/');

  /// A [OneOffHandler] for servicing WebSocket connections for
  /// [BrowserManager]s.
  ///
  /// This is one-off because each [BrowserManager] can only connect to a single
  /// WebSocket,
  final _webSocketHandler = OneOffHandler();

  /// A [PathHandler] used to serve compiled WASM tests.
  final _wasmHandler = PathHandler();

  /// The [WasmCompilerPool] managing active instances of `dart2wasm`.
  final _compilers = WasmCompilerPool();

  /// The temporary directory in which compiled WASM is emitted.
  final _compiledDir = createTempDir();

  /// The root directory served statically by this server.
  final String _root;

  /// Whether [close] has been called.
  bool get _closed => _closeMemo.hasRun;

  /// A map from browser identifiers to futures that will complete to the
  /// [BrowserManager]s for those browsers, or `null` if they failed to load.
  ///
  /// This should only be accessed through [_browserManagerFor].
  final _browserManagers = <Runtime, Future<BrowserManager?>>{};

  /// Settings for invoking each browser.
  ///
  /// This starts out with the default settings, which may be overridden by user settings.
  final _browserSettings =
      Map<Runtime, ExecutableSettings>.from(defaultSettings);

  /// A map from test suite paths to Futures that will complete once those
  /// suites are finished compiling.
  ///
  /// This is used to make sure that a given test suite is only compiled once
  /// per run, rather than once per browser per run.
  final _compileFutures = <String, Future<void>>{};

  /// The default template for html tests.
  final String _defaultTemplatePath;

  /// The `package:test` side wrapper for the Dart2Wasm runtime.
  final String _jsRuntimeWrapper;

  BrowserWasmPlatform._(this._server, Configuration config, String faviconPath,
      this._defaultTemplatePath, this._jsRuntimeWrapper,
      {String? root})
      : _config = config,
        _root = root ?? p.current {
    var cascade = shelf.Cascade().add(_webSocketHandler.handler);

    if (_config.pubServeUrl != null) {
      throw UnsupportedError(
          'WASM browser tests don\'t support the `--pub-serve` argument');
    }

    cascade = cascade
        .add(packagesDirHandler())
        .add(_wasmHandler.handler)
        .add(createStaticHandler(_root))
        .add(_wrapperHandler);

    var pipeline = shelf.Pipeline()
        .addMiddleware(PathHandler.nestedIn(_secret))
        .addHandler(cascade.handler);

    _server.mount(shelf.Cascade()
        .add(createFileHandler(faviconPath))
        .add(pipeline)
        .handler);
  }

  /// A handler that serves wrapper files used to bootstrap tests.
  shelf.Response _wrapperHandler(shelf.Request request) {
    var path = p.fromUri(request.url);

    if (path.endsWith('.html')) {
      var test = '${p.withoutExtension(path)}.dart';
      var scriptBase = htmlEscape.convert(p.basename(test));
      var link = '<link rel="x-dart-test" href="$scriptBase">';
      var testName = htmlEscape.convert(test);
      var template = _config.customHtmlTemplatePath ?? _defaultTemplatePath;
      var contents = File(template).readAsStringSync();
      var jsRuntime = 'dart2wasm_runtime.mjs';
      var processedContents = contents
          // Checked during loading phase that there is only one {{testScript}} placeholder.
          .replaceFirst('{{testScript}}', link)
          .replaceFirst('{{jsRuntimeUrl}}', jsRuntime)
          .replaceFirst(
              '{{wasmUrl}}', p.basename('$test.browser_test.dart.wasm'))
          .replaceAll('{{testName}}', testName);
      return shelf.Response.ok(processedContents,
          headers: {'Content-Type': 'text/html'});
    }

    return shelf.Response.notFound('Not found.');
  }

  @override
  ExecutableSettings parsePlatformSettings(YamlMap settings) =>
      ExecutableSettings.parse(settings);

  @override
  ExecutableSettings mergePlatformSettings(
          ExecutableSettings settings1, ExecutableSettings settings2) =>
      settings1.merge(settings2);

  @override
  void customizePlatform(Runtime runtime, ExecutableSettings settings) {
    var oldSettings =
        _browserSettings[runtime] ?? _browserSettings[runtime.root];
    if (oldSettings != null) settings = oldSettings.merge(settings);
    _browserSettings[runtime] = settings;
  }

  /// Loads the test suite at [path] on the platform [platform].
  ///
  /// This will start a browser to load the suite if one isn't already running.
  /// Throws an [ArgumentError] if `platform.platform` isn't a browser.
  @override
  Future<RunnerSuite?> load(String path, SuitePlatform platform,
      SuiteConfiguration suiteConfig, Map<String, Object?> message) async {
    if (suiteConfig.precompiledPath != null) {
      throw UnsupportedError(
          'The wasm platform doesn\'t support precompiled suites');
    }

    var browser = platform.runtime;
    assert(suiteConfig.runtimes.contains(browser.identifier));

    if (!browser.isBrowser) {
      throw ArgumentError('$browser is not a browser.');
    }

    // TODO: Support custom html?

    Uri suiteUrl;
    await _compileSuite(path, suiteConfig);

    if (_closed) return null;
    suiteUrl = url.resolveUri(
        p.toUri('${p.withoutExtension(p.relative(path, from: _root))}.html'));

    if (_closed) return null;

    var browserManager = await _browserManagerFor(browser);
    if (_closed || browserManager == null) return null;

    var suite = await browserManager.load(path, suiteUrl, suiteConfig, message);
    if (_closed) return null;
    return suite;
  }

  /// Compile the test suite at [dartPath] to WASM.
  ///
  /// Once the suite has been compiled, it's added to [_wasmHandler] so it can be
  /// served.
  Future<void> _compileSuite(String dartPath, SuiteConfiguration suiteConfig) {
    return _compileFutures.putIfAbsent(dartPath, () async {
      var dir = Directory(_compiledDir).createTempSync('test_').path;

      var baseCompiledPath =
          p.join(dir, '${p.basename(dartPath)}.browser_test.dart');
      var baseUrl =
          '${p.toUri(p.relative(dartPath, from: _root)).path}.browser_test.dart';
      var wasmUrl = '$baseUrl.wasm';
      var jsRuntimeWrapperUrl = '$baseUrl.js';
      var jsRuntimeUrl = p.join(p.dirname(dartPath), 'dart2wasm_runtime.mjs');
      var htmlUrl = '$baseUrl.html';

      var bootstrapContent = '''
        ${suiteConfig.metadata.languageVersionComment ?? await rootPackageLanguageVersionComment}
        import "package:test/src/bootstrap/browser.dart";

        import "${p.toUri(p.absolute(dartPath))}" as test;

        void main() {
          internalBootstrapBrowserTest(() => test.main);
        }
      ''';

      await _compilers.compile(bootstrapContent, baseCompiledPath, suiteConfig);
      if (_closed) return;

      var wasmPath = '$baseCompiledPath.wasm';
      _wasmHandler.add(wasmUrl, (request) {
        return shelf.Response.ok(File(wasmPath).readAsBytesSync(),
            headers: {'Content-Type': 'application/wasm'});
      });

      _wasmHandler.add(jsRuntimeWrapperUrl, (request) {
        return shelf.Response.ok(File(_jsRuntimeWrapper).readAsBytesSync(),
            headers: {'Content-Type': 'application/javascript'});
      });

      var jsRuntimePath = p.join(dir, 'dart2wasm_runtime.mjs');
      _wasmHandler.add(jsRuntimeUrl, (request) {
        return shelf.Response.ok(File(jsRuntimePath).readAsBytesSync(),
            headers: {'Content-Type': 'application/javascript'});
      });

      var htmlPath = '$baseCompiledPath.html';
      _wasmHandler.add(htmlUrl, (request) {
        return shelf.Response.ok(File(htmlPath).readAsBytesSync(),
            headers: {'Content-Type': 'text/html'});
      });
    });
  }

  /// Returns the [BrowserManager] for [browser].
  ///
  /// If no browser manager is running yet, starts one.
  ///
  /// TODO: Share a browser manager with the regular browser platform.
  Future<BrowserManager?> _browserManagerFor(Runtime browser) {
    var managerFuture = _browserManagers[browser];
    if (managerFuture != null) return managerFuture;

    var completer = Completer<WebSocketChannel>.sync();
    var path = _webSocketHandler.create(webSocketHandler(completer.complete));
    var webSocketUrl = url.replace(scheme: 'ws').resolve(path);
    var hostUrl = url
        .resolve('packages/test/src/runner/browser/static/index.html')
        .replace(queryParameters: {
      'managerUrl': webSocketUrl.toString(),
      'debug': _config.debug.toString()
    });

    var future = BrowserManager.start(browser, hostUrl, completer.future,
            _browserSettings[browser]!, _config)
        .onError((error, _) {
      throw StateError('Unable to spawn Chrome Beta, which is required by the '
          'experimental-chrome-wasm platform. You may also need to set the '
          'executable path in your dart_test.yaml file, documented here: '
          'https://github.com/dart-lang/test/blob/master/pkgs/test/doc/configuration.md#override_platforms'
          '\n\n$error\n');
    });

    // Store null values for browsers that error out so we know not to load them
    // again.
    _browserManagers[browser] =
        future.then<BrowserManager?>((value) => value).onError((_, __) => null);

    return future;
  }

  /// Close all the browsers that the server currently has open.
  ///
  /// Note that this doesn't close the server itself. Browser tests can still be
  /// loaded, they'll just spawn new browsers.
  @override
  Future<List<void>> closeEphemeral() {
    var managers = _browserManagers.values.toList();
    _browserManagers.clear();
    return Future.wait(managers.map((manager) async {
      var result = await manager;
      if (result == null) return;
      await result.close();
    }));
  }

  /// Closes the server and releases all its resources.
  ///
  /// Returns a [Future] that completes once the server is closed and its
  /// resources have been fully released.
  @override
  Future<void> close() async => _closeMemo.runOnce(() async {
        await Future.wait([
          for (var browser in _browserManagers.values)
            browser.then((b) => b?.close()),
          _server.close(),
          _compilers.close(),
        ]);

        Directory(_compiledDir).deleteSync(recursive: true);
      });
  final _closeMemo = AsyncMemoizer<void>();
}
