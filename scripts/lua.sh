#!/bin/bash

set -eu

ROOT_DIR=$(cd $(dirname "$0")/..; pwd)

. $ROOT_DIR/env.sh ${ARCH}

rm -rf $ROOT_DIR/libmpv/lua_build
cp -rf $ROOT_DIR/libmpv/lua $ROOT_DIR/libmpv/lua_build
pushd $ROOT_DIR/libmpv/lua_build

if [ "$1" == "build" ]; then
	echo -e "\nBuilding lua..."
elif [ "$1" == "clean" ]; then
	make clean
	exit 0
else
	exit 1
fi

# LUA_T= and LUAC_T= to disable building lua & luac
make CC="$CC" AR="$AR rcu" RANLIB="$RANLIB" PLAT=linux LUA_T= LUAC_T= -j$CORES

# Cheat make on macOS to copy makefile itself to bin instead of install lua & luac
make INSTALL_TOP=$DEST TO_BIN="Makefile" install

make INSTALL_TOP=$DEST pc > $DEST/lib/pkgconfig/lua.pc
cat >> $DEST/lib/pkgconfig/lua.pc <<'EOF'

Name: Lua
Description:
Version: ${version}
Libs: -L${libdir} -llua
Cflags: -I${includedir}
EOF

popd
