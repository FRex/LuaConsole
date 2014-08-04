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

class LuaConsole;
class LuaConsoleModel;
class LuaConsoleView;

const int kCursorHome = -100000;
const int kCursorEnd = 100000;
const int kInnerWidth = 78;

}

#endif	/* LUACONSOLECOMMON_HPP */

