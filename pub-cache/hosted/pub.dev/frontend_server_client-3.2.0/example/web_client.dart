// Copyright 2020 The Dart Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';
import 'dart:io';

import 'package:async/async.dart';
import 'package:frontend_server_client/frontend_server_client.dart';
import 'package:path/path.dart' as p;
import 'package:shelf/shelf.dart';
import 'package:shelf/shelf_io.dart' as shelf_io;
import 'package:shelf_packages_handler/shelf_packages_handler.dart';
import 'package:shelf_static/shelf_static.dart';

void main(List<String> args) async {
  try {
    watch.start();
    if (args.isNotEmpty) {
      throw ArgumentError('No command line args are supported');
    }

    _print('compiling the dart sdk');
    var sdkCompileResult = await Process.run(Platform.resolvedExecutable, [
      p.join(sdkDir, 'bin', 'snapshots', 'dartdevc.dart.snapshot'),
      '--multi-root-scheme=org-dartlang-sdk',
      '--modules=amd',
      '--module-name=dart_sdk',
      '--sound-null-safety',
      '-o',
      dartSdkJs,
      p.url.join(sdkDir, sdkKernelPath),
    ]);
    if (sdkCompileResult.exitCode != 0) {
      _print('Failed to compile the dart sdk to JS:\n'
          '${sdkCompileResult.stdout}\n'
          '${sdkCompileResult.stderr}');
      exit(sdkCompileResult.exitCode);
    }

    _print('starting frontend server');
    var client = await DartDevcFrontendServerClient.start(
        'org-dartlang-root:///$app', outputDill,
        fileSystemRoots: [p.current],
        fileSystemScheme: 'org-dartlang-root',
        platformKernel: p.toUri(sdkKernelPath).toString(),
        verbose: true);

    _print('compiling $app');
    await client.compile([]);
    client.accept();
    _print('done compiling $app');

    _print('starting shelf server');
    var cascade = Cascade()
        .add(_clientHandler(client))
        .add(createStaticHandler(p.current))
        .add(createFileHandler(dartSdkJs, url: 'example/app/dart_sdk.js'))
        .add(createFileHandler(
            p.join(sdkDir, 'lib', 'dev_compiler', 'web',
                'dart_stack_trace_mapper.js'),
            url: 'example/app/dart_stack_trace_mapper.js'))
        .add(createFileHandler(
            p.join(
                sdkDir, 'lib', 'dev_compiler', 'kernel', 'amd', 'require.js'),
            url: 'example/app/require.js'))
        .add(packagesDirHandler());
    final server = await shelf_io.serve(cascade.handler, 'localhost', 8080);
    _print('server ready');

    // The file we will be editing in the repl
    var appFile = File(app);
    var originalContent = await appFile.readAsString();
    var appLines = const LineSplitter().convert(originalContent);
    var getterText = 'String get message =>';
    var messageLine =
        appLines.indexWhere((line) => line.startsWith(getterText));

    var stdinQueue = StreamQueue(
        stdin.transform(utf8.decoder).transform(const LineSplitter()));
    _prompt();
    while (await stdinQueue.hasNext) {
      var newMessage = await stdinQueue.next;
      if (newMessage == 'quit') {
        await server.close();
        await stdinQueue.cancel();
        break;
      } else if (newMessage == 'reset') {
        print('resetting');
        client.reset();
        _print('restoring $app');
        await appFile.writeAsString(originalContent);
      } else {
        _print('editing $app');
        appLines[messageLine] = '$getterText "$newMessage";';
        var newContent = appLines.join('\n');
        await appFile.writeAsString(newContent);

        _print('recompiling $app with edits');
        var result =
            await client.compile([Uri.parse('org-dartlang-root:///$app')]);
        if (result.errorCount > 0) {
          print('Compile errors: \n${result.compilerOutputLines.join('\n')}');
          await client.reject();
        } else {
          _print('Recompile succeeded for $app');
          client.accept();
          // TODO: support hot restart
          print('reload app to see the new message');
        }
      }

      _prompt();
    }

    _print('restoring $app');
    await appFile.writeAsString(originalContent);
    _print('exiting');
    await client.shutdown();
  } finally {
    Directory(p.join('.dart_tool', 'out')).deleteSync(recursive: true);
  }
}

Handler _clientHandler(DartDevcFrontendServerClient client) {
  return (Request request) {
    var path = request.url.path;
    var packagesIndex = path.indexOf('/packages/');
    if (packagesIndex > 0) {
      path = request.url.path.substring(packagesIndex);
    } else {
      path = request.url.path;
    }
    if (!path.startsWith('/')) path = '/$path';
    if (path.endsWith('.dart.js') && path != '/example/app/main.dart.js') {
      path = path.replaceFirst('.dart.js', '.dart.lib.js', path.length - 8);
    }
    var assetBytes = client.assetBytes(path);
    if (assetBytes == null) return Response.notFound('path not found');
    return Response.ok(assetBytes,
        headers: {HttpHeaders.contentTypeHeader: 'application/javascript'});
  };
}

void _print(String message) {
  print('${watch.elapsed}: $message');
}

void _prompt() => stdout.write(
    'Enter a new message to print and recompile, or type `quit` to exit:');

final app = 'example/app/main.dart';
final dartSdkJs = p.join('.dart_tool', 'out', 'dart_sdk.js');
final outputDill = p.join('.dart_tool', 'out', 'example_app.dill');
final sdkDir = p.dirname(p.dirname(Platform.resolvedExecutable));
final sdkKernelPath =
    p.join(sdkDir, 'lib', '_internal', 'ddc_platform_sound.dill');
final watch = Stopwatch();
