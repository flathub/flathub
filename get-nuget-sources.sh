# Requires the repo to be cloned to "repo" folder.

python3 flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py \
    --only-arches x86_64 \
    --dotnet 10 \
    --freedesktop 25.08 \
    ./nuget-sources.x86_64.json \
    ./repo/src/SourceGit.csproj \
    --dotnet-args -p:PublishAot=true -p:SelfContained=true -p:PublishTrimmed=true -p:PublishSingleFile=true -p:RuntimeIdentifier=linux-x64

python3 flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py \
    --only-arches aarch64 \
    --dotnet 10 \
    --freedesktop 25.08 \
    ./nuget-sources.aarch64.json \
    ./repo/src/SourceGit.csproj \
    --dotnet-args -p:PublishAot=true -p:SelfContained=true -p:PublishTrimmed=true -p:PublishSingleFile=true -p:RuntimeIdentifier=linux-arm64

jq -s '
  def dmerge($a; $b):
    if ($a|type) == "object" and ($b|type) == "object" then
      reduce (($a|keys_unsorted) + ($b|keys_unsorted) | unique)[] as $k
        ({}; .[$k] = dmerge($a[$k]; $b[$k]))
    elif ($a|type) == "array" and ($b|type) == "array" then
      $a + $b
    elif $b == null then
      $a
    else
      $b
    end;

  add
  | sort_by(.url)
  | group_by(.url)
  | map(reduce .[] as $o ({}; dmerge(.; $o)))
' nuget-sources.x86_64.json nuget-sources.aarch64.json > nuget-sources.json

rm nuget-sources.x86_64.json nuget-sources.aarch64.json
