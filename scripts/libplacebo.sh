#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}
rm -rf $ROOT_DIR/libmpv/libplacebo_build
cp -rf $ROOT_DIR/libmpv/libplacebo $ROOT_DIR/libmpv/libplacebo_build
pushd $ROOT_DIR/libmpv/libplacebo_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding libplacebo..."
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
  -Ddovi=enabled -Dlibdovi=disabled \
  -Dlcms=disabled \
  -Dshaderc=disabled \
  -Dvulkan=disabled \
  -Dopengl=enabled \
  -Ddemos=false
ninja -j$CORES
ninja install

popd
