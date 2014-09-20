/* 
 * File:   LuaSFMLConsoleInput.hpp
 * Author: frex
 *
 * Created on September 20, 2014, 7:07 PM
 */

#ifndef LUASFMLCONSOLEINPUT_HPP
#define	LUASFMLCONSOLEINPUT_HPP

#include <SFML/Window/Event.hpp>

namespace lua{

class LuaConsoleModel;

class LuaSFMLConsoleInput
{
public:
    LuaSFMLConsoleInput(LuaConsoleModel * model = nullptr);
    void setModel(LuaConsoleModel * model);
    LuaConsoleModel * getModel() const;
    bool handleEvent(sf::Event event);
    
private:
    void handleKeyEvent(sf::Event eve);
        
    LuaConsoleModel * m_model;
    
};

} //lua

#endif	/* LUASFMLCONSOLEINPUT_HPP */

