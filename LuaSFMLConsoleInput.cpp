#include "LuaSFMLConsoleInput.hpp"
#include "LuaConsoleModel.hpp"

namespace blua {

LuaSFMLConsoleInput::LuaSFMLConsoleInput(LuaConsoleModel* model) :
m_model(model),
m_togglekey(sf::Keyboard::Unknown) { }

void LuaSFMLConsoleInput::setModel(LuaConsoleModel* model)
{
    m_model = model;
}

LuaConsoleModel* LuaSFMLConsoleInput::getModel() const
{
    return m_model;
}

bool LuaSFMLConsoleInput::handleEvent(sf::Event event)
{
    if(!m_model)
        return false;

    if(m_togglekey != sf::Keyboard::Unknown && event.type == sf::Event::KeyPressed && event.key.code == m_togglekey)
    {
        m_model->toggleVisible();
        return true;
    }

    if(!m_model->isVisible())
        return false;

    switch(event.type)
    {
        case sf::Event::KeyPressed:
            if(event.key.control)
            {
                handleCtrlKeyEvent(event);
            }
            else
            {
                handleKeyEvent(event);
            }
            return true;
        case sf::Event::TextEntered:
            m_model->addChar(static_cast<char>(event.text.unicode));
            return true;
        default:
            return false;
    } //eve.type
    return false;
}

void LuaSFMLConsoleInput::handleKeyEvent(sf::Event event)
{
    assert(event.type == sf::Event::KeyPressed);

    switch(event.key.code)
    {
        case sf::Keyboard::BackSpace:
            m_model->backspace();
            break;
        case sf::Keyboard::Delete:
            m_model->del();
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
        default:
            //TODO:optionally do not consume all keys?
            break;
    }
}

void LuaSFMLConsoleInput::handleCtrlKeyEvent(sf::Event event)
{
    assert(event.type == sf::Event::KeyPressed);

    switch(event.key.code)
    {
        case sf::Keyboard::Left:
            m_model->moveCursorOneWord(blua::EMD_LEFT);
            break;
        case sf::Keyboard::Right:
            m_model->moveCursorOneWord(blua::EMD_RIGHT);
            break;
        default:
            //TODO:optionally do not consume all keys? (as above)
            break;
    }
}

void LuaSFMLConsoleInput::setToggleKey(sf::Keyboard::Key key)
{
    m_togglekey = key;
}

sf::Keyboard::Key LuaSFMLConsoleInput::getToggleKey() const
{
    return m_togglekey;
}

} //blua
