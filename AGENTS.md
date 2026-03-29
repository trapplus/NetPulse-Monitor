<!--
╔═════════════════════════════════════════════════════════════════════════════╗
║                        IMMUTABLE GOVERNANCE BLOCK                           ║
║           Этот блок не изменяется ни Claude, ни Codex, ни кем-либо          ║
║                      кроме владельца проекта                                ║
╠═════════════════════════════════════════════════════════════════════════════╣
║                                                                             ║
║  ИЕРАРХИЯ РЕШЕНИЙ:                                                          ║
║    1. Владелец            - абсолютный приоритет, решения безоговорочны     ║
║    2. Claude              - архитектурные и технические решения             ║
║    3. Codex               - исполнение конкретных задач по готовому ТЗ      ║
║                                                                             ║
║  ПРАВИЛА ДЛЯ CODEX:                                                         ║
║    - Codex не проектирует архитектуру и не меняет структуру классов         ║
║    - Codex получает конкретную задачу ("реализуй X вот так") и выполняет    ║
║    - При конфликте решений Claude и Codex - приоритет у Claude              ║
║    - Codex не редактирует этот файл                                         ║
║                                                                             ║
║  ПРАВИЛА ДЛЯ CLAUDE:                                                        ║
║    - Claude не редактирует этот (IMMUTABLE GOVERNANCE) блок                 ║
║    - Claude не редактирует блок IMMUTABLE PROJECT SPEC ниже                 ║
║    - Изменения в остальных разделах помечаются: LLM Claude: <описание>::    ║
║    - Решения Claude считаются более технически корректными, чем Codex       ║
║                                                                             ║
║  ЦЕЛЬ ФАЙЛА:                                                                ║
║    Единый источник истины для AI-ассистентов. Проект движется               ║
║    в одном направлении, без архитектурных конфликтов.                       ║
║                                                                             ║
╚═════════════════════════════════════════════════════════════════════════════╝
-->

<!--
╔══════════════════════════════════════════════════════════════════════════════╗
║                       IMMUTABLE PROJECT SPEC                                 ║
║     Базовые параметры проекта - не изменяются без решения владельца          ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  Проект:      NetPulse Monitor                                               ║
║  Тип:         Графический дашборд мониторинга сети (Linux)                   ║
║  Платформа:   Linux, требуется root (uid=0)                                  ║
║  Язык:        C++20                                                          ║
║  Компилятор:  clang++ 22.x (Arch Linux)                                      ║
║  STL:         libstdc++ (GCC) - libc++                                       ║
║  LSP/линтер:  clangd (Zed)                                                   ║
║  Графика:     SFML 3.x   (рендеринг ТОЛЬКО в главном потоке)                 ║
║  Сборка:      CMake + Ninja                                                  ║
║  Зависимости: libcurl (HTTP API), libpcap (перехват пакетов)                 ║
║  Окно:        Resizable, заголовок "NetPulse Monitor"                        ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
-->

# NetPulse Monitor - AI Context Document

<!-- LLM Claude: добавлены секции Dev environment tips, Testing instructions, PR instructions по образцу шаблона:: -->

## Dev environment tips

- Run `make build` to configure and compile in one step.
- Run `make clean && make build` if CMake cache seems stale or you changed CMakeLists.txt.
- Use `make run` to build and launch — it handles `xhost +local:` and `sudo -E` automatically.
- Check `build/compile_commands.json` to confirm clangd is picking up the right flags — if it's missing, add `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to the cmake call in Makefile.
- To navigate the source tree quickly: headers live in `include/<Module>/`, implementations in `src/<Module>/` — same subdirectory structure mirrors each other.

## Testing instructions

- Find the CI pipeline in `.github/workflows/build.yml`.
- The CI builds inside `archlinux:latest` Docker — if something passes locally but fails in CI, check that your dependency is listed in the `pacman -S` line in the workflow.
- Before committing, do a clean build locally: `make clean && make build`.
- Fix all compiler warnings before merging — the project runs with `-Wall -Wextra -Wpedantic -Wshadow` and warnings are treated as noise that hides real issues.
- If you add a new source file, add it to `SOURCES` in `CMakeLists.txt` — otherwise it silently won't compile.

## PR instructions

- Tag format for releases: `v<major>.<minor>.<patch>` — pushing a tag triggers the CI release pipeline.
- Always do `make clean && make build` before committing to catch any include or linker issues.
- Do not commit the `build/` directory — it's in `.gitignore`.

---

## Do not

- Do not delete existing comments unless explicitly told to
- Do not touch files unrelated to the assigned task — only modify code responsible for what you were asked to implement
- Exception: you may add  values to  to avoid scattering magic numbers across files
- Do not modify architecture — class structure, file layout, and module boundaries are frozen until the owner says otherwise
- Be technical and precise — the owner is exact in their wording, take it literally
- If a prompt is ambiguous or imprecise, do not guess and do not proceed — ignore it and ask the owner to rephrase

## Правила для AI-ассистентов

### Toolchain
- Компилятор: `clang++ 22.x`, стандарт C++20, STL: `libstdc++`
- LSP: `clangd` - его предупреждения имеют приоритет над догадками модели
- Сборка локально: `make build` (CMake + Ninja)
- Сборка в CI: Docker `archlinux:latest`, те же флаги

### Правила по `#include`
- Каждый используемый символ должен иметь явный `#include` в том файле где используется
- Transitive includes не считаются надёжными - даже если работают на нашей платформе, это деталь реализации которая может измениться
- Исключение явного include не допускается без комментария объясняющего почему
- Системные POSIX заголовки (`<unistd.h>`, `<arpa/inet.h>`, `<pcap.h>`) - всегда явно

**Документирование опциональных includes:**

Когда include транзитивно приходит через другой заголовок и сознательно не дублируется:
```cpp
// <string> comes in via IDataProvider.hpp, no need to re-include
```

Когда include добавлен явно хотя технически транзитивный - объяснить почему:
```cpp
// explicit include - popen/fgets are C stdio, not guaranteed through SFML headers
#include <cstdio>
```

### Правила по комментариям
- Язык: английский, разговорный стиль - как пишет живой человек, не LLM
- Комментировать why и неочевидные решения, не what
- Никакого JavaDoc-стиля, никаких `// This function iterates over the collection`
- Хорошо: `// grab first line and strip the newline`, `// don't block main thread here`
- Плохо: `// This method retrieves the version string of the specified utility`
- Каждый нетривиальный логический блок - минимум одна строка комментария

### Общие правила кода
- Никаких `system()` с пользовательским вводом - только `popen()` с whitelist
- Не блокировать main thread долгими операциями
- SFML: рендеринг и события только в главном потоке
- Потокобезопасность: данные между потоками только через mutex или ThreadSafeQueue
- Smart pointers везде где есть владение (`unique_ptr`, `shared_ptr`)
- `= default` и `= delete` предпочтительнее пустых реализаций

---

## Архитектура

```
ApplicationController  (main loop, SFML window, events, thread coordination)
         │
 ┌───────┼───────┐
 │       │       │
Data   Render  Shared
Layer  Layer   Data
```

## Data Layer

### `IDataProvider` - абстрактный интерфейс
Базовый класс для всех провайдеров. `fetch()` - вызывается из Data-потока по таймеру, `stop()` - сигнал остановки. Все провайдеры регистрируются в `DataManager`.

### `SystemInfoProvider` ✅
Версии и статусы: OpenSSH, Docker, OpenSSL, rfkill, NetworkManager, dhclient, systemd, iptables/nftables. `popen()` с жёстким whitelist команд. Три состояния: Active / Inactive / NotInstalled. Обновление каждые 30 сек.

### `PacketSnifferProvider`
Перехват пакетов через libpcap (root). Порты 80, 443, 8080 и др. Парсит HTTP-заголовки из TCP-потока. Пишет в `ThreadSafeQueue`.

### `ExternalAPIProvider`
HTTPS через libcurl к `2ip.io`, `ipapi.co`. Запрос при старте + каждые 5–10 мин. Кэш. Не блокирует main thread.

### `NetworkDeviceProvider`
Читает `/proc/net/arp`. IP, MAC, интерфейс, статус.

### `ConnectionProvider`
Читает `/proc/net/tcp` и `/proc/net/udp`. Парсит hex через `NetworkUtils`.

---

## Render Layer

### `IRenderer` - абстрактный интерфейс
`draw(sf::RenderWindow&)` - только из главного потока.

### `DashboardRenderer`
Компонует все пять блоков. Читает из `DataManager`.

### `ConnectionVisualizer`
Граф подключений. Цвет линии: зелёный (ESTABLISHED), жёлтый (LISTEN), серый (TIME_WAIT). Анимация частиц. Клик = детали.

### `UI::Panel`, `UI::TextBlock`, `UI::Button`
Базовые UI-компоненты. Button поддерживает hover/click.

---

## Shared Data

### `ThreadSafeQueue<T>` - header-only
`push()` из Data/API потоков, блокирующий `pop()`, `stop()` для завершения.

### `DataManager` - header-only
Владеет провайдерами через `unique_ptr`. Render читает через методы провайдеров под их внутренним mutex.

---

## Utils

### `RootCheck` - header-only
`namespace RootCheck { inline bool isRoot() }` - первое в `main()`.

### `Logger`
`Log::info/warn/error()` - stdout с меткой уровня.

### `NetworkUtils`
`hexToIP()`, `hexToPort()` - парсинг `/proc/net/tcp` и `/proc/net/udp`.

---

## App

### `Config.hpp`
```
WINDOW_WIDTH/HEIGHT = 1280x720  (resizable)
TARGET_FPS          = 60
WINDOW_TITLE        = "NetPulse Monitor"
BG_R/G/B            = 18,18,18
API_REFRESH_SEC     = 300.0
DATA_REFRESH_SEC    = 2.0
REQUEST_LOG_LIMIT   = 100
```

### `ApplicationController`
Главный цикл: `processEvents → update → render`. Владеет `DataManager` и Data-потоком. Деструктор останавливает поток через `m_running = false` + `join()`.

---

## Структура проекта

```
NetPulse/
├── CMakeLists.txt
├── Makefile
├── AGENTS.md
├── README.md
├── .gitignore
├── .github/workflows/build.yml
├── assets/fonts/
├── include/
│   ├── App/    ApplicationController.hpp, Config.hpp
│   ├── Data/   IDataProvider.hpp, *Provider.hpp
│   ├── Render/ IRenderer.hpp, DashboardRenderer.hpp, ConnectionVisualizer.hpp, UI/
│   └── Utils/  ThreadSafeQueue.hpp, DataManager.hpp, Logger.hpp, NetworkUtils.hpp, RootCheck.hpp
└── src/
    ├── main.cpp
    ├── App/    ApplicationController.cpp
    ├── Data/   *Provider.cpp
    ├── Render/ *.cpp, UI/*.cpp
    └── Utils/  Logger.cpp, NetworkUtils.cpp, RootCheck.cpp
```

---

## Блоки данных

**Блок 1 - System Info ✅:** таблица имя | версия | статус. Цвета: зелёный/жёлтый/серый.

**Блок 2 - Request Log:** libpcap, скользящее окно 50–100 записей. GET=зелёный, POST=красный, PUT=жёлтый, DELETE=синий, PATCH=оранжевый, OPTIONS/HEAD=фиолетовый, UNKNOWN=серый.

**Блок 3 - External IP:** libcurl → `2ip.io` / `ipapi.co`. IP, провайдер, город/страна.

**Блок 4 - Network Devices:** `/proc/net/arp`. IP, MAC, интерфейс, статус.

**Блок 5 - Connection Visualization:** граф, цвет по статусу, частицы, клик = детали.

---

## Технические требования

**Потоки:**
- Thread 1 (Main): SFML рендеринг + события
- Thread 2 (Data): опрос провайдеров по таймеру
- Thread 3 (API): libcurl запросы
- Синхронизация: `ThreadSafeQueue` + внутренние mutex провайдеров

**Root:** `RootCheck::isRoot()` - первое в `main()`.

**CI:** Ubuntu runner → Docker `archlinux:latest` → `pacman -Syu` → сборка clang+ninja → Release по тегу `v*`.

---

## Визуальный стиль

Тёмный фон `#121212`. Моноширинный шрифт (DejaVuSansMono). Gruvbox или Material Design Green. Частицы для трафика, пульсация для активных подключений.

---

## Milestones

| # | Задача | Статус |
|---|---|---|
| M1 | CMake + SFML + libcurl + libpcap | ✅ |
| M2 | Root-проверка + базовое окно SFML | ✅ |
| M3 | Блок 1: System Info | ✅ |
| M4 | Блок 4: Network Devices (ARP) | ✅ |
| M5 | Блок 5: Connection Visualization | ✅ |
| M6 | Блок 3: External IP (libcurl) | ✅ |
| M7 | Блок 2: Packet Sniffer (libpcap) | ✅ |
| M8 | UI-полировка, анимации | ✅ |
| M9 | Фикс логических ошибок работы программы и конечный рефакторинг | ✅ |
| M10 | README, документация, демо | ⬜ |
