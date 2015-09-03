#include <SFML/Graphics.hpp>
#include <lua.hpp>
#include <LuaConsole/LuaConsoleModel.hpp>
#include <LuaConsole/LuaSFMLConsoleView.hpp>
#include <LuaConsole/LuaSFMLConsoleInput.hpp>
#include <cstdlib>
#include <ctime>


//array of (html named) colors to use in the rainbow
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

//echo using a different color for each character

int demo_rainbowEcho(lua_State * L)
{
    std::size_t len;
    const char * msg = luaL_checklstring(L, 1, &len);

    //get the model from the registry
    blua::LuaConsoleModel * model = blua::LuaConsoleModel::getFromRegistry(L);

    //check, since it might be null if it was deleted before this lua state
    if(model)
    {
        //create a color string and fill it with random colors
        blua::ColorString color(len, 0x0);
        for(std::size_t i = 0u; i < len; ++i)
            color[i] = colors[std::rand() % kColorsCount];

        //echo it out using the function that takes message string and color string
        model->echoLine(msg, color);
    }
    return 0;
}

//set console title

int demo_setTitle(lua_State * L)
{
    const char * title = luaL_checkstring(L, 1);

    //get the model from the registry
    blua::LuaConsoleModel * model = blua::LuaConsoleModel::getFromRegistry(L);

    //check, since it might be null if it was deleted before this lua state
    if(model)
        model->setTitle(title);

    return 0;
}

//echo in color

int demo_echoColored(lua_State * L)
{
    const char * msg = luaL_checkstring(L, 1);
    unsigned color = static_cast<unsigned>(luaL_checknumber(L, 2)); //no check unsigned in 5.1/Jit

    blua::LuaConsoleModel * model = blua::LuaConsoleModel::getFromRegistry(L);
    if(model)
        model->echoColored(msg, color);

    return 0;
}

const char * const kColorNames[] = {
    "error", "hint", "code", "echo", "prompt", "title", "frame", "background",
    "cursor", 0x0
};

//set one of console colors

int demo_setConsoleColor(lua_State * L)
{
    const int opt = luaL_checkoption(L, 1, 0x0, kColorNames);
    unsigned color = static_cast<unsigned>(luaL_checknumber(L, 2)); //no check unsigned in 5.1/Jit

    blua::LuaConsoleModel * model = blua::LuaConsoleModel::getFromRegistry(L);
    if(model)
        model->setColor(static_cast<blua::ECONSOLE_COLOR>(opt), color);

    return 0;
}

const luaL_Reg demoReg[] = {
    {"rainbowEcho", &demo_rainbowEcho},
    {"setTitle", &demo_setTitle},
    {"echoColored", &demo_echoColored},
    {"setConsoleColor", &demo_setConsoleColor},
    {0x0, 0x0}
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
    //do that because we are gonna be using rand() in the demo functions
    std::srand(std::time(0x0));
    sf::RenderWindow app(sf::VideoMode(890u, 520u), "LuaConsole");
    app.setFramerateLimit(30u);

    //open our state, as usual, open demo table in it too
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    openDemo(L);

    //create our model
    blua::LuaConsoleModel model;

    //tell it which console it has to handle, if you forget you get clear errors
    //on each attempt to write or complete code, telling you you forgot to set it
    model.setL(L);

    //create the input which will filter and translate sf::Event s
    //into calls to model api functions that move the cursor, type characters etc.
    blua::LuaSFMLConsoleInput input(&model);
    input.setToggleKey(sf::Keyboard::LSystem);

    //since view doesnt use any methods of LuaConsoleModel it doesnt need any
    //pointer to it, this class handles the font and drawing itself,
    //but not the layouting, which is handled by model
    blua::LuaSFMLConsoleView view;

    while(app.isOpen())
    {
        sf::Event eve;
        while(app.pollEvent(eve))
        {
            if(eve.type == sf::Event::Closed)
                app.close();

            //send even to console model via the input helper class
            //this returns a bool saying whether or not the event was
            //consumed by console but we dont care about it here
            input.handleEvent(eve);
        }
        app.clear();

        //pull all changes of characters, colors, etc. from the model
        //and get ready to render them (or not, if console is hidden)
        view.geoRebuild(&model);

        //draw the view with usual syntax, since it inherits from sf::Drawable
        app.draw(view);
        app.display();
    }
    lua_close(L);
}
