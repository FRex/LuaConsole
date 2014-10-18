/* 
 * File:   LuaConsoleModel.hpp
 * Author: frex
 *
 * Created on July 19, 2014, 11:34 PM
 */

#ifndef LUACONSOLEMODEL_HPP
#define	LUACONSOLEMODEL_HPP

#include <string>
#include <vector>

#include "LuaPointerOwner.hpp"

struct lua_State;

namespace lua {

class LuaConsoleModel;
typedef void (*CallbackFunc)(LuaConsoleModel*, void*);
typedef std::basic_string<unsigned> ColorString;

class ColoredLine
{
public:
    std::string Text;
    ColorString Color;

    void resizeColorToFitText(unsigned fill)
    {
        Color.resize(Text.size(), fill);
    }

};

//constants to move by to get to end or start of prompt line
const int kCursorHome = -100000;
const int kCursorEnd = 100000;

enum ECONSOLE_OPTIONS
{
    ECO_HISTORY = 1, //load and save history in plaintext file - luaconsolehistory.txt
    ECO_INIT = 2, //load init file - luaconsoleinit.lua
    ECO_START_VISIBLE = 4, //start visible, this will likely get overwritten by init and so on

    //keep last:
    ECO_DEFAULT = 7, //do all of these helpful things above
    ECO_NONE = 0 //do none of the helpful things, ALL is up to user now
};

enum ECONSOLE_COLOR
{
    ECC_ERROR = 0, //color of lua errors, default red
    ECC_HINT = 1, //color of hints, default green
    ECC_CODE = 2, //color of code typed into console, default yellow
    ECC_ECHO = 3, //color of echo'd text, default white
    ECC_PROMPT = 4, //color of the prompt line text, default white
    ECC_TITLE = 5, //color of the title, default white
    ECC_FRAME = 6, //color of the title, default darkgrey

    ECC_COUNT //count, keep last
};

enum ECALLBACK_TYPE
{
    ECT_NEWHISTORY = 0,

    ECT_COUNT //count, keep last
};

class ScreenCell
{
public:
    unsigned Char;
    unsigned Color;

};

class LuaConsoleModel
{
public:
    //general api:
    static LuaConsoleModel * getFromRegistry(lua_State * L);
    LuaConsoleModel(unsigned options = ECO_DEFAULT);
    ~LuaConsoleModel();
    void setL(lua_State * L);
    void echo(const std::string& str);
    void echoColored(const std::string& str, unsigned textcolor);
    void echoLine(const std::string& str, const ColorString& colors);
    const std::string& getTitle() const;
    void setTitle(const std::string& title);
    const std::vector<std::string>& getHistory() const;
    void setHistory(const std::vector<std::string>& history);
    void setCallback(ECALLBACK_TYPE type, CallbackFunc func, void * data);
    void setVisible(bool visible);
    bool isVisible() const;
    void toggleVisible();
    void setColor(ECONSOLE_COLOR which, unsigned color);
    unsigned getColor(ECONSOLE_COLOR which) const;
    void setEnterRepeatLast(bool eer);
    bool getEnterRepeatLast() const;


    //api for controller:
    void moveCursor(int move);
    void readHistory(int change);
    void parseLastLine();
    void addChar(char c);
    void backspace();
    void del();
    void tryComplete();


    //api for view:
    unsigned getDirtyness()const;
    int getCurPos() const;
    const ScreenCell * getScreenBuffer() const;

private:
    ScreenCell * getCells(int x, int y) const;
    const std::string& getWideMsg(int index) const;
    const ColorString& getWideColor(int index) const;
    const std::string& getLastLine() const;
    void updateBuffer() const;


    CallbackFunc m_callbackfuncs[ECT_COUNT]; //callbakcs called on certain events
    void * m_callbackdata[ECT_COUNT]; //data for callbacks
    unsigned m_dirtyness; //our current dirtyness
    mutable unsigned m_lastupdate; //when was last update of buffer
    std::string m_lastline; //the prompt line, colorless
    int m_cur; //position of cursor in last line
    std::string m_buffcmd; //command buffer for uncompleted chunks
    lua_State * L; //lua state we are talking with
    std::vector<std::string> m_history; //the history buffer
    int m_hindex; //index in history
    std::vector<ColoredLine> m_msg; //actual messages that got echoed
    int m_w; //width of console, not counting the borders
    std::vector<ColoredLine> m_widemsg; //messages adjusted/split to fit width of console
    const ColoredLine m_empty; //empty line constant
    const unsigned m_options; //options passed at construction
    bool m_visible; //are we visible?
    unsigned m_colors[ECC_COUNT]; //colors of various kinds of text
    bool m_emptyenterrepeat; //should pressing enter with empty prompt repeat last line?
    mutable ScreenCell m_screen[80 * 24]; //screen buff = chars && colors --make this adjustable?
    std::string m_title; //title of the console
    LuaPointerOwner<LuaConsoleModel> m_luaptr; //the lua pointer of ours that handles two way deletions

};

}

#endif	/* LUACONSOLEMODEL_HPP */

