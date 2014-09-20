#include <SFML/Graphics.hpp>
#include "LuaHeader.hpp"
#include "LuaConsoleModel.hpp"
#include "LuaSFMLConsoleView.hpp"
#include "LuaSFMLConsoleInput.hpp"

int main()
{
    sf::RenderWindow app(sf::VideoMode(890u, 520u), "LuaConsole");
    app.setFramerateLimit(30u);
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    lua::LuaConsoleModel model;
    model.setL(L);
    lua::LuaSFMLConsoleInput input(&model);
    lua::LuaSFMLConsoleView view;

    while(app.isOpen())
    {
        sf::Event eve;
        while(app.pollEvent(eve))
        {
            if(eve.type == sf::Event::Closed) app.close();
            input.handleEvent(eve);
        }
        app.clear();
        view.geoRebuild(&model);
        app.draw(view);
        app.display();
    }
    lua_close(L);
}
