#!/usr/bin/env bash
mkdir -p .build/{checkouts,repositories}
cd .build/repositories || exit 1
mkdir -p swift-argument-parser-59ba1edd swift-syntax-cb9339b1 swift-system-b9feb0b6 swift-tools-support-core-1e273aae swift-markdown-b692ce3c swift-cmark-4b3746f9
cd ..
cp -R ../{swift-argument-parser,swift-syntax,swift-system,swift-tools-support-core,swift-markdown,swift-cmark} checkouts
cd checkouts || exit 1
cp -Rv swift-argument-parser/.git/* ../repositories/swift-argument-parser-59ba1edd/
cp -Rv swift-syntax/.git/* ../repositories/swift-syntax-cb9339b1/
cp -Rv swift-system/.git/* ../repositories/swift-system-b9feb0b6/
cp -Rv swift-tools-support-core/.git/* ../repositories/swift-tools-support-core-1e273aae/
cp -Rv swift-markdown/.git/* ../repositories/swift-markdown-b692ce3c/
cp -Rv swift-cmark/.git/* ../repositories/swift-cmark-4b3746f9/
