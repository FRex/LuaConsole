#include "LuaSFMLConsoleView.hpp"
#include "LuaConsoleModel.hpp"
#include "LuaConsoleCommon.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

namespace lua {

//frame/cursor/special unicode chars:
const sf::Uint32 kFullBlockChar = 0x2588u;
const sf::Uint32 kVerticalBarChar = 0x2550u;
const sf::Uint32 kHorizontalBarChar = 0x2551u;
const sf::Uint32 kULFrameChar = 0x2554u;
const sf::Uint32 kBLFrameChar = 0x255au;
const sf::Uint32 kBRFrameChar = 0x255du;
const sf::Uint32 kURFrameChar = 0x2557u;

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
m_font(nullptr),
m_ownfont(false),
m_defaultfont(defaultfont)
{
    setBackgroundColor(sf::Color(0u, 127u, 127u, 127u)); //mild half cyan

    m_vertices.setPrimitiveType(sf::Quads);

    for(int i = 0; i < 24 * 80; ++i)
    {
        m_screen[i].Char = ' '; //0x2588
        m_screen[i].Color = sf::Color::White;
    }

    //vertical
    for(int i = 0; i < 80; ++i)
    {
        m_screen[i + 80 * 0].Char = kVerticalBarChar;
        m_screen[i + 80 * 23].Char = kVerticalBarChar;
    }

    //horizontal
    for(int i = 0; i < 24; ++i)
    {
        m_screen[0 + 80 * i].Char = kHorizontalBarChar;
        m_screen[79 + 80 * i].Char = kHorizontalBarChar;
    }

    //corners
    m_screen[0 + 80 * 0].Char = kULFrameChar;
    m_screen[0 + 80 * 23].Char = kBLFrameChar;
    m_screen[79 + 80 * 23].Char = kBRFrameChar;
    m_screen[79 + 80 * 0].Char = kURFrameChar;

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
    if(m_ownfont) delete m_font;
}

void LuaSFMLConsoleView::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if(!m_font || !m_modelvisible) return;

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

ScreenCell * LuaSFMLConsoleView::getCells(int x, int y)
{
    assert(0 < x);
    assert(x < 79);
    assert(0 < y);
    assert(y < 23);
    return m_screen + x + 80 * y;
}

void LuaSFMLConsoleView::doMsgs(const LuaConsoleModel * model)
{
    for(int i = 1; i < 22; ++i)
    {
        const std::string& l = model->getWideMsg(i - 22);
        const ColorString& c = model->getWideColor(i - 22);

        ScreenCell * a = getCells(1, i);

        for(int x = 0; x < kInnerWidth; ++x)
        {
            a[x].Char = ' ';
            a[x].Color = sf::Color::White;
        }

        for(std::size_t x = 0u; x < l.size(); ++x)
        {
            a[x].Char = l[x];
            a[x].Color = toColor(c[x]);
        }
    }

    ScreenCell * a = getCells(1, 22);

    for(int x = 0; x < kInnerWidth; ++x)
    {
        a[x].Char = ' ';
        a[x].Color = sf::Color::White;
    }

    for(std::size_t x = 0; x < model->getLastLine().size(); ++x)
    {
        a[x].Char = model->getLastLine()[x];
    }
}

void LuaSFMLConsoleView::setBackgroundColor(sf::Color c)
{
    m_consolecolor = c;
}

void LuaSFMLConsoleView::setFont(const sf::Font * font)
{
    //do something to dirtyness to force rebuilding letters??

    if(m_ownfont) delete m_font;

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
    if(!model) return;
    m_modelvisible = model->isVisible();
    if(m_lastdirtyness == model->getDirtyness()) return; //no need
    if(!m_font) return; //take dirtyness after so setting font late works

    m_lastdirtyness = model->getDirtyness();
    if(!m_modelvisible) return;

    doMsgs(model);

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
        sf::Uint32 curChar = m_screen[i].Char;

        // Apply the kerning offset
        x += m_font->getKerning(prevChar, curChar, kFontSize);
        prevChar = curChar;

        if(model->getCurPos() + 80u * 22u == i)
        {
            const sf::Glyph g = m_font->getGlyph(kFullBlockChar, kFontSize, false);
            m_r.setSize(sf::Vector2f(g.bounds.width, g.bounds.height));
            m_r.setPosition(sf::Vector2f(x + g.bounds.left, y + g.bounds.top));
            m_r.setFillColor(sf::Color::Cyan);
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
        const sf::Color col = m_screen[i].Color;
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
        }
    }
}

} //lua

