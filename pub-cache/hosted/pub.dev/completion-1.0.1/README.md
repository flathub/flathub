*Add shell command completion to your Dart console applications.*

[![Build Status](https://github.com/kevmoo/completion.dart/workflows/ci/badge.svg?branch=master)](https://github.com/kevmoo/completion.dart/actions?query=workflow%3A"ci"+branch%3Amaster)
[![pub package](https://img.shields.io/pub/v/completion.svg)](https://pub.dev/packages/completion)
[![package publisher](https://img.shields.io/pub/publisher/completion.svg)](https://pub.dev/packages/completion/publisher)

To use this package, instead of this:

```dart
import 'package:args/args.dart';

void main(List<String> args) {
  final argParser = ArgParser()..addFlag('option', help: 'flag help');
  // ... add more options ...
  final argResults = argParser.parse(args);
  // ...
}
```

do this:

```dart
import 'package:args/args.dart';
import 'package:completion/completion.dart' as completion;

void main(List<String> args) {
  final argParser = ArgParser()..addFlag('option', help: 'flag help');
  // ... add more options ...
  final argResults = completion.tryArgsCompletion(args, argParser);
  // ...
}
```

(The only difference is calling `complete.tryArgsCompletion` in place of `argParser.parse`)

This will add a "completion" command to your app, which the shell will use
to complete arguments.

To generate the setup script automatically, call `generateCompletionScript`
with the names of the executables that your Dart script runs as (typically
just one, but it could be more).

Also, see [the example](./example).
