/* 
 * File:   LuaSFMLConsoleInput.hpp
 * Author: frex
 *
 * Created on September 20, 2014, 7:07 PM
 */

#ifndef LUASFMLCONSOLEINPUT_HPP
#define	LUASFMLCONSOLEINPUT_HPP

#include <SFML/Window/Event.hpp>
#include <LuaConsole/LuaConsoleExport.hpp>

namespace blua{

class LuaConsoleModel;

class LUACONSOLEAPI LuaSFMLConsoleInput
{
public:
    LuaSFMLConsoleInput(LuaConsoleModel * model = 0x0);
    void setModel(LuaConsoleModel * model);
    LuaConsoleModel * getModel() const;
    bool handleEvent(sf::Event event);
    void setToggleKey(sf::Keyboard::Key key);
    sf::Keyboard::Key getToggleKey() const;
    
private:
    void handleKeyEvent(sf::Event eve);
    void handleCtrlKeyEvent(sf::Event eve);
        
    LuaConsoleModel * m_model;
    sf::Keyboard::Key m_togglekey;
    
};

} //blua

#endif	/* LUASFMLCONSOLEINPUT_HPP */

