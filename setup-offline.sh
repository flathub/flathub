#!/usr/bin/env bash
mkdir .build/repositories
cd .build/repositories
mkdir ./swift-argument-parser-54a11a8d
cp -r ../checkouts/swift-argument-parser/.git/* ./swift-argument-parser-54a11a8d
mkdir ./swift-cmark-7ac81116
cp -r ../checkouts/swift-cmark/.git/* ./swift-cmark-7ac81116
mkdir ./swift-markdown-6b045e7c
cp -r ../checkouts/swift-markdown/.git/* ./swift-markdown-6b045e7c
mkdir ./swift-syntax-e1f983d3
cp -r ../checkouts/swift-syntax/.git/* ./swift-syntax-e1f983d3