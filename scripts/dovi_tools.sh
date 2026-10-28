#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/dovi_tools_build

cp -rf $ROOT_DIR/libmpv/dovi_tools $ROOT_DIR/libmpv/dovi_tools_build
pushd $ROOT_DIR/libmpv/dovi_tools_build/dolby_vision

if [ "$1" == "build" ]; then
	echo -e "\nBuilding dovi tools..."
elif [ "$1" == "clean" ]; then
	cargo clean
	exit 0
else
	exit 1
fi

if [[ ${ARCH} == "arm64" ]]; then
  export TARGET=aarch64-unknown-linux-ohos
else
  export TARGET=x86_64-unknown-linux-ohos
fi

cargo cinstall \
  --release \
  --target=${TARGET} \
  --library-type=staticlib \
  --prefix=$DEST \
  --libdir=lib

popd
