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

#include <LuaConsole/LuaPointerOwner.hpp>

struct lua_State;

namespace blua {

//type of callback function, use this as hooks to implement own history, etc.
//void pointer is user data that you pass when setting the hook, it can be null
//will not be touched at all, just passed to you along with 'this' of calling LuaConsoleModel
class LuaConsoleModel;
typedef void (*CallbackFunc)(LuaConsoleModel*, void*);


//type used to pass color of lines, both internally and in one of public echo functions
typedef std::basic_string<unsigned> ColorString;


namespace priv {

//internal structure to hold line of text and line of assigned colors

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

} //priv


//constants to move by to get to end or start of prompt line, for use with home/end keys
const int kCursorHome = -100000;
const int kCursorEnd = 100000;

//constants to scroll to first or last line in scrollLines
const int kScrollLinesEnd = 1000000000;
const int kScrollLinesBegin = -1000000000;

//console options passed at construction, they are immutable for lifetime of console
//to simplify code, so - when in doubt - opt out

enum ECONSOLE_OPTION
{
    ECO_HISTORY = 1, //load and save history in plaintext file - luaconsolehistory.txt
    ECO_INIT = 2, //load init file - luaconsoleinit.lua
    ECO_START_VISIBLE = 4, //start visible, this will likely get overwritten by init and so on


    //keep last:
    ECO_DEFAULT = 7, //do all of these helpful things above
    ECO_NONE = 0 //do none of the helpful things, ALL is up to user now
};


//specific purpose colors of the console

enum ECONSOLE_COLOR
{
    ECC_ERROR = 0, //color of lua errors, default red
    ECC_HINT = 1, //color of hints, default green
    ECC_CODE = 2, //color of code typed into console, default yellow
    ECC_ECHO = 3, //color of echo'd text, default white
    ECC_PROMPT = 4, //color of the prompt line text, default white
    ECC_TITLE = 5, //color of the title, default white
    ECC_FRAME = 6, //color of the frame, default darkgrey
    ECC_BACKGROUND = 7, //color of the background, default halfcyan - 0x007f7f7f which is half of cyan
    ECC_CURSOR = 8, //color of the cursor, default cyan
    ECC_EVAL = 9, //color of evals, default darkgrey
    
    ECONSOLE_COLOR_COUNT //count, keep last
};

        
//types of console callbacks

enum ECALLBACK_TYPE
{
    //called AFTER new line is in history but BEFORE passing it to lua state
    //so even if it crashes, destroys the process, etc. you can get and save it
    //use it as hook for own history handling
    ECT_NEWHISTORY = 0,


    ECALLBACK_TYPE_COUNT //count, keep last
};

//for use instead of bool in moveCursorOneWord

enum EMOVE_DIRECTION
{
    EMD_RIGHT,
    EMD_LEFT
};

//a single UTF-32 character* in console, with its' color
//*so far it can only be an ascii char but this might change

class ScreenCell
{
public:
    unsigned Char;
    unsigned Color;

};

class LuaConsoleModel
{
public:
    //GENERAL API://////////////////////////////////////////////////////////////

    //get the console attached to this state, might return null, relatively typesafe
    static LuaConsoleModel * getFromRegistry(lua_State * L);

    //as above, but lua errors instead of returning null
    static LuaConsoleModel * checkFromRegistry(lua_State * L);

    //ctor, does NOT allocate lua state, takes a bitflag of ECONSOLE_OPTION values
    LuaConsoleModel(unsigned options = ECO_DEFAULT);

    //dtor
    ~LuaConsoleModel();

    //sets the lua state and attaches console to it, unattaches from last state
    //if not called or called with null console will print errors on usage
    void setL(lua_State * L);

    //print line to console in the default echo color (see ECC_ECHO)
    //this is accessible from lua too, as 'echo' and takes single string there too
    void echo(const std::string& str);

    //print line to console with all characters in the specified color
    void echoColored(const std::string& str, unsigned textcolor);

    //print line to console, with each character having a color set individually
    //if there are more they are ignored, if there are less, the rest are printed
    //in default echo color (see ECC_ECHO)
    void echoLine(const std::string& str, const ColorString& colors);

    //get the title, by default console has empty ("") title
    const std::string& getTitle() const;

    //set title of console, itll appear in top frame bar and be in ECC_TITLE color
    void setTitle(const std::string& title);

    //set callback for certain events, see ECALLBACK_TYPE enum for more info
    //void is userdata and will not be touched, only passed to your func
    //use this to implement own history, etc.
    void setCallback(ECALLBACK_TYPE type, CallbackFunc func, void * data);

    //set whether or not console is visible, invisible console shouldnt process
    //any input and shouldn't draw
    void setVisible(bool visible);

    //check whether or not the conosle is visible
    bool isVisible() const;

    //toggle visibility of the console, equivalent to setVisible(!isVisible())
    void toggleVisible();

    //set one of the console colors, see ECONSOLE_COLOR enum for more info
    void setColor(ECONSOLE_COLOR which, unsigned color);

    //get one of the console colors
    unsigned getColor(ECONSOLE_COLOR which) const;

    //set whether or not pressing enter with prompt empty runs the last line again
    //by default this is enabled, if you want to send empty line with this enabled
    //then send a single space instead
    void setEnterRepeatLast(bool eer);

    //check whether or not pressing enter with empty prompt repeats last history command
    bool getEnterRepeatLast() const;

    //set new size of history buffer
    void setHistorySize(std::size_t newsize);

    //set certain item of history buffer to a value
    void setHistoryItem(std::size_t index, const std::string& item);

    //get current size of history buffer
    std::size_t getHistorySize() const;

    //get content of certain item of history buffer
    const std::string& getHistoryItem(std::size_t index) const;

    //load history from a file, this uses ifstream and loads last history size
    //lines from the file if you want own fs handling, different behaviour, etc.
    //then use get/set history item/size
    bool loadHistoryFromFile(const std::string& filename);

    //save history to file, this uses ofstream and either appends all lines to
    //specified file or overwrites the file and fills it with the history lines
    //bool append controls whether to append or overwrite
    void saveHistoryToFile(const std::string& filename, bool append = true);

    //set characters that jumping over words won't consider part of a word
    //see moveCursorOneWord for explanation about what exactly happens then
    //PS: this is set to a sane default for lua
    void setSkipCharacters(const std::string& chars);

    //get characters that are not considered parts of words when jumping words
    //see moveCursorOneWord for explanation about what exactly happens then
    //PS: this is set to a sane default for lua
    const std::string& getSkipCharacters() const;
    
    //set whether or not all code typed by hand should print its' return
    //values to console, this is on by default
    //it means that where there are return values (even if they are nil) it
    //will print them to console with ECC_EVAL color
    //(when there are no values (0) there is no printing done)
    void setPrintEval(bool print);
    
    //check whether or not evals print to console
    bool getPrintEval() const;


    //API FOR CONTROLLER:///////////////////////////////////////////////////////

    //move cursor by given amount of characters, itll be clipped to [0,lastlinesize]
    //automatically, pass kCursorEnd or kCursorHome to go to lastline or 0 respectively
    //use this for Home/End key press and for Left/Right arrow keys
    void moveCursor(int move);

    //scroll lines by given amount, this is clipped to never scroll past first
    //and last line, use this for ctrl + pageup/pagedown/up/down/home/end
    //pass kScrollLinesEnd or kScrollLinesBegin to go as much back/forth as possible
    void scrollLines(int amount);

    //move cursor by one word left or right (see EMOVE_DIRECTION) like
    //bash (at least KDE and xfce terminals) does, that is:
    //when moving left, skip a word and land on its' first char
    //when moving right, skip a word and land just after its' last char
    //if there is no word to skip then cursor jumps to end(right) or start(left) of prompt line
    void moveCursorOneWord(EMOVE_DIRECTION move);

    //change the index of history and set last line to currently selected item
    //if index is equal to history size then we set an empty line
    //use this (pass -1 and 1) for Up/Down arrows to get bash-like history behavior
    void readHistory(int change);

    //send last line to lua state (or print error), incomplete chunks are handled OK too
    //use this for Enter/Return key press
    void parseLastLine();

    //add character to last line, use this for normal typing, etc.
    void addChar(char c);

    //delete character before cursor, use this for Backspace key
    void backspace();

    //delete character under cursor, use this for Delete key
    void del();

    //try and complete code or print hints (or errors) based on what is available
    //in current lua state and what is in the last prompt line, use this for Tab
    void tryComplete();


    //API FOR VIEW://///////////////////////////////////////////////////////////

    //get dirtyness, if this is different (usually it's more, but just testing for
    //difference is better) than last time checked, then something has changed
    unsigned getDirtyness()const;

    //get current position of cursor in the prompt line
    int getCurPos() const;

    //get the screen buffer, its' size is 80x24, 80 columns, 24 lines, first 80
    //elements are first line, next 80 are second line, etc.
    //there are no newlines in it so dont look for them
    const ScreenCell * getScreenBuffer() const;

private:
    ScreenCell * getCells(int x, int y) const;
    const std::string& getWideMsg(int index) const;
    const ColorString& getWideColor(int index) const;
    void updateBuffer() const;
    void printLuaStackInColor(int first, int last, unsigned color);

    CallbackFunc m_callbackfuncs[ECALLBACK_TYPE_COUNT]; //callbakcs called on certain events
    void * m_callbackdata[ECALLBACK_TYPE_COUNT]; //data for callbacks
    unsigned m_dirtyness; //our current dirtyness
    mutable unsigned m_lastupdate; //when was last update of buffer
    std::string m_lastline; //the prompt line, colorless
    int m_cur; //position of cursor in last line
    std::string m_buffcmd; //command buffer for uncompleted chunks
    lua_State * L; //lua state we are talking with
    std::vector<std::string> m_history; //the history buffer
    int m_hindex; //index in history
    std::vector<priv::ColoredLine> m_msg; //actual messages that got echoed
    int m_w; //width of console, not counting the borders
    std::vector<priv::ColoredLine> m_widemsg; //messages adjusted/split to fit width of console
    const priv::ColoredLine m_empty; //empty line constant
    const unsigned m_options; //options passed at construction
    bool m_visible; //are we visible?
    unsigned m_colors[ECONSOLE_COLOR_COUNT]; //colors of various kinds of text
    bool m_emptyenterrepeat; //should pressing enter with empty prompt repeat last line?
    mutable ScreenCell m_screen[80 * 24]; //screen buff = chars && colors --make this adjustable?
    std::string m_title; //title of the console
    LuaPointerOwner<LuaConsoleModel> m_luaptr; //the lua pointer of ours that handles two way deletions
    std::string m_skipchars; //characters we don't consider part of a word when jumping over words
    int m_firstmsg; //offset of first message - for scrolling
    bool m_printeval; //do we print returned values of handtyped scripts?

};

} //blua

#endif	/* LUACONSOLEMODEL_HPP */

