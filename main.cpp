#include <SFML/Graphics.hpp>
#include "LuaHeader.hpp"
#include "LuaConsoleModel.hpp"
#include "LuaSFMLConsoleView.hpp"
#include "LuaSFMLConsoleInput.hpp"
#include <cstdlib>
#include <ctime>

const unsigned colors[] = {
    0xffffffFF, //white  
    0xff0000FF, //red
    0x00ff00FF, //green
    0x0000ffFF, //blue
    0xffff00FF, //yellow
    0xff00ffFF, //magenta
    0x00ffffFF, //cyan
    0x5f9ea0FF, //cadet blue
    0xa52a2aFF, //brown
    0x7fff00FF, //chartreuse
    0xdc143cFF, //crimson
    0xb8860bFF, //dark golden rod
    0xff8c00FF, //dark orange
    0xff1493FF, //deep pink
    0xff69b4FF, //hot pink
    0x808000FF, //olive
    0x8b008bFF, //dark magenta
    0x000000FF //black
};

const unsigned kColorsCount = sizeof (colors) / sizeof (colors[0]);

int demo_rainbowEcho(lua_State * L)
{
    std::size_t len;
    const char * msg = luaL_checklstring(L, 1, &len);
    lua::LuaConsoleModel * model = lua::LuaConsoleModel::getFromRegistry(L);
    if(model)
    {
        lua::ColorString color(len, 0x0);
        for(std::size_t i = 0u; i < len; ++i)
            color[i] = colors[std::rand() % kColorsCount];

        model->echoLine(msg, color);
    }
    return 0;
}

int demo_setTitle(lua_State * L)
{
    const char * title = luaL_checkstring(L, 1);
    lua::LuaConsoleModel * model = lua::LuaConsoleModel::getFromRegistry(L);
    if(model)
        model->setTitle(title);

    return 0;
}

const luaL_Reg demoReg[] = {
    {"rainbowEcho", &demo_rainbowEcho},
    {"setTitle", &demo_setTitle},
    {nullptr, nullptr}
};

void openDemo(lua_State * L)
{
    lua_newtable(L);
    //we do reg manually because
    //lua 5.2 deprecated register and 5.1 doesnt have setfuncs yet
    int iter = 0;
    while(demoReg[iter].name)
    {
        lua_pushstring(L, demoReg[iter].name);
        lua_pushcfunction(L, demoReg[iter].func);
        lua_settable(L, -3);
        ++iter;
    }
    lua_setglobal(L, "demo");
}

int main()
{
    std::srand(std::time(nullptr));
    sf::RenderWindow app(sf::VideoMode(890u, 520u), "LuaConsole");
    app.setFramerateLimit(30u);
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    openDemo(L);
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
