/* 
 * File:   LuaConsoleView.hpp
 * Author: frex
 *
 * Created on July 19, 2014, 11:34 PM
 */

#ifndef LUACONSOLEVIEW_HPP
#define	LUACONSOLEVIEW_HPP

#include "LuaConsoleCommon.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Font.hpp>

namespace lua {

class LuaConsoleModel;

class LuaConsoleView
{
    friend class LuaConsole;
public:
    void setBackgroundColor(sf::Color c);

private:
    LuaConsoleView();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const;
    void geoRebuild(const LuaConsoleModel& model); //keep last

    sf::Uint32 * getCells(int x, int y);
    void doMsgs(const LuaConsoleModel& model);

    unsigned m_lastdirtyness; //for caching/laziness

    sf::Color m_backcolor;
    sf::RectangleShape m_r; //cursor shape?
    sf::Uint32 m_screen[80 * 24]; //make this adjustable?

    sf::Font m_font; //this needs to be a ptr that can be set?
    sf::VertexArray m_vertices; //vertices with font

};

}

#endif	/* LUACONSOLEVIEW_HPP */

