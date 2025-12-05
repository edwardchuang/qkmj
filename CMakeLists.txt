cmake_minimum_required(VERSION 3.10)
project(qkmj C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Include directories for source code
include_directories(
    ${CMAKE_SOURCE_DIR}/qkmjclient
    ${CMAKE_SOURCE_DIR}/qkmjserver
)

# Check for ncursesw first, then ncurses
find_package(Curses)
include(CheckIncludeFile)

if(CURSES_FOUND)
    pkg_check_modules(NCURSESW IMPORTED_TARGET ncursesw)
    if(NCURSESW_FOUND)
        set(CURSES_LINK_LIB PkgConfig::NCURSESW)
        add_definitions(-D_XOPEN_SOURCE_EXTENDED)
        
        # Check headers using the include dirs from pkg-config
        set(CMAKE_REQUIRED_INCLUDES ${NCURSESW_INCLUDE_DIRS})
    else()
        set(CURSES_LINK_LIB ${CURSES_LIBRARIES})
        include_directories(${CURSES_INCLUDE_DIRS})
        set(CMAKE_REQUIRED_INCLUDES ${CURSES_INCLUDE_DIRS})
    endif()

    check_include_file("ncursesw/curses.h" HAVE_NCURSESW_CURSES_H)
    check_include_file("ncurses/ncurses.h" HAVE_NCURSES_NCURSES_H)
    check_include_file("ncurses/curses.h" HAVE_NCURSES_CURSES_H)
    check_include_file("ncurses.h" HAVE_NCURSES_H)
    check_include_file("curses.h" HAVE_CURSES_H)

    if(HAVE_NCURSESW_CURSES_H)
        add_definitions(-DHAVE_NCURSESW_CURSES_H)
    elseif(HAVE_NCURSES_NCURSES_H)
        add_definitions(-DHAVE_NCURSES_NCURSES_H)
    elseif(HAVE_NCURSES_CURSES_H)
        add_definitions(-DHAVE_NCURSES_CURSES_H)
    elseif(HAVE_NCURSES_H)
        add_definitions(-DHAVE_NCURSES_H)
    elseif(HAVE_CURSES_H)
        add_definitions(-DHAVE_CURSES_H)
    endif()
endif()

# Find other dependencies
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
if(CJSON_FOUND)
    set(CMAKE_REQUIRED_INCLUDES ${CJSON_INCLUDE_DIRS})
    check_include_file("cjson/cJSON.h" HAVE_CJSON_CJSON_H)
    check_include_file("cJSON.h" HAVE_CJSON_H)
    
    if(HAVE_CJSON_CJSON_H)
        add_definitions(-DHAVE_CJSON_CJSON_H)
    elseif(HAVE_CJSON_H)
        add_definitions(-DHAVE_CJSON_H)
    endif()
endif()

pkg_check_modules(MONGOC REQUIRED IMPORTED_TARGET mongoc2)
pkg_check_modules(BSON REQUIRED IMPORTED_TARGET bson2)

# Math library
find_library(MATH_LIB m)

# Crypt library
find_library(CRYPT_LIB crypt)
if(NOT CRYPT_LIB)
    set(CRYPT_LIB "")
endif()

# --- mjgps Target ---
add_executable(mjgps
    qkmjserver/mongo.c
    qkmjserver/mjgps.c
    qkmjserver/mjgps_mongo_helpers.c
)

target_link_libraries(mjgps
    PkgConfig::CJSON
    PkgConfig::MONGOC
    PkgConfig::BSON
    ${MATH_LIB}
    ${CRYPT_LIB}
)

# --- qkmj Target ---
add_executable(qkmj
    qkmjclient/card.c
    qkmjclient/check.c
    qkmjclient/checkscr.c
    qkmjclient/chkmake.c
    qkmjclient/command.c
    qkmjclient/input.c
    qkmjclient/message.c
    qkmjclient/misc.c
    qkmjclient/qkmj.c
    qkmjclient/screen.c
    qkmjclient/socket.c
)

target_link_libraries(qkmj
    ${CURSES_LINK_LIB}
    PkgConfig::CJSON
    ${MATH_LIB}
)

# --- mongo_test Target ---
add_executable(mongo_test
    qkmjserver/mongo_test.c
    qkmjserver/mongo.c
)

target_link_libraries(mongo_test
    PkgConfig::CJSON
    PkgConfig::MONGOC
    PkgConfig::BSON
)

# --- mjrec Target ---
add_executable(mjrec
    qkmjserver/mjrec.c
    qkmjserver/mongo.c
    qkmjserver/mjgps_mongo_helpers.c
)

target_link_libraries(mjrec
    PkgConfig::CJSON
    PkgConfig::MONGOC
    PkgConfig::BSON
    ${CRYPT_LIB}
)
