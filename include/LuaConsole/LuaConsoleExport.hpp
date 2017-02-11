#ifndef LUACONSOLEEXPORT_HPP
#define LUACONSOLEEXPORT_HPP

#ifdef _WIN32
    #ifdef LUACONSOLE_EXPORTS
        #define LUACONSOLEAPI __declspec(dllexport)
    #else
        #define LUACONSOLEAPI __declspec(dllimport)
    #endif
#endif

#endif LUACONSOLEEXPORT_HPP
