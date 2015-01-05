#include <LuaConsole/LuaSFMLConsoleView.hpp>
#include <LuaConsole/LuaConsoleModel.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace blua {

const unsigned kFontSize = 18u;

const char * const kFontName = "DejaVuSansMono.ttf";

static sf::Color toColor(unsigned color)
{
    sf::Color ret;
    ret.r = (color & 0xff000000) >> 24;
    ret.g = (color & 0xff0000) >> 16;
    ret.b = (color & 0xff00) >> 8;
    ret.a = color & 0xff;
    return ret;
}

LuaSFMLConsoleView::LuaSFMLConsoleView(bool defaultfont) :
m_lastdirtyness(0u),
m_font(0x0),
m_ownfont(false),
m_defaultfont(defaultfont),
m_modelvisible(false)
{
    m_vertices.setPrimitiveType(sf::Quads);

    if(m_defaultfont)
    {
        sf::Font * dfont = new sf::Font;
        dfont->loadFromFile(kFontName);
        m_font = dfont;
        m_ownfont = true;
    }
}

LuaSFMLConsoleView::~LuaSFMLConsoleView()
{
    if(m_ownfont)
        delete m_font;
}

void LuaSFMLConsoleView::draw(sf::RenderTarget& target, sf::RenderStates /* states --unused */) const
{
    if(!m_font || !m_modelvisible)
        return;

    //save old view and set "normal" view for our drawing
    sf::View v = target.getView();
    target.setView(sf::View(sf::FloatRect(sf::Vector2f(), sf::Vector2f(target.getSize()))));

    sf::RectangleShape sha;
    sha.setPosition(m_vertices.getBounds().left, m_vertices.getBounds().top);
    sha.setSize(sf::Vector2f(m_vertices.getBounds().width, m_vertices.getBounds().height));
    sha.setFillColor(m_consolecolor);
    target.draw(sha);
    target.draw(m_r);
    target.draw(m_vertices, &m_font->getTexture(kFontSize));

    //reset original view
    target.setView(v);
}

void LuaSFMLConsoleView::setFont(const sf::Font * font)
{
    //do something to dirtyness to force rebuilding letters??

    if(m_ownfont)
        delete m_font;

    m_font = font;
    m_ownfont = false; //never own a set font

    //if set font is null and we have default option then set it
    if(!m_font && m_defaultfont)
    {
        sf::Font * dfont = new sf::Font;
        dfont->loadFromFile(kFontName);
        m_font = dfont;
        m_ownfont = true;
    }
}

const sf::Font * LuaSFMLConsoleView::getFont() const
{
    return m_font;
}

//code below was taken from SFML Text.cpp and MODIFIED,
//it is NOT the original software, if looking for original software see:
//https://github.com/LaurentGomila/SFML/

////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2014 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
// you must not claim that you wrote the original software.
// If you use this software in a product, an acknowledgment
// in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
// and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

void LuaSFMLConsoleView::geoRebuild(const LuaConsoleModel * model)
{
    if(!model)
        return;

    //this can change between calls but doesnt warrant expensive rebuild
    //so doesnt bump dirtyness, so we take it right here
    m_consolecolor = toColor(model->getColor(ECC_BACKGROUND));
    m_r.setFillColor(toColor(model->getColor(ECC_CURSOR)));

    //obviously no need to rebuild when model isn't any dirtier
    if(m_lastdirtyness == model->getDirtyness())
        return;

    //take dirtyness after font so setting font late works
    if(!m_font)
        return;

    m_lastdirtyness = model->getDirtyness();
    m_modelvisible = model->isVisible();

    //dont bother build geo that isnt going to be drawn
    if(!m_modelvisible)
        return;

    const ScreenCell * screen = model->getScreenBuffer();

    // Clear the previous geometry
    m_vertices.clear();

    // Precompute the variables needed by the algorithm
    float hspace = m_font->getGlyph(L' ', kFontSize, false).advance;
    float vspace = m_font->getLineSpacing(kFontSize);
    float x = 0.f;
    float y = kFontSize;

    sf::Uint32 prevChar = 0u;

    for(std::size_t i = 0u; i < 24u * 80u; ++i)
    {
        sf::Uint32 curChar = screen[i].Char;

        // Apply the kerning offset
        x += m_font->getKerning(prevChar, curChar, kFontSize);
        prevChar = curChar;

        //check if cursor is here and if yes, add fullblock gylph
        if(model->getCurPos() + 80u * 22u == i)
        {
            const sf::Uint32 kFullBlockChar = 0x2588u; //unicode fullblock
            const sf::Glyph g = m_font->getGlyph(kFullBlockChar, kFontSize, false);
            m_r.setSize(sf::Vector2f(g.bounds.width, g.bounds.height));
            m_r.setPosition(sf::Vector2f(x + g.bounds.left, y + g.bounds.top));
        }

        // Handle spaces
        switch(curChar)
        {
            case '\t': case '\n': case ' ':
                x += hspace;
                continue;
        }

        // Extract the current glyph's description
        const sf::Glyph& glyph = m_font->getGlyph(curChar, kFontSize, false);

        int left = glyph.bounds.left;
        int top = glyph.bounds.top;
        int right = glyph.bounds.left + glyph.bounds.width;
        int bottom = glyph.bounds.top + glyph.bounds.height;

        float u1 = glyph.textureRect.left;
        float v1 = glyph.textureRect.top;
        float u2 = glyph.textureRect.left + glyph.textureRect.width;
        float v2 = glyph.textureRect.top + glyph.textureRect.height;

        //add a quad for the current character
        const sf::Color col = toColor(screen[i].Color);
        m_vertices.append(sf::Vertex(sf::Vector2f(x + left, y + top), col, sf::Vector2f(u1, v1)));
        m_vertices.append(sf::Vertex(sf::Vector2f(x + right, y + top), col, sf::Vector2f(u2, v1)));
        m_vertices.append(sf::Vertex(sf::Vector2f(x + right, y + bottom), col, sf::Vector2f(u2, v2)));
        m_vertices.append(sf::Vertex(sf::Vector2f(x + left, y + bottom), col, sf::Vector2f(u1, v2)));

        // Advance to the next character
        x += glyph.advance;

        if(i % 80 == 79)
        {
            y += vspace;
            x = 0;

            //we dont use \ns so we fake them, this is in theory for kerning but
            //probably doesnt matter since we (supposedly) use a monospaced
            //font anyway
            prevChar = '\n';
        }
    } //for(std::size_t i = 0u; i < 24u * 80u; ++i)
}//geoRebuild

} //blua
