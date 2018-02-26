set -e
export PATH=$PATH:~/.bin
mv gulp-electron-cache /tmp/
mv .electron .bin ~/
chmod u+x ~/.bin/yarn.js
ln -s yarn.js ~/.bin/yarn
yarn config set yarn-offline-mirror "$(realpath ./yarn-mirror)"
tar -xzvf yarn-mirror/vscode-ripgrep-0.7.1-patch.0.1.tgz
mv package vscode-ripgrep-0.7.1-patch.0.1
pushd vscode-ripgrep-0.7.1-patch.0.1
    yarn link
    mkdir bin
    unzip ../misc/ripgrep-0.7.1-patch.0-linux-$(node -e 'console.log(process.arch)').zip rg -d bin/
    chmod 755 bin/rg
popd
tar -xzvf yarn-mirror/vscode-1.0.1.tgz
mv package vscode-1.0.1
pushd vscode-1.0.1
    yarn link
    echo > bin/install
    cp ../vscode/src/vs/vscode.d.ts .
popd
pushd vscode
    echo '[]' > build/builtInExtensions.json
    yarn link vscode-ripgrep
    pushd extensions
        rm -r vscode-api-tests vscode-colorize-tests
        pushd emmet
            yarn link vscode
        popd
    popd
    sed -i "s/'vscode\-api\-tests',//" build/gulpfile.vscode.js
    sed -i "s/'vscode\-colorize\-tests',//" build/gulpfile.vscode.js
    sed -i "s/'vscode\-api\-tests',//" build/npm/postinstall.js
    sed -i "s/'vscode\-colorize\-tests',//" build/npm/postinstall.js
    python2 /app/lib/node_modules/npm/node_modules/node-gyp/gyp/gyp_main.py --help || true
    npm_config_tarball="$(realpath ../misc/iojs-v1.7.9.tar.gz)" yarn install --offline --verbose --frozen-lockfile
    rm node_modules/vscode-ripgrep
    cp -r ../vscode-ripgrep-0.7.1-patch.0.1 node_modules/vscode-ripgrep
    echo "/// <reference types='@types/node'/>" > extensions/emmet/src/typings/refs.d.ts
    node_modules/.bin/gulp vscode-linux-$(node -e 'console.log(process.arch)')-min $([ $FLATPAK_ARCH == 'x86_64' ] && echo '--max_old_space_size=4096')
popd
mv VSCode-linux-$(node -e 'console.log(process.arch)') /app/
ln -s ../VSCode-linux-$(node -e 'console.log(process.arch)')/code-oss /app/bin/code-oss
