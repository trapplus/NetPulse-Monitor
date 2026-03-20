#pragma once
#include <unistd.h>  // explicit - getuid() is POSIX, never comes transitively

// simple namespace wrapper so we dont pollute global scope with isRoot
// called first thing in main() before any initialization
namespace RootCheck {
    inline bool isRoot() { return ::getuid() == 0; }
}
