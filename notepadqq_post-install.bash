#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './notepadqq_setup.bash';

echo 'install symlink';
ln -sf '/app/lib/notepadqq/notepadqq-bin' '/app/bin/notepadqq';
echo;

echo 'install docs';
install -d "/app/share/doc/${app_name}";
install -p -m 0644 "CONTRIBUTING.md" "README.md" "/app/share/doc/${app_name}/";
echo;

echo 'install licenses';
install -d "/app/share/licenses/${app_name}";
install -p -m 0644 "COPYING" "/app/share/licenses/${app_name}/";
echo;

echo 'check desktop';
desktop-file-validate "/app/share/applications/${app_name}.desktop";
echo "status: ${?}";
echo;

echo 'check appdata';
appstream-util validate-relax --nonet "/app/share/metainfo/${app_name}.appdata.xml";
echo "status: ${?}";
echo;

