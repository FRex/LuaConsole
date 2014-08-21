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
    void setFont(const sf::Font * font);

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

    const sf::Font * m_font;
    sf::VertexArray m_vertices; //vertices with font

};

}

#endif	/* LUACONSOLEVIEW_HPP */

