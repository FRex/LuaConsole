#include "LuaConsoleModel.hpp"
#include "LuaHeader.hpp"
#include "LuaCompletion.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <fstream>

namespace lua {

//how wide is console -- this has to be adjustable later
const int kInnerWidth = 78;

//name of metatable/type in registry that our console is using
const char * const kMetaname = "bla_LuaConsole";

//frame/cursor/special unicode chars:
const unsigned kFullBlockChar = 0x2588u;
const unsigned kVerticalBarChar = 0x2550u;
const unsigned kHorizontalBarChar = 0x2551u;
const unsigned kULFrameChar = 0x2554u;
const unsigned kBLFrameChar = 0x255au;
const unsigned kBRFrameChar = 0x255du;
const unsigned kURFrameChar = 0x2557u;

//how many history items and messages(not wide) to keep
const int kHistoryKeptCount = 100;
const int kMessagesKeptCount = 100;

const char * const kHistoryFilename = "luaconsolehistory.txt";
const char * const kInitFilename = "luaconsoleinit.lua";

//this is the best way to do it when we assume 1 console per lua state
static int TheLightKey;

inline static void * getLightKey()
{
    return &TheLightKey;
}

LuaConsoleModel* LuaConsoleModel::getFromRegistry(lua_State* L)
{
    //get our console from the registry
    lua_pushlightuserdata(L, getLightKey());
    lua_gettable(L, LUA_REGISTRYINDEX);

    //if we fail we return null
    LuaConsoleModel * ret = nullptr;

    //if we got userdata and it has a metatable
    if(lua_type(L, -1) == LUA_TUSERDATA && lua_getmetatable(L, -1))
    {
        //we now got the ud's table pushed (since getmetatable returned true)
        //so look up the _real_ console table in registry
        lua_getfield(L, LUA_REGISTRYINDEX, kMetaname);

        //if it's equal to what our userdata has, we are 99.9% good to go
        //(the 0.01% is for _insanely_ broken state or someone super actively
        //*sabotaging* us, we can deal with neither)
        if(bla_lua_equal(L, -1, -2))
        {
            //get the console itself
            ret = *static_cast<LuaConsoleModel**>(lua_touserdata(L, -3));
        }

        //pop both tables, ours and regs
        lua_pop(L, 2);
    }

    lua_pop(L, 1); //pop the console itself
    return ret;
}

LuaConsoleModel::LuaConsoleModel(unsigned options) :
m_dirtyness(1u), //because 0u is what view starts at
m_lastupdate(0u),
m_cur(1),
L(nullptr),
m_w(kInnerWidth),
m_options(options),
m_visible(options & ECO_START_VISIBLE),
m_emptyenterrepeat(true)
{
    for(int i = 0; i < 24 * 80; ++i)
    {
        m_screen[i].Char = ' '; //0x2588
        m_screen[i].Color = 0xffffffff;
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

    m_colors[ECC_ERROR] = 0xff0000ff;
    m_colors[ECC_HINT] = 0x00ff00ff;
    m_colors[ECC_CODE] = 0xffff00ff;
    m_colors[ECC_ECHO] = 0xffffffff;
    m_colors[ECC_PROMPT] = 0xffffffff;
    m_colors[ECC_TITLE] = 0xffffffff;
    m_colors[ECC_FRAME] = 0xa9a9a9ff;

    //read history from file if desired
    if(m_options & ECO_HISTORY)
    {
        std::ifstream file(kHistoryFilename);
        std::string str;
        while(std::getline(file, str) && m_history.size() < kHistoryKeptCount)
        {
            m_history.push_back(str);
        }
    }
    m_hindex = m_history.size();

    for(int i = 0; i < ECT_COUNT; ++i)
    {
        m_callbackdata[i] = nullptr;
        m_callbackfuncs[i] = nullptr;
    }
}

LuaConsoleModel::~LuaConsoleModel()
{
    //save history to file if desired
    if(m_options & ECO_HISTORY)
    {
        std::ofstream file(kHistoryFilename);
        for(std::size_t i = 0u; i < m_history.size(); ++i)
        {
            file << m_history[i] << std::endl;
        }
    }
}

void LuaConsoleModel::moveCursor(int move)
{
    m_cur += move;
    m_cur = std::max<int>(m_cur, 1);
    m_cur = std::min<int>(m_lastline.size() + 1, m_cur);
    ++m_dirtyness;
}

void LuaConsoleModel::readHistory(int change)
{
    m_hindex += change;
    m_hindex = std::max<int>(m_hindex, 0);
    m_hindex = std::min<int>(m_hindex, m_history.size());

    if(static_cast<std::size_t>(m_hindex) == m_history.size())
    {
        m_lastline.clear();
        moveCursor(kCursorHome);
    }
    else
    {
        m_lastline = m_history[m_hindex];
        moveCursor(kCursorEnd);
    }

    ++m_dirtyness;
}

void LuaConsoleModel::parseLastLine()
{
    if(m_lastline.size() == 0u && m_emptyenterrepeat && !m_history.empty())
        m_lastline = m_history.back();

    echoColored(m_lastline, m_colors[ECC_CODE]);
    m_history.push_back(m_lastline);

    if(m_history.size() > kHistoryKeptCount) m_history.erase(m_history.begin());

    m_hindex = m_history.size();

    //call before running, in case crash, exit etc.
    if(m_callbackfuncs[ECT_NEWHISTORY])
        m_callbackfuncs[ECT_NEWHISTORY](this, m_callbackdata[ECT_NEWHISTORY]);

    m_buffcmd += m_lastline;
    m_buffcmd += '\n';

    if(L)
    {
        if(luaL_dostring(L, m_buffcmd.c_str()))
        {
            std::size_t len;
            const char * err = lua_tolstring(L, -1, &len);

            if(!lua::incompleteChunkError(err, len))
            {
                m_buffcmd.clear(); //failed normally - clear it
                echoColored(err, m_colors[ECC_ERROR]);
            }

            lua_pop(L, 1);
        }//got an error, real or <eof>/incomplete chunk one
        else
        {
            m_buffcmd.clear(); //worked & done - clear it
        }
    }//L is not null
    else
    {
        //say kindly we are kind of in trouble
        echoColored("Lua state pointer is NULL, commands have no effect", m_colors[ECC_ERROR]);
    }//L is null

    m_lastline.clear();
    m_cur = 1;
    ++m_dirtyness;
}

void LuaConsoleModel::addChar(char c)
{
    if(c < ' ' || c >= 127 || m_cur >= kInnerWidth || m_lastline.size() + 1u >= kInnerWidth)
        return;

    m_lastline.insert(m_lastline.begin() + m_cur - 1, c);
    ++m_cur;
    ++m_dirtyness;
}

void LuaConsoleModel::backspace()
{
    if(m_cur > 1)
    {
        --m_cur;
        m_lastline.erase(m_cur - 1, 1);
        ++m_dirtyness;
    }
}

void LuaConsoleModel::del()
{
    m_lastline.erase(m_cur - 1, 1);
    ++m_dirtyness;
}

unsigned LuaConsoleModel::getDirtyness() const
{
    return m_dirtyness;
}

//split str on newlines and to fit 'width' length and push to given vector (if not null)
//returns how many messages str was split into

static std::size_t pushWideMessages(const ColoredLine& str, std::vector<ColoredLine>* widemsgs, unsigned width)
{
    std::size_t ret = 0u;
    std::size_t charcount = 0u;
    std::size_t start = 0u;

    //push pieces of str if they go over width or if we encounter a newline
    for(std::size_t i = 0u; i < str.Text.size(); ++i)
    {
        ++charcount;
        if(str.Text[i] == '\n' || charcount >= width)
        {
            if(str.Text[i] == '\n') --charcount;
            if(widemsgs)
            {
                ColoredLine line;
                line.Text = str.Text.substr(start, charcount);
                line.Color = str.Color.substr(start, charcount);
                widemsgs->push_back(line);
            }
            ++ret;
            start = i + 1u;
            charcount = 0u;
        }
    }

    //push last piece if loop didn't
    if(charcount != 0u)
    {
        if(widemsgs)
        {
            ColoredLine line;
            line.Text = str.Text.substr(start, charcount);
            line.Color = str.Color.substr(start, charcount);
            widemsgs->push_back(line);
        }
        ++ret;
    }
    return ret;
}

void LuaConsoleModel::echo(const std::string& str)
{
    echoColored(str, m_colors[ECC_ECHO]);
}

void LuaConsoleModel::echoColored(const std::string& str, unsigned textcolor)
{
    const ColorString color(str.size(), textcolor);
    echoLine(str, color);
}

void LuaConsoleModel::echoLine(const std::string& str, const ColorString& colors)
{
    if(str.empty()) return echoLine(" ", colors); //workaround for a bug??

    ColoredLine line;
    line.Text = str;
    line.Color = colors;
    line.resizeColorToFitText(m_colors[ECC_ECHO]);

    m_msg.push_back(line);

    pushWideMessages(line, &m_widemsg, m_w);

    if(m_msg.size() > kMessagesKeptCount)
    {
        const std::size_t msgs = pushWideMessages(*m_msg.begin(), nullptr, m_w);
        m_msg.erase(m_msg.begin());
        m_widemsg.erase(m_widemsg.begin(), m_widemsg.begin() + msgs);
    }

    ++m_dirtyness;
}

const std::string& LuaConsoleModel::getWideMsg(int index) const
{
    if(index < 0) index = m_widemsg.size() + index;
    if(index < 0 || static_cast<std::size_t>(index) >= m_widemsg.size()) return m_empty.Text;

    return m_widemsg[index].Text;
}

const ColorString& LuaConsoleModel::getWideColor(int index) const
{
    if(index < 0) index = m_widemsg.size() + index;
    if(index < 0 || static_cast<std::size_t>(index) >= m_widemsg.size()) return m_empty.Color;

    return m_widemsg[index].Color;
}

const std::string& LuaConsoleModel::getLastLine() const
{
    return m_lastline;
}

int LuaConsoleModel::getCurPos() const
{
    return m_cur;
}

static int ConsoleModel_echo(lua_State * L)
{
    LuaConsoleModel * m = *static_cast<LuaConsoleModel**>(lua_touserdata(L, lua_upvalueindex(1)));
    if(m)
        m->echo(luaL_checkstring(L, 1));

    return 0;
}

static int ConsoleModel_gc(lua_State * L)
{
    LuaConsoleModel * m = *static_cast<LuaConsoleModel**>(lua_touserdata(L, 1));

    //rationale: assume if our instance in registry and all references of it 
    //got destroyed we are not able to or supposed to use the same L anymore
    if(m)
        m->setL(nullptr);

    return 0;
}

void LuaConsoleModel::setL(lua_State * L)
{
    //TODO: add support for more L's being linked/using echos at once??
    this->L = L;
    
    //clear and disarm, so the state that used us before is no longer able to
    m_luaptr.clearLuaPointer();
    m_luaptr.disarmLuaPointer();

    if(L)
    {
        LuaConsoleModel ** ptr = static_cast<LuaConsoleModel**>(lua_newuserdata(L, sizeof (LuaConsoleModel*)));
        (*ptr) = this;
        m_luaptr.setLuaPointer(ptr);

        //make and set new metatable with gc in it

        luaL_newmetatable(L, kMetaname); //table
        lua_pushliteral(L, "__gc");
        lua_pushcfunction(L, &ConsoleModel_gc);
        lua_settable(L, -3); //table[gc]=ConsoleModel_gc
        lua_setmetatable(L, -2);

        lua_pushlightuserdata(L, getLightKey());
        lua_pushvalue(L, -2);
        lua_settable(L, LUA_REGISTRYINDEX);

        lua_pushcclosure(L, &ConsoleModel_echo, 1);
        lua_setglobal(L, "echo");

        if(m_options & ECO_INIT)
        {
            if(luaL_dofile(L, kInitFilename) == BLA_LUA_OK)
            {
                m_visible = lua_toboolean(L, -1);
            }
            else
            {
                echoColored(lua_tostring(L, -1), m_colors[ECC_ERROR]);
                m_visible = true; //crapped up init is important so show console right away
            }
        }
    }//if L
}

void LuaConsoleModel::tryComplete()
{
    if(!L)
    {
        //be nice and say there won't be any completions
        echoColored("Lua state pointer is NULL, no completion available", m_colors[ECC_ERROR]);
        return;
    }

    std::vector<std::string> possible; //possible matches
    std::string last;

    prepareHints(L, m_lastline, last);
    const bool normalhints = collectHints(L, possible, last, false);
    const bool hasmetaindex = luaL_getmetafield(L, -1, "__index");
    const bool metahints = hasmetaindex && collectHints(L, possible, last, false);
    if(!(normalhints || metahints))
    {
        //if all else fails, assume we want _any_ completion and use global table
        bla_lua_pushglobaltable(L);
        collectHints(L, possible, last, false);
    }

    lua_settop(L, 0); //pop all trash we put on the stack

    if(possible.size() > 1u)
    {
        std::string msg = possible[0];
        for(std::size_t i = 1u; i < possible.size(); ++i)
        {
            msg += " " + possible[i];
        }
        echoColored(msg, m_colors[ECC_HINT]);

        const std::string commonprefix = commonPrefix(possible);
        m_lastline += commonprefix.substr(last.size());
        ++m_dirtyness;
        moveCursor(kCursorEnd);
    }
    else if(possible.size() == 1)
    {
        //m_lastline.erase(m_lastline.size() - last.size());
        m_lastline += possible[0].substr(last.size());
        ++m_dirtyness;
        moveCursor(kCursorEnd);
    }
}

const std::vector<std::string>& LuaConsoleModel::getHistory() const
{
    return m_history;
}

void LuaConsoleModel::setHistory(const std::vector<std::string>& history)
{
    m_history = history;
    m_hindex = history.size();
}

void LuaConsoleModel::setCallback(ECALLBACK_TYPE type, CallbackFunc func, void* data)
{
    if(type == ECT_COUNT)
        return;

    m_callbackfuncs[type] = func;
    m_callbackdata[type] = data;
}

void LuaConsoleModel::setVisible(bool visible)
{
    if(m_visible != visible)
        ++m_dirtyness;

    m_visible = visible;
}

bool LuaConsoleModel::isVisible() const
{
    return m_visible;
}

void LuaConsoleModel::toggleVisible()
{
    m_visible = !m_visible;
    ++m_dirtyness;
}

void LuaConsoleModel::setColor(ECONSOLE_COLOR which, unsigned color)
{
    if(which != ECC_COUNT)
        m_colors[which] = color;
}

unsigned LuaConsoleModel::getColor(ECONSOLE_COLOR which) const
{
    if(which == ECC_COUNT)
        return 0xffffffff;

    return m_colors[which];
}

void LuaConsoleModel::setEnterRepeatLast(bool eer)
{
    m_emptyenterrepeat = eer;
}

bool LuaConsoleModel::getEnterRepeatLast() const
{
    return m_emptyenterrepeat;
}

ScreenCell * LuaConsoleModel::getCells(int x, int y) const
{
    assert(0 < x);
    assert(x < 79);
    assert(0 < y);
    assert(y < 23);
    return m_screen + x + 80 * y;
}

const ScreenCell* LuaConsoleModel::getScreenBuffer() const
{
    updateBuffer();
    return m_screen;
}

void LuaConsoleModel::updateBuffer() const
{
    if(m_lastupdate == m_dirtyness)
        return;

    m_lastupdate = m_dirtyness;

    //first we clear out the top bar
    for(int i = 1; i < 79; ++i)
        m_screen[i].Char = kVerticalBarChar;

    //then we ensure frame is all OK colored, since setting title overwrites colors
    for(int i = 0; i < 80; ++i)
    {
        m_screen[i + 0 * 80].Color = m_colors[ECC_FRAME];
        m_screen[i + 23 * 80].Color = m_colors[ECC_FRAME];
    }
    for(int i = 0; i < 23; ++i)
    {
        m_screen[0 + i * 80].Color = m_colors[ECC_FRAME];
        m_screen[79 + i * 80].Color = m_colors[ECC_FRAME];
    }

    //now we can set the title and its' color
    for(int i = 1; i < std::min<int>(79, m_title.length() + 1); ++i)
    {
        m_screen[i].Char = m_title[i - 1];
        m_screen[i].Color = m_colors[ECC_TITLE];
    }


    for(int i = 1; i < 22; ++i)
    {
        const std::string& l = getWideMsg(i - 22);
        const ColorString& c = getWideColor(i - 22);

        ScreenCell * a = getCells(1, i);

        for(int x = 0; x < kInnerWidth; ++x)
        {
            a[x].Char = ' ';
            a[x].Color = 0xffffffff;
        }

        for(std::size_t x = 0u; x < l.size(); ++x)
        {
            a[x].Char = l[x];
            a[x].Color = c[x];
        }
    }

    ScreenCell * a = getCells(1, 22);

    for(int x = 0; x < kInnerWidth; ++x)
    {
        a[x].Char = ' ';
        a[x].Color = m_colors[ECC_PROMPT];
    }

    for(std::size_t x = 0; x < getLastLine().size(); ++x)
    {
        a[x].Char = getLastLine()[x];
    }
}

const std::string& LuaConsoleModel::getTitle() const
{
    return m_title;
}

void LuaConsoleModel::setTitle(const std::string& title)
{
    if(m_title != title)
        ++m_dirtyness;

    m_title = title;
}


} //lua
