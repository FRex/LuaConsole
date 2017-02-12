#include <LuaConsole/LuaConsoleModel.hpp>
#include <LuaConsole/LuaHeader.hpp>
#include <LuaConsole/LuaCompletion.hpp>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <fstream>

namespace blua {

//name of metatable/type in registry that our console is using
const char * const kMetaname = "bla_LuaConsole";

//frame/cursor/special unicode chars:
const unsigned kVerticalBarChar = 0x2550u;
const unsigned kHorizontalBarChar = 0x2551u;
const unsigned kULFrameChar = 0x2554u;
const unsigned kBLFrameChar = 0x255au;
const unsigned kBRFrameChar = 0x255du;
const unsigned kURFrameChar = 0x2557u;

//how many history items to keep by default 
const int kDefaultHistorySize = 100;

//how many messages(not wide) to keep, this is for internal workings of console mostly
const int kMessagesKeptCount = 3000;

const char * const kHistoryFilename = "luaconsolehistory.txt";
const char * const kInitFilename = "luaconsoleinit.lua";

//default skipable chars, very sane for lua and similar to bash
const char * const kDefaultSkipChars = " ,.;()[]{}:'\"";

//check if c is in skipchars

inline static bool isSkipChar(char c, const char * skipchars)
{
    return 0x0 != std::strchr(skipchars, c);
}

static int findFirstSkipCharAfterNonskip(const std::string& line, const char *skips, int iter, int start)
{
    bool gotnonskip = false;
    for(int i = start + iter; 0 <= i && i<static_cast<int>(line.size()); i += iter)
    {
        if(gotnonskip)
        {
            if(isSkipChar(line[i], skips))
                return i;
        }
        else
        {
            if(!isSkipChar(line[i], skips))
                gotnonskip = true;
        }
    }//for
    return -1;
}

//this is the best way to do it when we assume 1 console per lua state
static int TheLightKey;

inline static void * getLightKey()
{
    return &TheLightKey;
}

LuaConsoleModel * LuaConsoleModel::getFromRegistry(lua_State* L)
{
    //get our console from the registry
    lua_pushlightuserdata(L, getLightKey());
    lua_gettable(L, LUA_REGISTRYINDEX);

    //if we fail we return null
    LuaConsoleModel * ret = 0x0;

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

LuaConsoleModel* LuaConsoleModel::checkFromRegistry(lua_State* L)
{
    LuaConsoleModel * ret = getFromRegistry(L);
    if(!ret)
        luaL_error(L, "LuaConsole not attached to this state");

    return ret;
}

LuaConsoleModel::LuaConsoleModel(unsigned options) :
m_dirtyness(1u), //because 0u is what view starts at
m_lastupdate(0u),
m_cur(1),
L(0x0),
m_empty(),
m_options(options),
m_visible(options & ECO_START_VISIBLE),
m_emptyenterrepeat(true),
m_skipchars(kDefaultSkipChars),
m_firstmsg(0),
m_printeval(true),
m_addreturn(true),
m_commentcommands(true),
m_lastlineoffset(0u)
{
    setConsoleSize(80u, 24u);

    m_colors[ECC_ERROR] = 0xff0000ff;
    m_colors[ECC_HINT] = 0x00ff00ff;
    m_colors[ECC_CODE] = 0xffff00ff;
    m_colors[ECC_ECHO] = 0xffffffff;
    m_colors[ECC_PROMPT] = 0xffffffff;
    m_colors[ECC_TITLE] = 0xffffffff;
    m_colors[ECC_FRAME] = 0xa9a9a9ff;
    m_colors[ECC_BACKGROUND] = 0x007f7f7f;
    m_colors[ECC_CURSOR] = 0x00ffffff;
    m_colors[ECC_EVAL] = 0xa9a9a9ff;
    m_colors[ECC_HISTORY] = 0xb8860bff;

    //always give sane history size default, even if not asked for reading it
    setHistorySize(kDefaultHistorySize);

    //read history from file if desired
    if(m_options & ECO_HISTORY)
        loadHistoryFromFile(kHistoryFilename);

    m_hindex = m_history.size();

    for(int i = 0; i < ECALLBACK_TYPE_COUNT; ++i)
    {
        m_callbackdata[i] = 0x0;
        m_callbackfuncs[i] = 0x0;
    }
}

LuaConsoleModel::~LuaConsoleModel()
{
    //save history to file if desired, append
    if(m_options & ECO_HISTORY)
        saveHistoryToFile(kHistoryFilename, false);
}

void LuaConsoleModel::moveCursor(int move)
{
    m_cur += move;
    m_cur = std::max<int>(m_cur, 1);
    m_cur = std::min<int>(m_lastline.size() + 1, m_cur);
    ensureCurInView();
    ++m_dirtyness;
}

void LuaConsoleModel::scrollLines(int amount)
{
    m_firstmsg += amount;

    //below code ensures we go no further than last or first line
    m_firstmsg = std::max<int>(m_firstmsg, (m_height - 3) - static_cast<int>(m_widemsg.size()));
    m_firstmsg = std::min(m_firstmsg, 0);
    ++m_dirtyness;
}

void LuaConsoleModel::moveCursorOneWord(EMOVE_DIRECTION move)
{
    const int iter = (move == EMD_LEFT)?-1:1;

    //see below about why we do 'm_cur - 1' not just 'm_cur'
    int targ = findFirstSkipCharAfterNonskip(m_lastline, m_skipchars.c_str(), iter, m_cur - 1);

    //if target is -1 we failed, so just move to home or end, as needed
    if(targ == -1)
    {
        moveCursor(move == EMD_LEFT?kCursorHome:kCursorEnd);
    }
    else
    {
        //add 1 if we moved left, we wanna be on first char of word we just skipped
        if(move == EMD_LEFT)
            ++targ;

        //unfortunate 'hax' of m_Cur being in 'screen space', not 'line space'
        //so 0th char in line is 1 in m_cur, so we add 1
        m_cur = targ + 1;
        ensureCurInView();
        ++m_dirtyness;
    }
}

void LuaConsoleModel::readHistory(int change)
{
    const bool waspromp = static_cast<std::size_t>(m_hindex) == m_history.size();

    m_hindex += change;
    m_hindex = std::max<int>(m_hindex, 0);
    m_hindex = std::min<int>(m_hindex, m_history.size());

    if(static_cast<std::size_t>(m_hindex) == m_history.size())
    {
        //if we came back from history, swap last line in
        if(!waspromp)
            std::swap(m_lastline, m_savedlastline);

        m_lastlineoffset = 0u;
        moveCursor(kCursorEnd);
    }
    else
    {
        //if we just entered history, swap out last line
        if(waspromp)
            std::swap(m_lastline, m_savedlastline);

        m_lastline = m_history[m_hindex];
        m_lastlineoffset = 0u;
        moveCursor(kCursorEnd);
    }

    ++m_dirtyness;
}

void LuaConsoleModel::printLuaStackInColor(int first, int last, unsigned color)
{
    std::stringstream ss;
    for(int i = first; i <= last; ++i)
    {
        //we pretty print just the way tostring lua call does
        switch(lua_type(L, i))
        {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                ss << lua_tostring(L, i);
                break;
            case LUA_TBOOLEAN:
                ss << (lua_toboolean(L, i)?"true":"false");
                break;
            case LUA_TNIL:
                ss << "nil";
                break;
            default:
                ss << luaL_typename(L, i) << ": " << lua_topointer(L, i);
                break;
        } //switch lua type i
        ss << ' ';
    }//for i = first to last
    echoColored(ss.str(), color);
}

//NOTE: we can't do dostring here because we would confuse runtime and parse
//errors then, in example:
//"a .. 10" when a is nil will fail at runtime with return added because it
//tries to concat nil but it DOES parse, so the error should be about nil a
//but if we did dostring we would go on (since return added version failed)
//and try "a .. 10" itself which would would fail to parse so the error would
//be parse error which is wrong, because we want concat error from return
//added version then

bool LuaConsoleModel::tryEval(bool addreturn)
{
    if(addreturn)
    {
        const std::string code = "return " + m_buffcmd;
        if(BLA_LUA_OK == luaL_loadstring(L, code.c_str()))
        {
            return true;
        }
        else
        {
            lua_pop(L, 1); //pop error - it doesn't matter with added return
            return false;
        }
    } //if addreturn
    else
    {
        return BLA_LUA_OK == luaL_loadstring(L, m_buffcmd.c_str());
    }
}

ELINE_PARSE_RESULT LuaConsoleModel::parseLastLine()
{
    ELINE_PARSE_RESULT ret = ELPR_OK;
    if(m_lastline.size() == 0u && m_emptyenterrepeat && !m_history.empty())
        m_lastline = m_history.back();

    echoColored(m_lastline, m_colors[ECC_CODE]);

    //always push and then remove first, other way around wouldn't be safe
    //just make our history always be x lines and do it that way instead of old
    //way, no need for 'maxhistory' variable or anything
    m_history.push_back(m_lastline);
    m_history.erase(m_history.begin());

    //to 'cancel out' previous history browsing
    m_hindex = m_history.size();

    //call before running, in case crash, exit etc.
    if(m_callbackfuncs[ECT_NEWHISTORY])
        m_callbackfuncs[ECT_NEWHISTORY](this, m_callbackdata[ECT_NEWHISTORY]);

    //if we are not midchunk then this code is fresh
    const bool freshcode = m_buffcmd.empty();
    m_buffcmd += m_lastline;
    m_buffcmd += '\n';

    if(L)
    {
        const int oldtop = lua_gettop(L);
        bool evalok;
        if(m_addreturn)
        {
            evalok = tryEval(true) || tryEval(false);
        }
        else
        {
            evalok = tryEval(false);
        }
        if(evalok && BLA_LUA_OK == lua_pcall(L, 0, LUA_MULTRET, 0))
        {
            m_buffcmd.clear(); //worked & done - clear it
            if(m_printeval && oldtop != lua_gettop(L))
                printLuaStackInColor(oldtop + 1, lua_gettop(L), m_colors[ECC_EVAL]);
        }
        else
        {
            std::size_t len;
            const char * err = lua_tolstring(L, -1, &len);
            ret = ELPR_MORE;
            if(!blua::incompleteChunkError(err, len))
            {
                m_buffcmd.clear(); //failed normally - clear it
                echoColored(err, m_colors[ECC_ERROR]);
                ret = evalok?ELPR_RUNTIME_ERROR:ELPR_PARSE_ERROR;
            }
            lua_pop(L, 1);
        }//got an error, real or <eof>/incomplete chunk one
    }//L is not null
    else
    {
        //say kindly we are kind of in trouble
        echoColored("Lua state pointer is NULL, commands have no effect", m_colors[ECC_ERROR]);
        ret = ELPR_NO_LUA;
    }//L is null

    //if this line was freshcode and cmd commands feature is enabled, check it
    if(freshcode && m_commentcommands)
        checkSpecialComments();

    m_lastline.clear();
    m_cur = 1;
    m_lastlineoffset = 0u;
    ++m_dirtyness;
    return ret;
}

void LuaConsoleModel::checkSpecialComments()
{
    if(m_lastline == "--clear")
        clearScreen();

    if(m_lastline == "--history")
        for(std::size_t i = 0u; i < m_history.size(); ++i)
            echoColored(m_history[i], m_colors[ECC_HISTORY]);

    if(m_lastline == "--quit")
        if(m_callbackfuncs[ECT_QUIT])
            m_callbackfuncs[ECT_QUIT](this, m_callbackdata[ECT_QUIT]);
}

void LuaConsoleModel::addChar(char c)
{
    if(c < ' ' || c >= 127)
        return;

    m_lastline.insert(m_lastline.begin() + (m_cur - 1), c);
    ++m_cur;
    ensureCurInView();
    ++m_dirtyness;
}

void LuaConsoleModel::backspace()
{
    if(m_cur > 1)
    {
        --m_cur;
        m_lastline.erase(m_cur - 1, 1);
        ensureCurInView();
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

static std::size_t pushWideMessages(const priv::ColoredLine& str, std::vector<priv::ColoredLine>* widemsgs, unsigned width)
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
                priv::ColoredLine line;
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
            priv::ColoredLine line;
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

    priv::ColoredLine line;
    line.Text = str;
    line.Color = colors;
    line.resizeColorToFitText(m_colors[ECC_ECHO]);

    m_msg.push_back(line);

    pushWideMessages(line, &m_widemsg, m_width - 2);

    if(m_msg.size() > kMessagesKeptCount)
    {
        const std::size_t msgs = pushWideMessages(*m_msg.begin(), 0x0, m_width - 2);
        m_msg.erase(m_msg.begin());
        m_widemsg.erase(m_widemsg.begin(), m_widemsg.begin() + msgs);
    }

    scrollLines(kScrollLinesEnd); //make this conditional?
    ++m_dirtyness;
}

const std::string& LuaConsoleModel::getWideMsg(int index) const
{
    if(index < 0) index = m_widemsg.size() + index;
    index += m_firstmsg;
    if(index < 0 || static_cast<std::size_t>(index) >= m_widemsg.size()) return m_empty.Text;

    return m_widemsg[index].Text;
}

const ColorString& LuaConsoleModel::getWideColor(int index) const
{
    if(index < 0) index = m_widemsg.size() + index;
    index += m_firstmsg;
    if(index < 0 || static_cast<std::size_t>(index) >= m_widemsg.size()) return m_empty.Color;

    return m_widemsg[index].Color;
}

int LuaConsoleModel::getCurPos() const
{
    return m_cur - m_lastlineoffset;
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
        m->setL(0x0);

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
            if(luaL_loadfile(L, kInitFilename) || lua_pcall(L, 0, 1, 0))
            {
                echoColored(lua_tostring(L, -1), m_colors[ECC_ERROR]);
                lua_pop(L, 1); //pop the error message
                m_visible = true; //crapped up init is important so show console right away
            }
            else
            {
                m_visible = lua_toboolean(L, -1);
                lua_pop(L, 1); //pop that boolean
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

    //last line part before and after the cursor
    const std::string lastbeg = m_lastline.substr(0, m_cur - 1);
    const std::string lastend = m_lastline.substr(m_cur - 1);
    priv::prepareHints(L, lastbeg, last);
    if(!priv::collectHints(L, possible, last, false))
    {
        //if no hints, assume we want _any_ completion and use global table
        bla_lua_pushglobaltable(L);
        priv::collectHints(L, possible, last, false);
    }

    lua_settop(L, 0); //pop all trash we put on the stack

    if(possible.size() > 1u)
    {
        const std::string commonprefix = priv::commonPrefix(possible);
        //if no common prefix or if we already have it or more
        if(commonprefix.empty() || commonprefix.size() <= last.size())
        {
            std::string msg = possible[0];
            for(std::size_t i = 1u; i < possible.size(); ++i)
                msg += " " + possible[i];

            echoColored(msg, m_colors[ECC_HINT]);
        }
        else
        {
            const std::string added = commonprefix.substr(last.size());
            m_lastline = lastbeg + added + lastend;
            ++m_dirtyness;
            moveCursor(added.size());
        } //commonprefix is not empty
    }
    else if(possible.size() == 1)
    {
        const std::string added = possible[0].substr(last.size());
        m_lastline = lastbeg + added + lastend;
        ++m_dirtyness;
        moveCursor(added.size());
    }
}

void LuaConsoleModel::setCallback(ECALLBACK_TYPE type, CallbackFunc func, void* data)
{
    if(type == ECALLBACK_TYPE_COUNT)
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
    if(which != ECONSOLE_COLOR_COUNT && m_colors[which] != color)
    {
        m_colors[which] = color;
        ++m_dirtyness;
    }
}

unsigned LuaConsoleModel::getColor(ECONSOLE_COLOR which) const
{
    if(which == ECONSOLE_COLOR_COUNT)
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
    assert(static_cast<unsigned>(x + 1) < m_width);
    assert(0 < y);
    assert(static_cast<unsigned>(y + 1) < m_height);
    return &m_screen[x + m_width * y];
}

const ScreenCell * LuaConsoleModel::getScreenBuffer() const
{
    updateBuffer();
    assert(!m_screen.empty());
    return &m_screen[0];
}

void LuaConsoleModel::setConsoleSize(unsigned width, unsigned height)
{
    if(width < 10u || height < 4u)
        return;

    if(m_width == width && m_height == height)
        return;

    //if width changed then redo wide message
    if(m_width != width)
    {
        m_widemsg.clear();
        for(unsigned i = 0u; i < m_msg.size(); ++i)
            pushWideMessages(m_msg[i], &m_widemsg, width - 2);
    }

    ++m_dirtyness;
    m_width = width;
    m_height = height;
    m_screen.assign(width * height, ScreenCell());

    //work space
    for(unsigned i = 0; i < m_width * m_height; ++i)
    {
        m_screen[i].Char = ' '; //0x2588
        m_screen[i].Color = 0xffffffff;
    }

    //vertical
    for(unsigned i = 0; i < m_width; ++i)
    {
        m_screen[i + m_width * 0].Char = kVerticalBarChar;
        m_screen[i + m_width * (m_height - 1)].Char = kVerticalBarChar;
    }

    //horizontal
    for(unsigned i = 0; i < m_height; ++i)
    {
        m_screen[0 + m_width * i].Char = kHorizontalBarChar;
        m_screen[m_width - 1 + m_width * i].Char = kHorizontalBarChar;
    }

    //corners
    m_screen[0 + m_width * 0].Char = kULFrameChar;
    m_screen[0 + m_width * (m_height - 1)].Char = kBLFrameChar;
    m_screen[m_width - 1 + m_width * (m_height - 1)].Char = kBRFrameChar;
    m_screen[m_width - 1 + m_width * 0].Char = kURFrameChar;
}

unsigned LuaConsoleModel::getConsoleWidth() const
{
    return m_width;
}

unsigned LuaConsoleModel::getConsoleHeight() const
{
    return m_height;
}

void LuaConsoleModel::updateBuffer() const
{
    if(m_lastupdate == m_dirtyness)
        return;

    m_lastupdate = m_dirtyness;

    //first we clear out the top bar
    for(unsigned i = 1; i < m_width - 1; ++i)
        m_screen[i].Char = kVerticalBarChar;

    //then we ensure frame is all OK colored, since setting title overwrites colors
    for(unsigned i = 0; i < m_width; ++i)
    {
        m_screen[i + 0 * m_width].Color = m_colors[ECC_FRAME];
        m_screen[i + (m_height - 1) * m_width].Color = m_colors[ECC_FRAME];
    }
    for(unsigned i = 0; i < m_height - 1; ++i)
    {
        m_screen[0 + i * m_width].Color = m_colors[ECC_FRAME];
        m_screen[m_width - 1 + i * m_width].Color = m_colors[ECC_FRAME];
    }

    //now we can set the title and its' color
    for(int i = 1; i < std::min<int>(m_width - 1, m_title.length() + 1); ++i)
    {
        m_screen[i].Char = m_title[i - 1];
        m_screen[i].Color = m_colors[ECC_TITLE];
    }


    for(unsigned i = 1; i < (m_height - 2); ++i)
    {
        const std::string& l = getWideMsg(i - (m_height - 2));
        const ColorString& c = getWideColor(i - (m_height - 2));

        ScreenCell * a = getCells(1, i);

        for(unsigned x = 0; x < (m_width - 2); ++x)
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

    ScreenCell * a = getCells(1, m_height - 2);

    for(unsigned x = 0; x < (m_width - 2); ++x)
    {
        a[x].Char = ' ';
        a[x].Color = m_colors[ECC_PROMPT];
    }

    for(std::size_t x = 0; x < (m_width - 2) && (m_lastlineoffset + x) < m_lastline.size(); ++x)
    {
        a[x].Char = m_lastline[m_lastlineoffset + x];
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

void LuaConsoleModel::setHistorySize(std::size_t newsize)
{
    m_history.resize(newsize);
}

void LuaConsoleModel::setHistoryItem(std::size_t index, const std::string& item)
{
    if(index < m_history.size())
        m_history[index] = item;
}

std::size_t LuaConsoleModel::getHistorySize() const
{
    return m_history.size();
}

const std::string& LuaConsoleModel::getHistoryItem(std::size_t index) const
{
    if(index < m_history.size())
        return m_history[index];

    return m_empty.Text;
}

bool LuaConsoleModel::loadHistoryFromFile(const std::string& filename)
{
    std::ifstream file(filename.c_str());
    if(!file.is_open())
        return false;

    if(getHistorySize() == 0u)
        return false;

    std::vector<std::string> buf(getHistorySize());
    std::string line;
    std::size_t iter = 0u;

    while(std::getline(file, line))
    {
        buf[iter % getHistorySize()] = line;
        ++iter;
    }

    if(iter == 0u)
        return false;

    for(std::size_t i = 0; i < getHistorySize(); ++i)
        m_history[i] = buf[(iter + i) % getHistorySize()];

    return true;
}

void LuaConsoleModel::saveHistoryToFile(const std::string& filename, bool append)
{
    std::ofstream file(filename.c_str(), append?std::ios::app:std::ios::trunc);
    for(std::size_t i = 0u; i < getHistorySize(); ++i)
        file << getHistoryItem(i) << std::endl;
}

void LuaConsoleModel::setSkipCharacters(const std::string& chars)
{
    m_skipchars = chars;
}

const std::string& LuaConsoleModel::getSkipCharacters() const
{
    return m_skipchars;
}

void LuaConsoleModel::setPrintEval(bool print)
{
    m_printeval = print;
}

bool LuaConsoleModel::getPrintEval() const
{
    return m_printeval;
}

void LuaConsoleModel::setAddReturn(bool add)
{
    m_addreturn = add;
}

bool LuaConsoleModel::getAddReturn() const
{
    return m_addreturn;
}

void LuaConsoleModel::setCommentCommands(bool enable)
{
    m_commentcommands = enable;
}

bool LuaConsoleModel::getCommentCommands() const
{
    return m_commentcommands;
}

void LuaConsoleModel::clearScreen()
{
    m_firstmsg = 0;
    m_msg.clear();
    m_widemsg.clear();
}

void LuaConsoleModel::ensureCurInView()
{
    if(static_cast<unsigned>(m_cur) <= m_lastlineoffset)
    {
        m_lastlineoffset = m_cur - 1;
    }
    while(static_cast<unsigned>(m_cur) > m_lastlineoffset + m_width - 2)
    {
        ++m_lastlineoffset;
    }
}

} //blua
