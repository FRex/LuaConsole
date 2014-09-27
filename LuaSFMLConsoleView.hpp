/* 
 * File:   LuaSFMLConsoleView.hpp
 * Author: frex
 *
 * Created on July 19, 2014, 11:34 PM
 */

#ifndef LUASFMLCONSOLEVIEW_HPP
#define	LUASFMLCONSOLEVIEW_HPP

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Font.hpp>

namespace lua {

class LuaConsoleModel;

class LuaSFMLConsoleView : public sf::Drawable
{
public:
    LuaSFMLConsoleView(bool defaultfont = true);
    ~LuaSFMLConsoleView();
    void setBackgroundColor(sf::Color c);
    void setFont(const sf::Font * font);
    void geoRebuild(const LuaConsoleModel * model); //keep last

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    unsigned m_lastdirtyness; //for caching/laziness

    sf::Color m_consolecolor;
    sf::RectangleShape m_r; //cursor shape?

    const sf::Font * m_font;
    bool m_ownfont;
    sf::VertexArray m_vertices; //vertices with font
    bool m_defaultfont;
    bool m_modelvisible = false;

};

}

#endif	/* LUASFMLCONSOLEVIEW_HPP */

