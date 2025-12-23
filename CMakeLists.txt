cmake_minimum_required(VERSION 3.10)
project(qkmj C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

# Get the latest git commit hash
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if(NOT GIT_COMMIT_HASH)
  set(GIT_COMMIT_HASH "unknown")
else()
  # Check if there are local changes
  execute_process(
    COMMAND git diff --quiet
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE IS_DIRTY
    ERROR_QUIET
  )
  if(IS_DIRTY)
    set(GIT_COMMIT_HASH "${GIT_COMMIT_HASH}-dirty")
  endif()
endif()

# Pass the hash to the compiler as a definition
add_definitions("-DGIT_HASH=\"${GIT_COMMIT_HASH}\"")

# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Include directories for source code
include_directories(
    ${CMAKE_SOURCE_DIR}/qkmjclient
    ${CMAKE_SOURCE_DIR}/qkmjserver
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
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

# Find CURL
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# Find Mongo C Driver
pkg_search_module(MONGOC REQUIRED IMPORTED_TARGET libmongoc-1.0 mongoc-1.0 mongoc2)
pkg_search_module(BSON REQUIRED IMPORTED_TARGET libbson-1.0 bson-1.0 bson2)

# Math library
find_library(MATH_LIB m)

# Crypt library
find_library(CRYPT_LIB crypt)
if(NOT CRYPT_LIB)
    set(CRYPT_LIB "")
endif()

# --- mjgps Target ---
add_executable(mjgps
    qkmjserver/mjgps.c
    qkmjserver/mongo.c
    qkmjserver/mjgps_mongo_helpers.c
    qkmjserver/logger.c
    qkmjserver/session_manager.c
    qkmjclient/protocol.c
)

target_link_libraries(mjgps
    ${MATH_LIB}
    ${CRYPT_LIB}
    PkgConfig::MONGOC
    PkgConfig::BSON
    PkgConfig::CJSON
)

# --- mjrec Target ---
add_executable(mjrec
    qkmjserver/mjrec.c
    qkmjserver/mongo.c
    qkmjserver/mjgps_mongo_helpers.c
)

target_link_libraries(mjrec
    PkgConfig::MONGOC
    PkgConfig::BSON
    PkgConfig::CJSON
)

# --- qkmj Target ---
add_executable(qkmj
    qkmjclient/ai_client.c
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
    qkmjclient/protocol.c
)

target_link_libraries(qkmj
    ${CURSES_LINK_LIB}
    ${MATH_LIB}
    PkgConfig::CJSON
    ${CURL_LIBRARIES}
)

# --- Tests ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_ai
    tests/test_ai_client.cpp
    tests/mock_qkmj.c
    qkmjclient/ai_client.c
    qkmjclient/protocol.c
)

target_include_directories(test_ai PRIVATE 
    ${CMAKE_SOURCE_DIR}/qkmjclient 
    ${CMAKE_SOURCE_DIR}/qkmjserver
    ${CJSON_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIR}
)

target_link_libraries(test_ai
    gtest_main
    PkgConfig::CJSON
    ${CURL_LIBRARIES}
)

add_test(NAME AIClientTest COMMAND test_ai)

add_executable(conn_test
    tests/connection_test.c
    qkmjclient/ai_client.c
    qkmjclient/protocol.c
)

target_include_directories(conn_test PRIVATE
    ${CMAKE_SOURCE_DIR}/qkmjclient 
    ${CMAKE_SOURCE_DIR}/qkmjserver 
    ${CJSON_INCLUDE_DIRS} 
    ${CURL_INCLUDE_DIR}
)

target_link_libraries(conn_test
    PkgConfig::CJSON
    ${CURL_LIBRARIES}
)

add_executable(session_test
    tests/test_session_manager.cpp
    qkmjserver/session_manager.c
    qkmjserver/mongo.c
    qkmjserver/logger.c
    # We include this just in case mongo.c or logger.c has implicit deps, though not strictly used by session_manager
    qkmjserver/mjgps_mongo_helpers.c 
)

target_include_directories(session_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/qkmjserver 
    ${CMAKE_SOURCE_DIR}/qkmjclient 
    ${CJSON_INCLUDE_DIRS}
)

target_link_libraries(session_test
    gtest_main
    PkgConfig::MONGOC
    PkgConfig::BSON
    PkgConfig::CJSON
    ${MATH_LIB}
)

add_test(NAME SessionManagerTest COMMAND session_test)

