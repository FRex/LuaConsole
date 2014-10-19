#include "LuaCompletion.hpp"
#include "LuaHeader.hpp"
#include <sstream>
#include <algorithm>

namespace blua {

//TODO: these still need some work to be more accurate

//this fetches the compacted string of table names so
//for "for i=1, vars . blabla : lul"
//it will return "vars.blabla.lul" and so on
//so the main table pushing code just needs to split the str on . and not worry
//about anything else

static std::string cleanTableList(const std::string& str)
{
    std::string ret;

    //remove spaces before and after dots, change : to .
    bool gotdot = false;
    std::size_t whitespacestart = 0u;
    bool gotwhite = false;
    for(std::size_t i = 0u; i < str.size(); ++i)
    {
        const char c = str[i] == ':'?'.':str[i];
        if(!gotwhite && c == ' ')
        {
            gotwhite = true;
            whitespacestart = i;
        }
        if(c == '.' && gotwhite)
        {
            for(std::size_t j = 0u; j < (i - whitespacestart); ++j)
                ret.erase(--ret.end());
        }
        if(c != ' ') gotwhite = false;
        if(c != ' ' || !gotdot) ret += c;
        if(c == '.') gotdot = true;
        if(c != '.' && c != ' ') gotdot = false;
    }

    //change 'special' chars to spaces (needed below)
    const std::string specials = "()[]{}\"'+-=/*^%#~,";
    for(std::size_t i = 0u; i < specials.size(); ++i)
        std::replace(ret.begin(), ret.end(), specials[i], ' ');

    //return everything starting at last space(exclusive)
    ret = ret.substr(ret.find_last_of(' ') + 1u);
    //    std::printf("got '%s' out of '%s'\n", ret.c_str(), str.c_str());
    return ret;
}

void prepareHints(lua_State * L, std::string str, std::string& last)
{
    str = cleanTableList(str); //fetch clean list of tables separated by .

    //split that list by .
    std::vector<std::string> tables; //list of tables except final one
    int begin = 0;
    for(std::size_t i = 0u; i < str.size(); ++i)
    {
        if(str[i] == '.')
        {
            tables.push_back(str.substr(begin, i - begin));
            begin = i + 1; //?
        }
    }
    last = str.substr(begin); //name of last table that we must hint to complete, might be empty

    bla_lua_pushglobaltable(L);
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
    lua_pushnil(L); //prepare iteration
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

        for(std::size_t i = 0u; i < last.size(); ++i) //compare up to our len = see if we are prefix of that key
            if(key[i] != last[i]) match = false;

        if(match && (!skipunderscore || key[0] != '_'))
            possible.push_back(key);

        lua_pop(L, 1); //pop our str copy and let lua_next see the key itself
    }
    return true;
}

std::string commonPrefix(const std::vector<std::string>& possible)
{
    std::string ret;
    std::size_t maxindex = 1000000000u;
    for(std::size_t i = 0u; i < possible.size(); ++i)
        maxindex = std::min(maxindex, possible[i].size());

    for(std::size_t checking = 0u; checking < maxindex; ++checking)
    {
        const char c = possible[0u][checking];
        for(std::size_t i = 1u; i < possible.size(); ++i)
        {
            if(c != possible[i][checking])
            {
                checking = maxindex;
                break;
            }
        }
        if(checking != maxindex)
            ret += c;
    }
    return ret;
}

} //blua
