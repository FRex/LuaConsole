#include "LuaCompletion.hpp"
#include <lua.hpp>
#include <sstream>
#include <algorithm>

namespace lua {

//TODO: these still need some work to be more accurate

void prepareHints(lua_State * L, std::string str, std::string& last)
{
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    auto ll = str.find_last_of("()[]{}\"'+-=/*^%#~");
    if(ll != std::string::npos) str.erase(0, 1 + ll);

    std::vector<std::string> tables;
    int begin = 0;
    for(std::size_t i = 0u; i < str.size(); ++i)
    {
        if(str[i] == '.' || str[i] == ':')
        {
            tables.push_back(str.substr(begin, i - begin));
            begin = i + 1; //?
        }
    }
    last = str.substr(begin);

    lua_pushglobaltable(L);
    for(std::size_t i = 0u; i < tables.size(); ++i)
    {
        if(lua_type(L, -1) != LUA_TTABLE) //if not a table, try fetch meta now
        {
            lua_getmetatable(L, -1);
        }

        if(lua_type(L, -1) != LUA_TTABLE && !luaL_getmetafield(L, -1, "__index") && !lua_getmetatable(L, -1)) break;
        if(lua_type(L, -1) != LUA_TTABLE) break; //no 
        lua_pushlstring(L, tables[i].c_str(), tables[i].size());
        lua_gettable(L, -2);
    }
}

bool collectHints(lua_State * L, std::vector<std::string>& possible, const std::string& last, bool usehidden)
{
    const bool skipunderscore = last.empty() && !usehidden;
    if(lua_type(L, -1) != LUA_TTABLE) return false;
    lua_pushnil(L); //perpare iteration
    while(lua_next(L, -2))
    {
        std::size_t keylen;
        const char * key;
        bool match = true;
        lua_pop(L, 1); //pop the value - we don't care for it yet
        lua_pushvalue(L, -1); //need this to not confuse lua_next
        key = lua_tolstring(L, -1, &keylen);

        if(last.size() > keylen) //cant match start of this str, we are longer
        {
            lua_pop(L, 1);
            continue;
        }

        for(std::size_t i = 0u; i < last.size(); ++i) //compare up to our len
            if(key[i] != last[i]) match = false;

        if(match && (!skipunderscore || key[0] != '_'))
            possible.push_back(key);

        lua_pop(L, 1); //pop our str copy and let lua_next see the key itself
    }
    return true;
}

}
