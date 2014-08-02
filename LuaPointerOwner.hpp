/* 
 * File:   LuaPointerOwner.hpp
 * Author: frex
 *
 * Created on July 23, 2014, 3:44 AM
 */

#ifndef LUAPOINTEROWNER_HPP
#define	LUAPOINTEROWNER_HPP

#include <SFML/System/NonCopyable.hpp>
#include <cassert>

namespace lua {

//this class can be inherited or used as a member variable
//it will null out the pointer it is point to at destruction
//useful to prevent accessing invalid pointer that was pushed to lua
//it is noncopyable as that would cause errors

template <typename T> class LuaPointerOwner : sf::NonCopyable
{
public:

    LuaPointerOwner() : m_luaptr(nullptr) { }

    ~LuaPointerOwner()
    {
        if(m_luaptr)
            (*m_luaptr) = nullptr;
    }

    void setLuaPointer(T ** ptr)
    {
        assert(!m_luaptr);
        m_luaptr = ptr;
    }

    //method to call when __gc is ran, to not null out memory no longer in use

    void disarmLuaPointer()
    {
        m_luaptr = nullptr;
    }

private:
    T ** m_luaptr = nullptr;

};

}

#endif	/* LUAPOINTEROWNER_HPP */

