<!--
╔═════════════════════════════════════════════════════════════════════════════╗
║                        IMMUTABLE GOVERNANCE BLOCK                           ║
║           Этот блок не изменяется ни Claude, ни Codex, ни кем-либо          ║
║                      кроме владельца проекта                                ║
╠═════════════════════════════════════════════════════════════════════════════╣
║                                                                             ║
║  ИЕРАРХИЯ РЕШЕНИЙ:                                                          ║
║    1. Владелец            — абсолютный приоритет, решения безоговорочны     ║
║    2. Claude              — архитектурные и технические решения             ║
║    3. Codex               — исполнение конкретных задач по готовому ТЗ      ║
║                                                                             ║
║  ПРАВИЛА ДЛЯ CODEX:                                                         ║
║    - Codex не проектирует архитектуру и не меняет структуру классов         ║
║    - Codex получает конкретную задачу ("реализуй X вот так") и выполняет    ║
║    - При конфликте решений Claude и Codex — приоритет у Claude              ║
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
║     Базовые параметры проекта — не изменяются без решения владельца          ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  Проект:      NetPulse Monitor                                               ║
║  Тип:         Графический дашборд мониторинга сети (Linux)                   ║
║  Платформа:   Linux, требуется root (uid=0)                                  ║
║  Язык:        C++20                                                          ║
║  Графика:     SFML 3.x   (рендеринг ТОЛЬКО в главном потоке)                 ║
║  Сборка:      CMake                                                          ║
║  Зависимости: libcurl (HTTP API), libpcap (перехват пакетов)                 ║
║  Окно:        Resizable, заголовок "NetPulse Monitor"                        ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
-->

# NetPulse Monitor — AI Context Document

## Архитектура

```
ApplicationController  (main loop, SFML window, events, thread coordination)
         │
 ┌───────┼───────┐
 │       │       │
Data   Render  Shared
Layer  Layer   Data
```

<!-- LLM Claude: обновлено описание классов, добавлен статус реализации M2:: -->

## Data Layer

### `IDataProvider` — абстрактный интерфейс
Базовый класс для всех провайдеров. Два метода: `fetch()` — вызывается из Data-потока по таймеру, `stop()` — сигнал остановки. Все провайдеры наследуют этот интерфейс и регистрируются в `ApplicationController`.

### `SystemInfoProvider`
Версии и статусы: OpenSSH, Docker, OpenSSL, rfkill, NetworkManager, dhclient, systemd, iptables/nftables. Источник: `popen()` с жёстким whitelist команд. Три состояния: активно / неактивно / не установлено.

### `PacketSnifferProvider`
Перехват пакетов через libpcap (root). Порты 80, 443, 8080 и др. Парсит HTTP-заголовки из TCP-потока. Поля записи: метод, путь, timestamp, src IP:port. Пишет в `ThreadSafeQueue` для передачи в рендер.

### `ExternalAPIProvider`
HTTPS через libcurl к `2ip.io`, `ipapi.co`. Запрос при старте + каждые 5–10 мин. Результат кэшируется. Работает в отдельном API-потоке — main thread не блокирует. Отдаёт: IP, провайдер, город/страна.

### `NetworkDeviceProvider`
Читает `/proc/net/arp`. Поля: IP, MAC, интерфейс, статус. Ping по клику — опционально.

### `ConnectionProvider`
Читает `/proc/net/tcp` и `/proc/net/udp`. Парсит hex через `NetworkUtils`. Поля: локальный адрес, удалённый адрес, статус (ESTABLISHED / LISTEN / TIME_WAIT). Данные идут в `ConnectionVisualizer`.

---

## Render Layer

### `IRenderer` — абстрактный интерфейс
Один метод: `draw(sf::RenderWindow&)`. Вызывается только из главного потока.

### `DashboardRenderer`
Компонует и рисует все пять блоков. Читает из `DataManager`. Знает расположение, размеры, цветовые схемы блоков.

### `ConnectionVisualizer`
Граф подключений. Центр = локальная машина, линии = соединения из `ConnectionProvider`. Цвет линии: зелёный (ESTABLISHED), жёлтый (LISTEN), серый (TIME_WAIT). Анимация частиц = трафик. Клик = детали.

### `UI::Panel`
Прямоугольник с фоном и рамкой. Базовый контейнер блоков дашборда.

### `UI::TextBlock`
Текст моноширинным шрифтом с цветом. Используется в логах, списках, статусах.

### `UI::Button`
Интерактивный элемент с hover/click. Используется для опциональных действий (например, ping в блоке Network Devices).

---

## Shared Data

### `ThreadSafeQueue<T>` — header-only шаблон
`std::mutex` + `std::condition_variable`. `push()` из Data/API потоков, `pop()` блокирующий из Render. `stop()` разблокирует ожидающие `pop()`. Живёт только в `.hpp` — требование C++ для шаблонов.

### `DataManager` — header-only
Централизованное хранилище. Render читает, Data пишет — всё под `std::mutex`. Поля расширяются по мере реализации блоков.

---

## Utils

### `RootCheck` — header-only
`namespace RootCheck { inline bool isRoot() }` — обёртка над `getuid() == 0`. Namespace нужен чтобы не конфликтовать с другими `isRoot` в проекте или библиотеках. Вызывается первой в `main()`.

### `Logger`
`Log::info()`, `Log::warn()`, `Log::error()` — вывод в stdout с меткой уровня.

### `NetworkUtils`
`hexToIP()` и `hexToPort()` — парсинг hex-строк из `/proc/net/tcp` и `/proc/net/udp`. Используется в `ConnectionProvider`.

---

## App

### `Config.hpp` — все константы проекта
```
WINDOW_WIDTH/HEIGHT = 1280x720 (начальный размер, окно resizable)
TARGET_FPS          = 60
WINDOW_TITLE        = "NetPulse Monitor"
BG_R/G/B            = 18,18,18  (тёмный фон)
API_REFRESH_SEC     = 300.0     (5 мин)
DATA_REFRESH_SEC    = 2.0
REQUEST_LOG_LIMIT   = 100
```

### `ApplicationController`
Владеет окном (`sf::Style::Default` = resizable). Главный цикл: `processEvents → update → render`. Закрытие: крестик или Escape. Шрифт (`DejaVuSansMono`) ищется по системным путям при старте.

Пока провайдеры не реализованы — `renderPlaceholders()` рисует 5 блоков-заглушек с названиями. Заглушки пересчитываются под текущий размер окна каждый кадр — тянутся при resize.

---

## Структура проекта

```
NetPulse/
├── CMakeLists.txt
├── llms.md
├── README.md
├── .gitignore
├── assets/fonts/
├── cmake/
├── include/
│   ├── App/
│   │   ├── ApplicationController.hpp
│   │   └── Config.hpp
│   ├── Data/
│   │   ├── IDataProvider.hpp
│   │   ├── SystemInfoProvider.hpp
│   │   ├── PacketSnifferProvider.hpp
│   │   ├── ExternalAPIProvider.hpp
│   │   ├── NetworkDeviceProvider.hpp
│   │   └── ConnectionProvider.hpp
│   ├── Render/
│   │   ├── IRenderer.hpp
│   │   ├── DashboardRenderer.hpp
│   │   ├── ConnectionVisualizer.hpp
│   │   └── UI/
│   │       ├── Panel.hpp
│   │       ├── TextBlock.hpp
│   │       └── Button.hpp
│   └── Utils/
│       ├── ThreadSafeQueue.hpp
│       ├── DataManager.hpp
│       ├── Logger.hpp
│       ├── NetworkUtils.hpp
│       └── RootCheck.hpp
└── src/
    ├── main.cpp
    ├── App/ApplicationController.cpp
    ├── Data/
    │   ├── SystemInfoProvider.cpp
    │   ├── PacketSnifferProvider.cpp
    │   ├── ExternalAPIProvider.cpp
    │   ├── NetworkDeviceProvider.cpp
    │   └── ConnectionProvider.cpp
    ├── Render/
    │   ├── DashboardRenderer.cpp
    │   ├── ConnectionVisualizer.cpp
    │   └── UI/
    │       ├── Panel.cpp
    │       ├── TextBlock.cpp
    │       └── Button.cpp
    └── Utils/
        ├── Logger.cpp
        ├── NetworkUtils.cpp
        └── RootCheck.cpp
```

---

## Блоки данных

**Блок 1 — System Info:** утилиты через `popen()`. Цвета: активно / неактивно / не установлено.

**Блок 2 — Request Log:** libpcap, скользящее окно 50–100 записей. GET=зелёный, POST=красный, PUT=жёлтый, DELETE=синий, PATCH=оранжевый, OPTIONS/HEAD=фиолетовый, UNKNOWN=серый.

**Блок 3 — External IP:** libcurl → `2ip.io` / `ipapi.co`. IP, провайдер, город/страна. Кэш + таймер.

**Блок 4 — Network Devices:** `/proc/net/arp`. IP, MAC, интерфейс, статус.

**Блок 5 — Connection Visualization:** `/proc/net/tcp` + `/proc/net/udp`. Граф, цвет по статусу, частицы, клик = детали.

---

## Технические требования

**Потоки:**
- Thread 1 (Main): SFML рендеринг + события
- Thread 2 (Data): опрос провайдеров по таймеру
- Thread 3 (API): libcurl запросы
- Синхронизация: `ThreadSafeQueue` + `DataManager` с `std::mutex`

**Безопасность:** whitelist для `popen()`, никаких `system()` с вводом, валидация внешних данных, таймауты для HTTP.

**Root:** `RootCheck::isRoot()` — первое в `main()`, до любой инициализации.

---

## Визуальный стиль

Тёмный фон `#121212`. Моноширинный шрифт (DejaVuSansMono). Схемы: Gruvbox или Material Design Green. Анимации: частицы для трафика, пульсация для активных подключений.

---

## Milestones

| # | Задача | Статус |
|---|---|---|
| M1 | CMake + SFML + libcurl + libpcap | ✅ |
| M2 | Root-проверка + базовое окно SFML | ✅ |
| M3 | Блок 1: System Info | ⬜ |
| M4 | Блок 4: Network Devices (ARP) | ⬜ |
| M5 | Блок 5: Connection Visualization | ⬜ |
| M6 | Блок 3: External IP (libcurl) | ⬜ |
| M7 | Блок 2: Packet Sniffer (libpcap) | ⬜ |
| M8 | UI-полировка, анимации | ⬜ |
| M9 | README, документация, демо | ⬜ |