LuaConsole
==========

Simple Lua game command console that uses SFML
The size, font, limits on history size etc. and frame characters are hardcodded (for now).
The controls are as in a normal text line/editor:
up, down, left, right, home, end, backspace, enter, tab(completion)
At startup the console will load luaconsolehistory.txt and luaconsoleinit.lua
At shutdown the console will save history to luaconsolehistory.txt
