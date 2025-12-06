## QKMJ Refactoring Project

Welcome to QKMJ Refactoring Project, you're responsible to refactoring this project, that written in 30 years ago in C language, to fulfill modern C language standard such as C17 and C23 conventions and best practices. This system use CMake and trying to make it portable. 

Your main goal is keep this program up and running and keep concise and tidy. 

## Command Lines 

  You may use `cmake -S . -B build && cmake --build build` to compile the project.
  Ensure you have `pkg-config` installed and it can find `libcjson`, `libmongoc-1.0`, and `libbson-1.0`.

## Prelimiary Goals

This is a traditional Client/Server architecture system. Your first assignment is understand the flow of this system, and corresponding command and parameters for communicating between client and server, analysis and understand then write it down into COMMANDS.md, with both English and Traditional Chinese (Taiwan)

For example, once client connected to server, what packet are sent to server and what messages are returned to the client, etc.

## Core Tasks

This is a very large system, with tens of thousand lines of C code, and the system is tightly coupled so please keep in mind and try refactor the source code one-by-one, otherwise you'll get confused if you mixed all of them.

Each task will be considered successfully completed once passing compile without any errors and warnings. Expect those lines indicating "GEMINI: DO-NOT-FIX"

1. ** Use UTF-8 encoding instead of Big-5**
   * Especially ncurses library and related function calls. try consider ncursesw if applicable **

2. ** Address security concerns and flaws **
   * Prevent buffer overflow by using strncpy to replace strcpy, strncmp to replace strcmp, snprintf to replace sprintf, etc. * Avoid strcat for efficiency.
   * use memmove for memory copying

3. ** Improvement on program performance **
   * do not use goto, avoid redundant loops, rewrite cleverly

4. ** Make the source code easy to maintain and read**
   * Break large functions into smaller ones
   * Follow Google coding style
   
5. ** Comment policy **
   * Use Traditional Chinese (Taiwan) as the main comment language

6. ** Error Handling **
   * Apply error handlings

7. ** Cast properly**

8. ** Rename the variable with understandable names**

### Your Role in Development

When asked to modify the action, you should:

1.  **Analyze the Request:** Understand how the requested change impacts the CMake build system, the shell scripts, and the documentation.
2.  **Plan Your Changes:** Propose a plan that includes modifications to all relevant files.
3.  **Implement and Verify:** Make the changes and ensure the action still functions as expected. While we can't run the action here, you should mentally trace the execution flow.
4.  **Update Documentation:** Ensure the `README.md` and any relevant examples are updated to reflect your changes.

## Useful Commands

### Build Release Version
To build a Release version (optimized, no debug symbols, no debug pause):
```bash
cmake -DCMAKE_BUILD_TYPE=Release -B build_release
cmake --build build_release
```
The executable will be located at `build_release/qkmj`.
