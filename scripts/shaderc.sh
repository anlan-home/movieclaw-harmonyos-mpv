#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)
echo ARCH$ARCH
. $ROOT_DIR/env.sh

rm -rf $ROOT_DIR/libmpv/shaderc_build
cp -rf $ROOT_DIR/libmpv/shaderc $ROOT_DIR/libmpv/shaderc_build
pushd $ROOT_DIR/libmpv/shaderc_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding shaderc..."
elif [ "$1" == "clean" ]; then
	rm -rf .build
	exit 0
else
	exit 1
fi

mkdir -p .build
cd .build

if [[ ${ARCH} == "arm64" ]]; then
  export OHOS_ARCH=arm64-v8a
else
  export OHOS_ARCH=x86_64
fi

cmake -L \
  -DCMAKE_TOOLCHAIN_FILE=$OHOS_NDK_HOME/native/build/cmake/ohos.toolchain.cmake \
  -DOHOS_ARCH=${OHOS_ARCH} \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX=$DEST \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SKIP_RPATH=TRUE \
  -DSHADERC_SKIP_TESTS=ON \
  -DSHADERC_SKIP_EXAMPLES=ON \
  -DSHADERC_SKIP_COPYRIGHT_CHECK=ON \
  -DSHADERC_ENABLE_WERROR_COMPILE=OFF \
  -DOHOS_STL=c++_shared \
  -GNinja \
  ..
ninja -j$CORES
ninja install

cd $DEST/lib/pkgconfig
mv shaderc.pc shaderc_shared.pc
mv shaderc_combined.pc shaderc.pc

popd
