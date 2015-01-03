#BLuaConsole
This is a simple but powerful terminal to use with the [Lua](http://www.lua.org) language.

The main classes behind all of the functionality except for input and rendering use only C++98 features and a common subset of 5.1/Jit and 5.2 Lua API.

There is a demo and an implementation of input and rendering for SFML 2.1 provided in this repo - one class for each of these tasks, four file total.


##Features:
* Easy to interface with any font rendering and key input
* Works with both Lua 5.2 and Lua 5.1, including LuaJIT
* (Optionally) Loads and runs an init script from luaconsoleinit.lua
* (Optionally) Loads and saves commands history from luaconsolehistory.txt
* Allows loading and saving commands history from plaintext file or setting each line directly (for custom filesystems etc.)
* Allows completing or hinting possible completions based on what is in the prompt line and in the Lua state currently
* Automatically checks if entered chunk of code is not complete and catches lines entered from prompt untill a full chunk is ready, just like standalone commandline Lua does
* Allows colorful text in console for different kinds of messages and comes with sane defaults for errors, code, hints, etc.
* Allows echoing to console, including colored text: both colored per line and colored per character
* Exports a single 'echo()' function, that echos single string in default echo color, to state it is attached to
* Puts itself into the registry table, using a pointer to private global int as light userdata key, and provides a way to get pointer to itself (or null if it's not in this Lua state or was reset to another one already) in a typesafe way
* Well commented out API and code

See the LuaConsoleModel.hpp and the comments above each function of the API for full list of features.

###Feedback
1. Forum thread on SFML forum: [link](http://en.sfml-dev.org/forums/index.php?topic=15962.0)
2. Creating an issue in this repo.
3. Forum thread on gamedev.net: [link](http://www.gamedev.net/topic/662186-bluaconsole/)


###Demo
There is a well commented demo of basic usage (with the only input and renderer provided so far - SFML 2.1) in main.cpp


###Key config
These are the key controls for SFML input but they are inspired by bash, gdb, etc. so they are a good set of keys and shortcuts to use both in this implementation and in future ones. There is nothing stopping you from changing them or writing an input class that allows customizing the key bindings. It's actually very easy to do that.

* Characters - put characters in the prompt line
* Backspace - delete characters before cursor
* Delete - delete characters under cursor
* Enter - send code from prompt
* Left - move one character to left
* Right - move one character to right
* End - move cursor to end of prompt line
* Home - move cursor to begining of prompt line
* Up - move one line back in history
* Down - move one line forward in history
* Tab - trigger code completion
* Ctrl + Left - jump a word left
* Ctrl + Right - jump  a word left

###Licensing
It's licensed under MIT license, see LICENSE file.
