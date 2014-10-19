/* 
 * File:   LuaPointerOwner.hpp
 * Author: frex
 *
 * Created on July 23, 2014, 3:44 AM
 */

#ifndef LUAPOINTEROWNER_HPP
#define	LUAPOINTEROWNER_HPP

#include <cassert>

namespace blua {

//this class can be inherited or used as a member variable
//it will null out the pointer it is point to at destruction
//useful to prevent accessing invalid pointer that was pushed to lua
//it is noncopyable as that would cause errors

template <typename T> class LuaPointerOwner
{
public:

    LuaPointerOwner() : m_luaptr(0x0) { }

    ~LuaPointerOwner()
    {
        clearLuaPointer();
    }

    void setLuaPointer(T ** ptr)
    {
        clearLuaPointer();
        m_luaptr = ptr;
    }

    //method to call when __gc is ran, to not null out memory no longer in use

    void disarmLuaPointer()
    {
        m_luaptr = 0x0;
    }

    //method to call when we want to 'unlink' the instance we are managing

    void clearLuaPointer()
    {
        if(m_luaptr)
            (*m_luaptr) = 0x0;
    }

private:
    //delete copy and assignment to forbid copying (leads to errors)
    LuaPointerOwner(const LuaPointerOwner& other);
    LuaPointerOwner& operator=(const LuaPointerOwner& other);

    T ** m_luaptr;

};

} //blua

#endif	/* LUAPOINTEROWNER_HPP */

