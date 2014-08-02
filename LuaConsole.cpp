#include "LuaConsole.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <lua.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "LuaConsoleModel.hpp"
#include "LuaConsoleView.hpp"

namespace lua {

LuaConsole::LuaConsole() : m_model(new LuaConsoleModel), m_view(new LuaConsoleView)
{
    m_model->setWidth(78);
    m_view->setBackgroundColor(sf::Color(0u, 127u, 127u, 127u)); //mild half cyan
}

LuaConsole::~LuaConsole()
{
    delete m_model;
    delete m_view;
}

void LuaConsole::echo(const std::string& msg)
{
    m_model->echo(msg);
}

bool LuaConsole::handleEvent(const sf::Event& eve)
{
    if(!isVisible()) return false;
    switch(eve.type)
    {
        case sf::Event::KeyPressed:
            handleKeyEvent(eve);
            return true;
            break;
        case sf::Event::TextEntered:
            m_model->addChar(static_cast<char>(eve.text.unicode));
            return true;
            break;
    } //eve.type
    return false;
}

void LuaConsole::handleKeyEvent(const sf::Event& eve)
{
    assert(eve.type == sf::Event::KeyPressed);

    switch(eve.key.code)
    {
        case sf::Keyboard::BackSpace:
            m_model->backspace();
            break;
        case sf::Keyboard::Return:
            m_model->parseLastLine();
            break;
        case sf::Keyboard::Left:
            m_model->moveCursor(-1);
            break;
        case sf::Keyboard::Right:
            m_model->moveCursor(1);
            break;
        case sf::Keyboard::End:
            m_model->moveCursor(kCursorEnd);
            break;
        case sf::Keyboard::Home:
            m_model->moveCursor(kCursorHome);
            break;
        case sf::Keyboard::Up:
            m_model->readHistory(-1);
            break;
        case sf::Keyboard::Down:
            m_model->readHistory(1);
            break;
        case sf::Keyboard::Tab:
            m_model->tryComplete();
            break;
    }
}

void LuaConsole::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if(!isVisible()) return;
    sf::View v = target.getView();
    target.setView(sf::View(sf::FloatRect(sf::Vector2f(), sf::Vector2f(target.getSize()))));
    m_view->geoRebuild(*m_model);
    m_view->draw(target, states);
    target.setView(v);
}

void LuaConsole::setL(lua_State * L)
{
    this->L = L;
    m_visible = m_model->setL(L);
}

void LuaConsole::setBackgroundColor(sf::Color c)
{
    m_view->setBackgroundColor(c);
}

LuaConsoleModel* LuaConsole::model() const
{
    return m_model;
}

LuaConsoleView* LuaConsole::view() const
{

    return m_view;
}

bool LuaConsole::isVisible() const
{

    return m_visible;
}

void LuaConsole::setVisible(bool visible)
{

    m_visible = visible;
}

void LuaConsole::toggleVisible()
{
    setVisible(!m_visible);
}

}
