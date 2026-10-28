#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/fontconfig_build
cp -rf $ROOT_DIR/libmpv/fontconfig $ROOT_DIR/libmpv/fontconfig_build
pushd $ROOT_DIR/libmpv/fontconfig_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding fontconfig..."
elif [ "$1" == "clean" ]; then
	rm -rf .build
	exit 0
else
	exit 1
fi

mkdir -p .build
cd .build

meson setup .. \
  --cross-file $ROOT_DIR/libmpv/crossfile-${ARCH}.ini \
  --prefix=$DEST \
  -Ddoc=disabled \
  -Dtests=disabled \
  -Dcache-build=disabled \
  -Dxml-backend=libxml2 \
  -Ddefault-fonts-dirs=/system/fonts
ninja -j$CORES
ninja install

popd
