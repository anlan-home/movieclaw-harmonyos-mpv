#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh 

rm -rf $ROOT_DIR/libmpv/mbedtls_build
cp -rf $ROOT_DIR/libmpv/mbedtls $ROOT_DIR/libmpv/mbedtls_build
pushd $ROOT_DIR/libmpv/mbedtls_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding mbedtls..."
elif [ "$1" == "clean" ]; then
	make clean
	exit 0
else
	exit 1
fi

python3 -m venv .venv
source .venv/bin/activate
pip install -r scripts/basic.requirements.txt

make -j$CORES no_test
make DESTDIR=$DEST install

popd
