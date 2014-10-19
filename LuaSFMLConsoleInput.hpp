/* 
 * File:   LuaSFMLConsoleInput.hpp
 * Author: frex
 *
 * Created on September 20, 2014, 7:07 PM
 */

#ifndef LUASFMLCONSOLEINPUT_HPP
#define	LUASFMLCONSOLEINPUT_HPP

#include <SFML/Window/Event.hpp>

namespace blua{

class LuaConsoleModel;

class LuaSFMLConsoleInput
{
public:
    LuaSFMLConsoleInput(LuaConsoleModel * model = 0x0);
    void setModel(LuaConsoleModel * model);
    LuaConsoleModel * getModel() const;
    bool handleEvent(sf::Event event);
    
private:
    void handleKeyEvent(sf::Event eve);
        
    LuaConsoleModel * m_model;
    
};

} //blua

#endif	/* LUASFMLCONSOLEINPUT_HPP */

