import os

godot_path = './godot-4.0-alpha16'
source_folder = './sources'
csprojs = [
    'modules/mono/glue/GodotSharp/GodotSharp/GodotSharp.csproj',
    'modules/mono/glue/GodotSharp/Godot.SourceGenerators.Internal/Godot.SourceGenerators.Internal.csproj',
    'modules/mono/editor/GodotTools/GodotTools/GodotTools.csproj',
    'modules/mono/editor/GodotTools/GodotTools.BuildLogger/GodotTools.BuildLogger.csproj',
    'modules/mono/editor/GodotTools/GodotTools.IdeMessaging/GodotTools.IdeMessaging.csproj',
    'modules/mono/editor/GodotTools/GodotTools.IdeMessaging.CLI/GodotTools.IdeMessaging.CLI.csproj',
    'modules/mono/editor/GodotTools/GodotTools.OpenVisualStudio/GodotTools.OpenVisualStudio.csproj',
    'modules/mono/editor/GodotTools/GodotTools.ProjectEditor/GodotTools.ProjectEditor.csproj',
    'modules/mono/editor/Godot.NET.Sdk/Godot.NET.Sdk/Godot.NET.Sdk.csproj',
    'modules/mono/editor/Godot.NET.Sdk/Godot.SourceGenerators/Godot.SourceGenerators.csproj'
]

def main():
    for csproj in csprojs:
        csproj_path = godot_path+'/'+csproj
        source_name=source_folder + csproj[csproj.rindex('/'):-6]+'json'
        os.system("python3 flatpak-dotnet-generator.py " + source_name + ' ' + csproj_path)

if __name__ == "__main__":
    main()

