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

class LuaConsoleCallbacks;

class LuaConsoleModel : public LuaPointerOwner<LuaConsoleModel>
{
    friend class LuaConsole;
public:
    void moveCursor(int move);
    void readHistory(int change);
    void parseLastLine();
    void addChar(char c);
    void backspace();
    void del();
    unsigned getDirtyness()const;
    void echo(const std::string& str);
    const std::string& getWideMsg(int index) const;
    const std::string& getLastLine() const;
    int getCurPos() const;
    void tryComplete();
    const std::vector<std::string>& getHistory() const;
    void setHistory(const std::vector<std::string>& history);
    void setCallbacks(LuaConsoleCallbacks * callbacks);

private:
    LuaConsoleModel(unsigned options);
    ~LuaConsoleModel();
    void setWidth(std::size_t w);
    bool setL(lua_State * L);

    //for renderer catching:
    unsigned m_dirtyness;

    std::string m_lastline;
    int m_cur;

    std::string m_buffcmd;
    lua_State * L;

    std::vector<std::string> m_history;
    int m_hindex;

    std::vector<std::string> m_msg;

    int m_w;
    std::vector<std::string> m_widemsg;

    const std::string m_empty;

    LuaConsoleCallbacks * m_callbacks;
    unsigned m_options;

};

}

#endif	/* LUACONSOLEMODEL_HPP */

