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

void LuaSFMLConsoleView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if(!m_font || !m_modelvisible)
        return;

    //draw in a single call in a 1:1 view
    sf::View v = target.getView();
    target.setView(sf::View(sf::FloatRect(sf::Vector2f(), sf::Vector2f(target.getSize()))));
    states.texture = &m_font->getTexture(kFontSize);
    target.draw(m_vertices, states);
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

    //reserve 4 vertices for background
    for(int i = 0; i < 4; ++i)
        m_vertices.append(sf::Vertex());

    // Precompute the variables needed by the algorithm
    float hspace = m_font->getGlyph(L' ', kFontSize, false).advance;
    float vspace = m_font->getLineSpacing(kFontSize);
    float x = 0.f;
    float y = kFontSize;

    sf::Uint32 prevChar = 0u;

    for(std::size_t i = 0u; i < model->getConsoleWidth() * model->getConsoleHeight(); ++i)
    {
        sf::Uint32 curChar = screen[i].Char;

        //move the reserved vertices so they won't affect the bounds
        if(i == 1u)
            for(std::size_t j = 0u; j < 4u; ++j)
                m_vertices[j].position = m_vertices[4].position;

        // Apply the kerning offset
        x += m_font->getKerning(prevChar, curChar, kFontSize);
        prevChar = curChar;

        //add a cursor under the glyph if this is the right position
        if(model->getCurPos() + (model->getConsoleWidth() * (model->getConsoleHeight() - 2)) == i)
        {
            const sf::Uint32 kFullBlockChar = 0x2588u; //unicode fullblock
            const sf::Glyph g = m_font->getGlyph(kFullBlockChar, kFontSize, false);
            const sf::Color cc = toColor(model->getColor(ECC_CURSOR));
            const sf::Vector2f tc = sf::Vector2f(1.f, 1.f); //solid pixel in SFML

            m_vertices.append(sf::Vertex(sf::Vector2f(x + g.bounds.left, y + g.bounds.top), cc, tc));
            m_vertices.append(sf::Vertex(sf::Vector2f(x + g.bounds.left, y + g.bounds.top + g.bounds.height), cc, tc));
            m_vertices.append(sf::Vertex(sf::Vector2f(x + g.bounds.left + g.bounds.width, y + g.bounds.top + g.bounds.height), cc, tc));
            m_vertices.append(sf::Vertex(sf::Vector2f(x + g.bounds.left + g.bounds.width, y + g.bounds.top), cc, tc));
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

        if((i + 1) % model->getConsoleWidth() == 0)
        {
            y += vspace;
            x = 0;

            //we dont use \ns so we fake them, this is in theory for kerning but
            //probably doesnt matter since we (supposedly) use a monospaced
            //font anyway
            prevChar = '\n';
        }
    } //for i

    //fill the reserved background vertices
    const sf::FloatRect vbounds = m_vertices.getBounds();
    m_vertices[0].position = sf::Vector2f(vbounds.left, vbounds.top);
    m_vertices[1].position = sf::Vector2f(vbounds.left, vbounds.top + vbounds.height);
    m_vertices[2].position = sf::Vector2f(vbounds.left + vbounds.width, vbounds.top + vbounds.height);
    m_vertices[3].position = sf::Vector2f(vbounds.left + vbounds.width, vbounds.top);

    const sf::Color bcolor = toColor(model->getColor(ECC_BACKGROUND));
    for(std::size_t j = 0u; j < 4u; ++j)
    {
        //SFML assumes this is a solid pixel
        m_vertices[j].texCoords = sf::Vector2f(1.f, 1.f);
        m_vertices[j].color = bcolor;
    }
}//geoRebuild

} //blua

