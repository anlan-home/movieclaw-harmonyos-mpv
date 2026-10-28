#!/bin/bash

set -exu

./download.sh
./patch.sh

export ARCH=amd64
mkdir -p ./libmpv/amd64-build
cp ./crossfiles/amd64-crossfile-linux.ini ./libmpv/crossfile.ini
./build.sh amd64

(
    cd ./libmpv/amd64-build
    zip libmpv_x86_64.zip libmpv.so
)

export ARCH=arm64
mkdir -p ./libmpv/arm64-build
cp ./crossfiles/arm64-crossfile-linux.ini ./libmpv/crossfile.ini
./build.sh arm64

(
    cd ./libmpv/arm64-build
    zip libmpv_aarch64.zip libmpv.so
)
