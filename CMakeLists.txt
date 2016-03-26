project(LuaConsole)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(SFML 2 COMPONENTS graphics window system REQUIRED)
find_package(Lua REQUIRED)

include_directories(${SFML_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/src
                    ${LUA_INCLUDE_DIR})

file(GLOB HEADERS RELATIVE ${PROJECT_SOURCE_DIR} "include/LuaConsole/*")
file(GLOB SOURCES RELATIVE ${PROJECT_SOURCE_DIR} "src/LuaConsole/*")

add_library(luaconsole ${HEADERS} ${SOURCES})

add_executable(example "examples/main.cpp")
target_link_libraries(example luaconsole
                              ${LUA_LIBRARIES}
                              ${SFML_LIBRARIES})

install(TARGETS luaconsole DESTINATION ./lib)
install(DIRECTORY "include" DESTINATION .)

install(TARGETS example DESTINATION .)
install(FILES DejaVuSansMono.ttf
              luaconsoleinit.lua
              luaconsolehistory.txt
              DESTINATION ./share/LuaConsole)

