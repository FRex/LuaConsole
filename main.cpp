#include <SFML/Graphics.hpp>
#include <lua.hpp>
#include "LuaConsole.hpp"

int main()
{
    sf::RenderWindow app(sf::VideoMode(890u, 520u), "LuaConsole");
    app.setFramerateLimit(30u);
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    lua::LuaConsole console;
    console.setL(L);

    while(app.isOpen())
    {
        sf::Event eve;
        while(app.pollEvent(eve))
        {
            if(eve.type == sf::Event::Closed) app.close();
            console.handleEvent(eve);
        }
        app.clear();
        app.draw(console);
        app.display();
    }
}
