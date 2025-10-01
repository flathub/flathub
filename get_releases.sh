#!/usr/bin/env bash
export BASE_URL=https://www.virtualbox.org/wiki/Download_Old_Builds
export CHANGELOG_BASE_URL=https://www.virtualbox.org/wiki/Changelog

all_releases() {
  curl -s -q $BASE_URL | \
    grep -E -e "<li><a.*Download_Old_Builds.*>" | \
    grep -oP "\d\.\d" | \
    xargs -0 | while IFS= read -r line ; do
      VERSION="$(curl -s -q "${BASE_URL}_$(tr . _ <<< "${line}")" | \
        grep -E -e "<a.*/wiki/Changelog.*>" | \
        grep -oP "VirtualBox \d.\d.\d" | \
        sed "s/VirtualBox //" | head -n 1)"

      DATE_RAW="$(curl -s -q "${BASE_URL}_$(tr . _ <<< "${line}")" | \
        grep -E "VirtualBox [[:digit:]]\.[[:digit:]]\.[[:digit:]]" | \
        grep -oP "released .*\)" | \
        sed "s/released //" | tr -d "\)" | head -n 1)"

      DATE="$(date -d "$(sed 's/nd// ; s/th//' <<< "${DATE_RAW}")" "+%Y-%m-%d")"

      printf " <release version=\"%s\" date=\"%s\"/>\n  <url type=\"details\">${CHANGELOG_BASE_URL}-${line}#v0</url>\n </release>\n" "${VERSION}" "${DATE}"
    done
}
all_releases
