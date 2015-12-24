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

namespace blua {

class LuaConsoleModel;

class LuaSFMLConsoleView : public sf::Drawable
{
public:
    LuaSFMLConsoleView(bool defaultfont = true);
    ~LuaSFMLConsoleView();
    void setFont(const sf::Font * font);
    const sf::Font * getFont() const;
    void geoRebuild(const LuaConsoleModel * model); //keep last

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    unsigned m_lastdirtyness; //for caching/laziness
    const sf::Font * m_font;
    bool m_ownfont;
    sf::VertexArray m_vertices; //vertices with font
    bool m_defaultfont;
    bool m_modelvisible;

};

} //blua

#endif	/* LUASFMLCONSOLEVIEW_HPP */

