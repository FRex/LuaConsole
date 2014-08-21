/* 
 * File:   LuaConsoleCallbacks.hpp
 * Author: frex
 *
 * Created on August 21, 2014, 8:54 PM
 */

#ifndef LUACONSOLECALLBACKS_HPP
#define	LUACONSOLECALLBACKS_HPP

namespace lua {

class LuaConsoleCallbacks
{
public:
    virtual ~LuaConsoleCallbacks() = default;
    virtual void onNewHistoryItem() = 0;

};

}

#endif	/* LUACONSOLECALLBACKS_HPP */

