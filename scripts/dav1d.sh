#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/dav1d_build
cp -rf $ROOT_DIR/libmpv/dav1d $ROOT_DIR/libmpv/dav1d_build
pushd $ROOT_DIR/libmpv/dav1d_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding dav1d..."
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
  -Db_lto=true \
  -Denable_tests=false \
  -Denable_tools=false
ninja -j$CORES
ninja install

popd
