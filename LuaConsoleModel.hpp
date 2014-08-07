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

class LuaConsoleModel : public LuaPointerOwner<LuaConsoleModel>
{
public:
    LuaConsoleModel();
    ~LuaConsoleModel();

    void moveCursor(int move);
    void readHistory(int change);
    void parseLastLine();
    void addChar(char c);
    void backspace();
    void del();
    unsigned getDirtyness()const;
    void echo(const std::string& str);
    void setWidth(std::size_t w);
    const std::string& getWideMsg(int index) const;
    const std::string& getLastLine() const;
    int getCurPos() const;
    bool setL(lua_State * L);
    void tryComplete();

private:

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

};

}

#endif	/* LUACONSOLEMODEL_HPP */

