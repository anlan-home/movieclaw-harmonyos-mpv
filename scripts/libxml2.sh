#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/libxml2_build
cp -rf $ROOT_DIR/libmpv/libxml2 $ROOT_DIR/libmpv/libxml2_build
pushd $ROOT_DIR/libmpv/libxml2_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding libxml2..."
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
  -Ddocs=disabled \
  -Dzlib=enabled \
  -Dsax1=enabled
ninja -j$CORES
ninja install

popd
