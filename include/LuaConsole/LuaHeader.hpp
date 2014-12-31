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
//NOTE:
//since we define things that might be defined (ie. equal, which is deprecated,
//do missing by default but may be present) we prepend bla_ to them
//this also lets code of model know which functions are 'common' between 51 and
//52 and which are 'patchwork' to allow this code sharing, also - this prevents
//getting in the way of/confusing the user if they use this header as well
//
//incompleteChunkError is exempt from that, since it's in our lua namespace,
//it's not a symbol from either of Lua versions and it ended up here just
//because it depends on the version of lua as well


//------------------------------------------------------------------------------
//LUA 5.2 ----------------------------------------------------------------------

#if (LUA_VERSION_NUM == 502)

namespace blua {

inline bool incompleteChunkError(const char * err, std::size_t len)
{
    return 0 == std::strcmp(err + len - 5u, "<eof>");
}

} //blua

//5.2 deprecated lua_equal so we use lua_compare to reimplement it:

inline int bla_lua_equal(lua_State * L, int index1, int index2)
{
    return lua_compare(L, index1, index2, LUA_OPEQ);
}

#define bla_lua_pushglobaltable lua_pushglobaltable

#define BLA_LUA_OK LUA_OK

#endif //LUA 5.2

//------------------------------------------------------------------------------
//LUA JIT AND 5.1 --------------------------------------------------------------
#if (LUA_VERSION_NUM == 501)

namespace blua {

inline bool incompleteChunkError(const char * err, std::size_t len)
{
    return 0 == std::strcmp(err + len - 7u, "'<eof>'");
}

} //blua

#define bla_lua_equal lua_equal

//pushglobaltable is missing but easy to fake via old method of getting globals:
#define bla_lua_pushglobaltable(L) (lua_pushvalue((L), LUA_GLOBALSINDEX))

//LUA_OK is missing but 0 is assumed to be 'success' value in comments, so:
#define BLA_LUA_OK 0

#endif //LUA 5.1

#endif	/* LUAHEADER_HPP */

