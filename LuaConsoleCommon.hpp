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

//constants to move by to get to end or start of prompt line
const int kCursorHome = -100000;
const int kCursorEnd = 100000;

//names of files are hardcoded in view and model .cpp files

}

#endif	/* LUACONSOLECOMMON_HPP */

