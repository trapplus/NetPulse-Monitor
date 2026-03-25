# Changelog

## v0.2.0 - 2026-03-25

### Added
- Extracted a shared connection DTO into `ConnectionInfo`, so the render layer no longer depends on `ConnectionProvider` headers directly.
  Ref: [`f6f05ad`](https://github.com/trapplus/NetPulse-Monitor/commit/f6f05ad)

### Changed
- Centralized runtime and UI constants in `Config.hpp`: refresh intervals, HTTP timeouts, layout offsets, panel spacing, font paths, and color palette now live in one place as `inline constexpr`.
  Ref: [`e6b9472`](https://github.com/trapplus/NetPulse-Monitor/commit/e6b9472)
- Refactored `ApplicationController` to use separate worker threads for local data refresh and external API refresh, reducing coupling between render timing and network I/O.
  Ref: [`e6b9472`](https://github.com/trapplus/NetPulse-Monitor/commit/e6b9472)
- Removed debug panel prefixes like `[ Block 1 ]` and kept only clean panel titles in the dashboard UI.
  Ref: [`e6b9472`](https://github.com/trapplus/NetPulse-Monitor/commit/e6b9472)
- Reworked `ExternalAPIProvider` access to return a single snapshot instead of multiple getters, and added explicit libcurl global init and request timeouts for safer runtime behavior.
  Ref: [`e6b9472`](https://github.com/trapplus/NetPulse-Monitor/commit/e6b9472)
- Moved more rendering constants out of implementation files and into config, including panel colors, text colors, tooltip styling, and connection graph sizing.
  Ref: [`e6b9472`](https://github.com/trapplus/NetPulse-Monitor/commit/e6b9472)

### Build / Cleanup
- Removed an unnecessary `CMAKE_C_COMPILER` override from `Makefile`.
  Ref: [`253561d`](https://github.com/trapplus/NetPulse-Monitor/commit/253561d)
- Removed the unused `RootCheck.cpp` translation unit because `RootCheck` is header-only.
  Ref: [`253561d`](https://github.com/trapplus/NetPulse-Monitor/commit/253561d)
- Trimmed unnecessary includes in render-related headers and `DataManager`.
  Ref: [`253561d`](https://github.com/trapplus/NetPulse-Monitor/commit/253561d)

### Included From Previous Merged Work
- External IP provider integration and block 3 rendering.
  Ref: [`fdbb749`](https://github.com/trapplus/NetPulse-Monitor/commit/fdbb749), PR merge [`af48f40`](https://github.com/trapplus/NetPulse-Monitor/commit/af48f40)
- Connection visualizer polish for block 5, including hover tooltip and protocol counters.
  Ref: [`72ad32a`](https://github.com/trapplus/NetPulse-Monitor/commit/72ad32a), PR merge [`17bf5dc`](https://github.com/trapplus/NetPulse-Monitor/commit/17bf5dc)
