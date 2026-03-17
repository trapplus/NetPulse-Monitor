#pragma once
#include <unistd.h>

namespace RootCheck {
    // Возвращает true если процесс запущен от root (uid=0)
    inline bool isRoot() { return ::getuid() == 0; }
}
