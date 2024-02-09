#!/usr/bin/env bash
mkdir -p .build/{checkouts,repositories}
cd .build/repositories || exit 1
mkdir -p CollectionConcurrencyKit-9b263e6b CryptoSwift-72c2bbc7 SourceKitten-45a013ca swift-argument-parser-59ba1edd swift-syntax-cb9339b1 SwiftyTextTable-dce42391 SWXMLHash-0bda306e Yams-00fc82b0
cd ..
cp -R ../{CollectionConcurrencyKit,CryptoSwift,SourceKitten,swift-argument-parser,swift-syntax,SwiftyTextTable,SWXMLHash,Yams} checkouts
cd checkouts || exit 1
cp -Rv CollectionConcurrencyKit/.git/* ../repositories/CollectionConcurrencyKit-9b263e6b
cp -Rv CryptoSwift/.git/* ../repositories/CryptoSwift-72c2bbc7
cp -Rv SourceKitten/.git/* ../repositories/SourceKitten-45a013ca
cp -Rv swift-argument-parser/.git/* ../repositories/swift-argument-parser-59ba1edd
cp -Rv swift-syntax/.git/* ../repositories/swift-syntax-cb9339b1
cp -Rv SwiftyTextTable/.git/* ../repositories/SwiftyTextTable-dce42391
cp -Rv SWXMLHash/.git/* ../repositories/SWXMLHash-0bda306e
cp -Rv Yams/.git/* ../repositories/Yams-00fc82b0
cd ../..
patch Package.swift swiftlint.patch || exit
