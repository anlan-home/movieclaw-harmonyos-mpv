#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/ffmpeg_build
cp -rf $ROOT_DIR/libmpv/ffmpeg $ROOT_DIR/libmpv/ffmpeg_build
pushd $ROOT_DIR/libmpv/ffmpeg_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding FFmpeg..."
elif [ "$1" == "clean" ]; then
	make distclean
	exit 0
else
	exit 1
fi
buildArch=""
if [[ ${ARCH} == "amd64" ]];then
    buildArch="x86_64"
else
    buildArch="aarch64"
fi

./configure \
  --prefix=$DEST \
  --arch=${buildArch} \
  --target-os=linux \
  --enable-static \
  --disable-shared \
  --enable-version3 \
  --enable-pic \
  --disable-doc \
  --disable-programs \
  \
  --enable-cross-compile \
  --cc="$CC" \
  --extra-cflags="-I$DEST/include" \
  --extra-ldflags="-L$DEST/lib" \
  --enable-libdav1d \
  --enable-mbedtls \
  --disable-vulkan \
  \
  --disable-devices \
  --disable-avdevice \
  --disable-muxers \
  --disable-encoders \
  --enable-ohcodec \
  --enable-encoder=png,mjpeg
make -j$CORES
make install
popd
