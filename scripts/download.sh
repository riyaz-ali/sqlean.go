#!/bin/bash
set -euo pipefail

export UPSTREAM=${UPSTREAM:-"nalgeon/sqlean"}
export VERSION=${VERSION:-"0.21.6"}

TMP=$(mktemp -d)
echo "Work: $TMP"
curl -sL "https://github.com/$UPSTREAM/archive/refs/tags/$VERSION.tar.gz" -o $TMP/sqlean.tgz
tar -xzf $TMP/sqlean.tgz -C $TMP

mkdir -p $(pwd)/pkg/extensions
cp -R $TMP/sqlean-$VERSION/src/* pkg/extensions
rm -f pkg/extensions/*.{c,h}