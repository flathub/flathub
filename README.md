# Flatpak package for varnam

[Varnam project](https://www.varnamproject.com/)

Flatpak package for [libvarnam](https://github.com/varnamproject/libvarnam) & [varnam editor](https://gitlab.com/subins2000/varnam).

It also includes pre-learnt Malayalam corpus. So, installing it would instantly give a really good transliteration for Malayalam language.

Currently this varnam instance works for :

* Malayalam
* Hindi
* Tamil
* Telugu
* Bangla

## Requirements

```
sudo apt install flatpak-builder elfutils
```

## Build

A GPG sign may be included with `--gpg-sign=KEYID`

```
flatpak-builder --repo=repo build-dir com.varnamproject.Varnam.json --force-clean --gpg-sign=
flatpak build-bundle repo varnam.flatpak com.varnamproject.Varnam
```

This will make a package named `varnam.flatpak`.

Or to install as build finishes :

```
flatpak-builder --install build-dir com.varnamproject.Varnam.json --force-clean
```

## Install

To install it, simply do :

```
flatpak install varnam.flatpak
```

Pass `--user` to install in user directory instead.