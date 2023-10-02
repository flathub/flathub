 
mkdir -p /app/etc /app/bin /app/lib/mono/4.5 /app/lib/mono/4.8-api /app/lib/mono/gac
cp /usr/lib/sdk/mono6/bin/mono /app/bin/mono
cp /usr/lib/sdk/mono6/lib/libMonoPosixHelper.so /app/lib/
cp /usr/lib/sdk/mono6/lib/libMonoSupportW.so /app/lib/
cp /usr/lib/sdk/mono6/lib/libmono-btls-shared.so /app/lib/
cp -d /usr/lib/sdk/mono6/lib/libmono-native.so* /app/lib/
cp /usr/lib/sdk/mono6/lib/mono/4.5/*.dll* /app/lib/mono/4.5/
cp /usr/lib/sdk/mono6/lib/mono/4.8-api/*.dll* /app/lib/mono/4.8-api/
rm -f /app/lib/mono/4.5/Microsoft.CodeAnalysis*
rm -f /app/lib/mono/4.8-api/Microsoft.CodeAnalysis*
cp -ar /usr/lib/sdk/mono6/etc/mono /app/etc
cp -ar /usr/lib/sdk/mono6/lib/mono/gac/* /app/lib/mono/gac/
rm -f /app/lib/mono/gac/*/*/*.pdb
