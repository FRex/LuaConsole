/* 
 * File:   LuaConsoleCommon.hpp
 * Author: frex
 *
 * Created on July 19, 2014, 11:38 PM
 */

#ifndef LUACONSOLECOMMON_HPP
#define	LUACONSOLECOMMON_HPP

#include <cassert>
#include <cstddef>

struct lua_State;

namespace lua {

const int kCursorHome = -100000;
const int kCursorEnd = 100000;
const int kInnerWidth = 78;

//names of files are hardcoded in view and model .cpp files

enum ECONSOLE_OPTIONS
{
    ECO_HISTORY = 1, //load and save history in plaintext file - luaconsolehistory.txt
    ECO_INIT = 2, //load init file - luaconsoleinit.lua

    //keep last:
    ECO_DEFAULT = 3, //do all of these helpful things above
    ECO_NONE = 0 //do none of the helpful things, ALL is up to user now
};

}

#endif	/* LUACONSOLECOMMON_HPP */

