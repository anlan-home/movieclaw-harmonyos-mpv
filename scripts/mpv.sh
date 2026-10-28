#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/mpv_build
cp -rf $ROOT_DIR/libmpv/mpv $ROOT_DIR/libmpv/mpv_build
pushd $ROOT_DIR/libmpv/mpv_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding mpv..."
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
  --prefix=$DEST/mpv \
  --default-library shared \
  -Dopensles=disabled \
  -Dohos=enabled \
  -Degl-ohos=enabled \
  -Dvulkan=disabled \
  -Dshaderc=disabled \
  -Dlua=enabled \
  -Dgpl=false \
  -Dbuild-date=false \
  -Dcplayer=false \
  -Dbuildtype=debug 
ninja -j$CORES
ninja install

cd $DEST/mpv/lib
mv libmpv.so ../../libmpv.so

popd
