# Dotnet 7 SDK extension

## How to use
You need to add following lines to flatpak manifest:
```json
"sdk-extensions": [
    "org.freedesktop.Sdk.Extension.dotnet7"
],
"build-options": {
    "append-path": "/usr/lib/sdk/dotnet7/bin",
    "append-ld-library-path": "/usr/lib/sdk/dotnet7/lib",
    "env": {
        "PKG_CONFIG_PATH": "/app/lib/pkgconfig:/app/share/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:/usr/lib/sdk/dotnet7/lib/pkgconfig"
    }
},
```

###  Scripts
* `install.sh` - copies dotnet runtime to package.
* `install-sdk.sh` - copies dotnet SDK to package.

### Publishing project

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net7.0/publish/ /app/bin/",
]
```

### Using nuget packages
If you want to use nuget packages it is recommended to use the [Flatpak .NET Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/dotnet) tool. It generates sources file that can be included in manifest.

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release --source ./nuget-sources YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net7.0/publish/ /app/bin/"
],
"sources": [
    "sources.json",
    "..."
]
```

### Publishing self contained app and trimmed binaries
Dotnet 7 gives option to include runtime in published application and trim their binaries. This allows you to significantly reduce the size of the package and get rid of `/usr/lib/sdk/dotnet7/bin/install.sh`. 

First you need to have following lines in `sources.json`. These packages are needed to build a project for specific runtime. 

```json
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm/7.0.0/microsoft.aspnetcore.app.runtime.linux-arm.7.0.0.nupkg",
    "sha512": "fe68af4fac5062db9b9be6a298a577f3a1cc70e218d05d1e98ea47fa2536e10977033d38c995daf90da2035bbe275c2e2c163ff639ee7adfaf094fd166a39933",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-arm.7.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm64/7.0.0/microsoft.aspnetcore.app.runtime.linux-arm64.7.0.0.nupkg",
    "sha512": "1c3ea618c23823c54f6fa82799b8743c4e919e1fa1196d18402e547faaafb52a4d6e2131ddef6092dc71a308e8f7bcd3feac825565cec112705dacf88b1f7e57",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-arm64.7.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-x64/7.0.0/microsoft.aspnetcore.app.runtime.linux-x64.7.0.0.nupkg",
    "sha512": "285817dc47116766e8f3279c0c6b7add76e1c362ed455b7d03790874bb4fab70f0507a31747a109a8afa3b80215b9c463a3d245e80ba14ee896114ef44f8892d",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-x64.7.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm/7.0.0/microsoft.netcore.app.runtime.linux-arm.7.0.0.nupkg",
    "sha512": "7860233cc8030f239a9a8fe3c5963cd19efc4b8fe25527230e84491b1c7b5d2ddfa6ad85f494c711f50a84bc1b4d9c6a6196ffce8df757b6487e592fa00640c2",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-arm.7.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm64/7.0.0/microsoft.netcore.app.runtime.linux-arm64.7.0.0.nupkg",
    "sha512": "b0970c968b230486964549a6665d90c69c8e38f44471f02d83ecf2bba1e76c162bd383abffbf2779b515413486ceba0507d234f41b690d961a5975c41ce6f437",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-arm64.7.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-x64/7.0.0/microsoft.netcore.app.runtime.linux-x64.7.0.0.nupkg",
    "sha512": "0d2c0cd4a669b753abebb2edaf65b9fd277836b029d6f945bc95f98ed487221b462f9a82dc199dd08c5b12f1f15134531e208fcad5931c426f9fdba6ab068786",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-x64.7.0.0.nupkg"
},
```

Then add build options:

```json
"build-options": {
    "arch": {
        "arm": {
            "env" : {
                "RUNTIME": "linux-arm"
            }
        },
        "aarch64": {
            "env" : {
                "RUNTIME": "linux-arm64"
            }
        },
        "x86_64": {
            "env" : {
                "RUNTIME": "linux-x64"
            }
        }
    }
},
"build-commands": [
    "mkdir -p /app/bin",
    "dotnet publish -c Release --source ./nuget-sources YourProject.csproj --runtime $RUNTIME --self-contained true",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net7.0/$RUNTIME/publish/* /app/bin/",
],
```
