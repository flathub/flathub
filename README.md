# SDK Extension for LLVM Project 18

This extension contains various components of the [LLVM Project](https://llvm.org) (version 18.x).

Among provided tools there are LLVM Core libraries, Clang, Clang Extra Tools, LLDB and LLD.

## Usage

### Build

In order to build your app with tools provided with this extension you have to set following variables in app manifest:

```
"sdk-extensions" : [ "org.freedesktop.Sdk.Extension.llvm18" ],
...
"build-options":{
    "append-path": "/usr/lib/sdk/llvm18/bin",
    "prepend-ld-library-path": "/usr/lib/sdk/llvm18/lib"
    }
```
Example:
```
{
  "id" : "org.example.MyApp",
  "runtime" : "org.freedesktop.Platform",
  "runtime-version" : "24.08",
  "sdk" : "org.freedesktop.Sdk",
  "sdk-extensions" : [ "org.freedesktop.Sdk.Extension.llvm18" ],
  "modules" : [
  {
    "name" : "Myapp",
    "build-options":{
    "append-path": "/usr/lib/sdk/llvm18/bin",
    "prepend-ld-library-path": "/usr/lib/sdk/llvm18/lib"
  }
 ]
}
```

#### Troubleshooting:

It's possible that your app will additionally need some special variable(s) beside the above. Please consult your app documentation or source files like `CMakeLists.txt` to confirm that.

If your app dynamically links to any shared library provided by this extension then you need to copy that library in `/app/lib` directory during the build, i.e:
```
cp /usr/lib/sdk/llvm18/lib/libLLVM-18.so /app/lib/
```

### Debugging/Development

In order to use this extension in flatpak SDK environment you may add all provided tools in your PATH by executing first:
```
source /usr/lib/sdk/llvm18/enable.sh
```
