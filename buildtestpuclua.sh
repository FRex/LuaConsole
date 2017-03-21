#!/bin/bash

LUAVERSIONS='
5.1.5
5.2.4
5.3.4
'

if [ $# -ne 1 ]
then
	echo "Launch in LuaConsole repo root dir to try to"
	echo "download PUC-Rio Lua and build console with it."
	echo "Usage: $0 luaversion"
	echo "Available versions: $LUAVERSIONS"
	exit 1
fi

PICKEDVER="$1"
echo "$LUAVERSIONS" | grep -qe "^${PICKEDVER}$"

if [ $? -ne 0 ]
then
	echo "No such version"
	echo "Available versions: $LUAVERSIONS"
	exit 2
fi

LUATGZURL="https://www.lua.org/ftp/lua-$PICKEDVER.tar.gz"
LUATGZFILE="lua-$PICKEDVER.tgz"
curl -s "$LUATGZURL" > "$LUATGZFILE"

if [ $? -ne 0 ]
then
	echo "Download failed: $LUATGZURL"
	exit 3
fi

echo "Download OK: $LUATGZURL"

LUADIR=$(tar --exclude '*/*' -tf "$LUATGZFILE")
rm -rf "$LUADIR"
tar -xf "$LUATGZFILE"
cd "$LUADIR"
make ansi -sj 3 || make c89 -sj 3 #5.3 renamed ansi to c89
echo "Lua make returned: $?"
echo '
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
' > 'src/lua.hpp'
cd ..

CONSOLEEXENAME="console-$PICKEDVER"
g++ examples/*.cpp -Iinclude -Isrc -I"${LUADIR}/src/" src/LuaConsole/*.cpp -lsfml-system -lsfml-window -lsfml-graphics "${LUADIR}/src/liblua.a" -o "$CONSOLEEXENAME"
echo "Buidling returned: $?"
