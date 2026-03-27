# Changelog

## v1.0.0 - 2026-03-27

### Added
- Request-log interface selector with clickable per-interface filtering.
- Connection visualizer display modes: `ALL`, `TCP`, `UDP`, `STATE`.
- Column headers for `System Info`, `Request Log`, and `Network Devices`.
- Hover summary for `Network Devices`.

### Changed
- Reworked dashboard layout so `Request Log` uses the full lower-left area.
- Moved `External IP` under `Network Devices`.
- Increased default and minimum window size for the current desktop layout.
- Refined panel styling: rounded containers, cleaner typography, monochrome surfaces, larger selectors.
- Normalized inner content surfaces so table/list padding is consistent across blocks.
- Adjusted `System Info` height to show the full service list again.
- Reworked `Connection Visualizer` state grouping so status sectors scale with actual connection counts.
- Removed header divider bars to simplify the final release UI.

### Fixed
- Resize-related layout breakage and overlapping panel content.
- `External IP` content clipping against the inner container.
- Visualizer header collisions caused by top statistics chips.
- Missing system-tool rows caused by the reduced `System Info` panel height.

### Notes
- `Connection Visualizer` still keeps its color palette; the rest of the dashboard was intentionally pushed toward a quieter monochrome UI.
- `AGENTS.md` milestone table was not updated in this release because the project rules mark that file as immutable for Codex.
