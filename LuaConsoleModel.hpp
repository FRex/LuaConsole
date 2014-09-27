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
#include "LuaConsoleCommon.hpp"

namespace lua {

class LuaConsoleCallbacks;

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

enum ECONSOLE_COLOR
{
    ECC_ERROR = 0, //color of lua errors, default red
    ECC_HINT = 1, //color of hints, default green
    ECC_CODE = 2, //color of code typed into console, default yellow
    ECC_ECHO = 3, //color of echo'd text, default white

    ECC_COUNT //count, keep last
};

class ScreenCell
{
public:
    unsigned Char;
    unsigned Color;

};

class LuaConsoleModel : public LuaPointerOwner<LuaConsoleModel>
{
public:
    static LuaConsoleModel * getFromRegistry(lua_State * L);

    LuaConsoleModel(unsigned options = ECO_DEFAULT);
    ~LuaConsoleModel();
    void setWidth(std::size_t w);
    void setL(lua_State * L);
    void moveCursor(int move);
    void readHistory(int change);
    void parseLastLine();
    void addChar(char c);
    void backspace();
    void del();
    unsigned getDirtyness()const;
    void echo(const std::string& str);
    void echoColored(const std::string& str, unsigned textcolor);
    void echoLine(const std::string& str, const ColorString& colors);
    const std::string& getWideMsg(int index) const;
    const ColorString& getWideColor(int index) const;
    const std::string& getLastLine() const;
    int getCurPos() const;
    void tryComplete();
    const std::vector<std::string>& getHistory() const;
    void setHistory(const std::vector<std::string>& history);
    void setCallbacks(LuaConsoleCallbacks * callbacks);
    void setVisible(bool visible);
    bool isVisible() const;
    void toggleVisible();
    void setColor(ECONSOLE_COLOR which, unsigned color);
    unsigned getColor(ECONSOLE_COLOR which) const;
    void setEnterRepeatLast(bool eer);
    bool getEnterRepeatLast() const;

    ScreenCell * getScreenBuffer() const;
    
private:
    ScreenCell * getCells(int x, int y) const;
    void updateBuffer() const;
    
    //for renderer catching:
    unsigned m_dirtyness;
    mutable unsigned m_lastupdate;

    std::string m_lastline;
    int m_cur;

    std::string m_buffcmd;
    lua_State * L;

    std::vector<std::string> m_history;
    int m_hindex;

    std::vector<ColoredLine> m_msg;

    int m_w;
    std::vector<ColoredLine> m_widemsg;

    const ColoredLine m_empty;

    LuaConsoleCallbacks * m_callbacks;
    unsigned m_options;

    bool m_visible;

    unsigned m_colors[ECC_COUNT];

    bool m_emptyenterrepeat;

    mutable ScreenCell m_screen[80 * 24]; //make this adjustable?

};

}

#endif	/* LUACONSOLEMODEL_HPP */

