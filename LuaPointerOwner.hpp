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
//
//it's used internally by lua console model but it's not in 'priv' namespace
//because it can just as well be used by anyone, since it has no console
//specific features/constraints
//since it requires using full userdata and makes class uncopyable
//it's not suitable for small light classes

template <typename T> class LuaPointerOwner
{
public:

    //default ctor, with null 'unarmed' pointer

    LuaPointerOwner() : m_luaptr(0x0) { }

    //destructor, will null out the pointer if we have one so anyone holding
    //the full userdata and trying to see the ptr in it will see null

    ~LuaPointerOwner()
    {
        clearLuaPointer();
    }

    //set lua ptr, use this with full userdata in which you store the pointer itself
    //use as such: setLuaPointer(static_cast<T**>(lua_newuserdata(L, sizeof (T*)))

    void setLuaPointer(T ** ptr)
    {
        clearLuaPointer();
        m_luaptr = ptr;
    }

    //method to call when __gc is ran, to not try null out memory no longer in use
    //later when we ourselves are destroyed

    void disarmLuaPointer()
    {
        m_luaptr = 0x0;
    }

    //method to call when we want to 'unlink' the instance we are managing
    //after that, anyone inspecting the ptr in our userdata will see null and can
    //react (or crash...) easily thanks to that

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

