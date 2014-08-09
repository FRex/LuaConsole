/* 
 * File:   LuaHeader.hpp
 * Author: frex
 *
 * Created on August 9, 2014, 6:35 PM
 */

#ifndef LUAHEADER_HPP
#define	LUAHEADER_HPP

#include <lua.hpp>
#include <cstddef>
#include <cstring>

//this header is to allow both lua jit (based on 5.1), 5.1 and 5.2 be
//used with the console by defining some very minor things

namespace lua {

//------------------------------------------------------------------------------
//LUA 5.2 ----------------------------------------------------------------------

#if (LUA_VERSION_NUM == 502)

inline bool incompleteChunkError(const char * err, std::size_t len)
{
    return 0 == std::strcmp(err + len - 5u, "<eof>");
}

#endif //LUA 5.2

//------------------------------------------------------------------------------
//LUA JIT AND 5.1 --------------------------------------------------------------
#if (LUA_VERSION_NUM == 501)

inline bool incompleteChunkError(const char * err, std::size_t len)
{
    return 0 == std::strcmp(err + len - 7u, "'<eof>'");
}

//pushglobaltable is missing but easy to fake via old method of getting globals:
#define lua_pushglobaltable(L) (lua_pushvalue((L), LUA_GLOBALSINDEX))

//LUA_OK is missing but 0 is assumed to be 'success' value in comments, so:
#define LUA_OK 0

#endif //LUA 5.1

} //lua

#endif	/* LUAHEADER_HPP */

