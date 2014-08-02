#include "LuaConsoleModel.hpp"
#include "LuaConsoleCommon.hpp"
#include <lua.hpp>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <fstream>

#include "LuaCompletion.hpp"

namespace lua {

//how many history items and messages(not wide) to keep
const int kHistoryKeptCount = 100;
const int kMessagesKeptCount = 100;

const char * const kHistoryFilename = "luaconsolehistory.txt";
const char * const kInitFilename = "luaconsoleinit.lua";

LuaConsoleModel::LuaConsoleModel() :
m_dirtyness(1u), //because 0u is what view starts at
m_cur(1),
L(nullptr)
{
    //read history from file
    std::ifstream file(kHistoryFilename);
    std::string str;
    while(std::getline(file, str) && m_history.size() < kHistoryKeptCount)
    {
        m_history.push_back(str);
    }
    m_hindex = m_history.size();
}

LuaConsoleModel::~LuaConsoleModel()
{
    //save history to file
    std::ofstream file(kHistoryFilename);
    for(int i = 0; i < m_history.size(); ++i)
    {
        file << m_history[i] << std::endl;
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

    if(m_hindex == m_history.size())
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
    assert(L);
    echo(m_lastline);
    m_history.push_back(m_lastline);

    if(m_history.size() > kHistoryKeptCount) m_history.erase(m_history.begin());

    m_hindex = m_history.size();
    m_buffcmd += m_lastline;
    m_buffcmd += '\n';

    if(luaL_dostring(L, m_buffcmd.c_str()))
    {
        std::size_t len;
        const char * err = lua_tolstring(L, -1, &len);

        if(0 != std::strcmp(err + len - 5, "<eof>"))
        {
            m_buffcmd.clear(); //failed normally - clear it
            echo(err);
        }

        lua_pop(L, 1);
    }
    else
    {
        m_buffcmd.clear(); //worked & done - clear it
    }

    m_lastline.clear();
    m_cur = 1;
    ++m_dirtyness;
}

void LuaConsoleModel::addChar(char c)
{
    if(c < ' ' || c >= 127 || m_cur >= kInnerWidth) return;

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

unsigned LuaConsoleModel::getDirtyness() const
{
    return m_dirtyness;
}

void LuaConsoleModel::echo(const std::string& str)
{
    if(str.empty()) return echo(" "); //workaround for a bug??

    m_msg.push_back(str);

    for(int i = 0; i * m_w < str.length(); ++i)
    {
        m_widemsg.push_back(str.substr(i*m_w, m_w));
    }

    m_widemsg.back().resize(m_w, ' ');

    if(m_msg.size() > kMessagesKeptCount)
    {
        const int s = m_msg.front().size();
        m_msg.erase(m_msg.begin());
        m_widemsg.erase(m_widemsg.begin(), m_widemsg.begin() + 1 + s / m_w);
    }

    ++m_dirtyness;
}

void LuaConsoleModel::setWidth(int w)
{
    m_w = w;
    m_empty.resize(w, ' ');
    m_widemsg.clear();

    for(const std::string& str : m_msg)
    {
        for(int i = 0; i * w < str.length(); ++i)
        {
            m_widemsg.push_back(str.substr(i*w, w));
        }

        m_widemsg.back().resize(w, ' ');
    }

    ++m_dirtyness;
}

const std::string& LuaConsoleModel::getWideMsg(int index) const
{
    if(index < 0) index = m_widemsg.size() + index;
    if(index < 0 || index >= m_widemsg.size()) return m_empty;

    return m_widemsg[index];
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
    m->echo(luaL_checkstring(L, 1));
    return 0;
}

static int ConsoleModel_gc(lua_State * L)
{
    LuaConsoleModel * m = *static_cast<LuaConsoleModel**>(lua_touserdata(L, 1));
    if(m) m->disarmLuaPointer();
    return 0;
}

bool LuaConsoleModel::setL(lua_State * L)
{
    //TODO: add support for more L's being linked/using echos at once??
    this->L = L;
    if(L)
    {
        LuaConsoleModel ** ptr = static_cast<LuaConsoleModel**>(lua_newuserdata(L, sizeof (LuaConsoleModel*)));
        (*ptr) = this;
        setLuaPointer(ptr);

        //make and set new metatable with gc in it
        lua_newtable(L); //table
        lua_pushliteral(L, "__gc");
        lua_pushcfunction(L, &ConsoleModel_gc);
        lua_settable(L, -3); //table[gc]=ConsoleModel_gc
        lua_setmetatable(L, -2);

        lua_pushcclosure(L, &ConsoleModel_echo, 1);
        lua_setglobal(L, "echo");
        if(luaL_dofile(L, kInitFilename) == LUA_OK)
        {
            return lua_toboolean(L, -1);
        }
        else
        {
            echo(lua_tostring(L, -1));
            return true; //crapped up init is important so show console right away
        }
    }
    return false;
}

void LuaConsoleModel::tryComplete()
{
    std::vector<std::string> possible; //possible matches
    std::string last;

    prepareHints(L, m_lastline, last);
    const bool normalhints = collectHints(L, possible, last, false);
    const bool hasmetaindex = luaL_getmetafield(L, -1, "__index");
    const bool metahints = hasmetaindex && collectHints(L, possible, last, false);
    if(!(normalhints || metahints))
    {
        //if all else fails, assume we want _any_ completion and use global table
        lua_pushglobaltable(L);
        collectHints(L, possible, last, false);
    }

    lua_settop(L, 0); //pop all trash we put on the stack

    if(possible.size() > 1)
    {
        std::string msg = possible[0];
        for(int i = 1; i < possible.size(); ++i)
        {
            msg += " " + possible[i];
        }
        echo(msg);
    }
    else if(possible.size() == 1)
    {
        //m_lastline.erase(m_lastline.size() - last.size());
        m_lastline += possible[0].substr(last.size());
        ++m_dirtyness;
        moveCursor(kCursorEnd);
    }
}

} //lua
