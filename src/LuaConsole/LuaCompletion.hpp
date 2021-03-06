/* 
 * File:   LuaCompletion.hpp
 * Author: frex
 *
 * Created on July 20, 2014, 1:12 PM
 */

#ifndef LUACOMPLETION_HPP
#define	LUACOMPLETION_HPP

#include <string>
#include <vector>

struct lua_State;

namespace blua {
namespace priv {

//splits the 'str' string, fill the 'last' string with part user needs completed
//and pushes right table/value on top of the stack
void prepareHints(lua_State * L, std::string str, std::string& last);

//collect hints for 'last' from value at top of the stack L and push them to 'possible'
//it also recurses through metatable chains (with no danger of infinite loop)
//usehidden decides if _names can be hints for empty string
bool collectHints(lua_State * L, std::vector<std::string>& possible, const std::string& last, bool usehidden);

//get the common prefix of all strings
std::string commonPrefix(const std::vector<std::string>& possible);

} //priv
} //blua

#endif	/* LUACOMPLETION_HPP */

