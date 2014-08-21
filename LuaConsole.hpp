/* 
 * File:   Terminal.hpp
 * Author: frex
 *
 * Created on February 2, 2014, 7:20 PM
 */

#ifndef LUACONSOLE_HPP
#define	LUACONSOLE_HPP

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Color.hpp>
#include <string>
#include <vector>

#include "LuaConsoleCommon.hpp"


//THIS CONSOLE STILL NEEDS LOTS OF WORK


namespace lua {

class LuaConsole : public sf::Drawable, private sf::NonCopyable
{
public:
    LuaConsole(unsigned options = ECO_DEFAULT);
    ~LuaConsole();

    void echo(const std::string& msg);
    bool handleEvent(const sf::Event& eve); //true if event was consumed

    void setL(lua_State * L);

    LuaConsoleModel* model() const;
    LuaConsoleView* view() const;

    void setVisible(bool visible);
    bool isVisible() const;
    void toggleVisible();

private:
    void handleKeyEvent(const sf::Event& eve);
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    LuaConsoleModel * m_model;
    LuaConsoleView * m_view;

    bool m_visible = false;
    unsigned m_options;

};

}

#endif	/* LUACONSOLE_HPP */

