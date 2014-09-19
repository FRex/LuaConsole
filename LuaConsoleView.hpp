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

class ScreenCell
{
public:
    sf::Uint32 Char;
    sf::Color Color;

};

class LuaConsoleView
{
    friend class LuaConsole;
public:
    void setBackgroundColor(sf::Color c);
    void setFont(const sf::Font * font);

private:
    LuaConsoleView(unsigned options);
    ~LuaConsoleView();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const;
    void geoRebuild(const LuaConsoleModel& model); //keep last

    ScreenCell * getCells(int x, int y);
    void doMsgs(const LuaConsoleModel& model);

    unsigned m_lastdirtyness; //for caching/laziness

    sf::Color m_consolecolor;
    sf::RectangleShape m_r; //cursor shape?
    ScreenCell m_screen[80 * 24]; //make this adjustable?

    const sf::Font * m_font;
    bool m_ownfont;
    sf::VertexArray m_vertices; //vertices with font
    unsigned m_options;

};

}

#endif	/* LUACONSOLEVIEW_HPP */

