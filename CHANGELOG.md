# Changelog

## cowbraten 1.1.0-rc.3 — 2026-11-20

The third RmlUi release candidate completes the ordinary legacy-interface migration. Legacy menu declarations are no longer loaded by default, while an explicit compatibility switch and the original files remain available for third-party content and recovery.

### Legacy-interface migration

- Reconstruct configurable `newgui` and Autoexec menus as compact RmlUi overlays instead of redirecting them to unrelated full-screen pages.
- Preserve typed buttons, fields, sliders, checkboxes, progress bars, package images, model previews, tabs, lifecycle hooks, and original engine-owned actions.
- Keep compact menus above the main application shell and allow function-key binds to open them while another native screen is visible.
- Route the engine apply-confirmation dialog through the same native compact-menu adapter.
- Add native loading and map presentation, gameplay status dock, secondary ammo indicators, configurable ammo bar, demo-recording indicator, mode flags/tokens, match score, and frag feedback.
- Add a persistent startup switch for legacy menu scripts. They are disabled by default but remain available as an explicit compatibility fallback.
- Retain the engine-owned spatial radar/minimap and texture palette, early-loading OpenGL fallback, and unknown third-party GUI fallback as intentional compatibility boundaries.

### Demo and interface refinements

- Add an online-only option for automatic demo recording while retaining manual and offline recording behavior when the option is disabled.
- Correct compact-menu layering and close-button alignment.
- Replace enlarged legacy radar blips with dedicated native flag artwork in CTF
  and Hold, and reuse the native skull artwork for carried Collect tokens.
- Restore legacy team colors, FFA scoreboard grouping, spectator score behavior, weapon feedback placement, and game-clock positioning in the native HUD.
- Improve large-text scoreboard columns, first-open interface preferences, and server-browser refresh presentation.
- Restore accented Latin and other legacy Sauer characters across native
  RmlUi surfaces by converting engine text to UTF-8 at the UI boundary,
  including chat, console, Profile, scoreboard, frag feedback, demo playback,
  Administration, the game menu, and the server browser.
- Convert edited Profile names back to Sauer's native encoding before applying
  or saving them, and connect RmlUi text fields to the system clipboard so
  copy and paste work with external applications.
- Add long-session performance telemetry and reusable FFA benchmark scenarios.

### Performance

- Cache unchanged gameplay HUD snapshots so RmlUi properties and layout are not invalidated every rendered frame.
- Skip empty frag-feedback updates and cap passive overlay reconciliation at 120 Hz while keeping interactive menus and rendering uncapped.
- Reduce measured gameplay UI update time from approximately 1.0 ms to 0.013 ms per frame in the Linux validation environment.

### Validation

- Pass the complete interface, routing, loading, HUD, demo, persistence, and performance-analysis test suite.
- Compile the Linux client and standalone server and validate a real FFA runtime session with legacy menu scripts disabled.
- Preserve p1xbraten 9.0.0 detection, the Sauerbraten network protocol, and the standard demo format.

## cowbraten 1.1.0-rc.2 — 2026-11-13

The second RmlUi release candidate. It expands the initial production slice into the normal player flow, adds responsive presentation and customization, and carries the complete 1.0.1 maintenance line. Final 1.1.0 remains gated on migrating or deliberately classifying every interface path that still draws through the legacy GUI.

### RmlUi

- Add the native RmlUi runtime, renderer, input integration, Sauer-backed file loading, shared theme, and typed action routing.
- Migrate Profile, game setup, Servers, Demos, Settings, Administration, About, text utilities, map editing, the in-game menu, chat, full console, and gameplay scoreboard.
- Keep the specialized 3D texture palette engine-owned and retain a safe legacy fallback for unknown third-party `showgui` documents.
- Add server filtering, sorting, selection, refresh, and connection through snapshots of the existing Sauer server list.
- Add typed Settings pages for gameplay, HUD, graphics, audio, mouse and keyboard, crosshair, chat, and console options.
- Add key binding capture with multiple bindings, exact removal, readable key names, conflict detection, and explicit replacement.
- Add RmlUi controls for Cowbraten and p1x features including weapon trails, feedback, radar, player models, clocks, score overlay, ammobar, frag messages, scoreboard columns, console filters, soundpacks, and audio devices.
- Preserve configuration persistence, legacy console handoff, GL reset recovery, drawable-size updates, and exact-name `showgui` compatibility for migrated screens.
- Preserve logical interface size on HiDPI displays with density-independent styling and drawable-resolution rendering.
- Improve responsive layouts at 800x600 and other compact viewports, raise microtext legibility, and simplify user-facing Settings descriptions.
- Normalize every Settings category with compact headings, values, toggles, sliders, and spacing, plus clear weapon grouping for trail-color controls.
- Add independent interface and text scaling, responsive desktop scaling, and selectable Legacy/Lato typography. Legacy loads Sauerbraten's `packages/fonts/default.ttf`, the same face used by classic console and FPS text.
- Let the top-left gameplay feed use the available screen width instead of an arbitrary 40–44% cap, while retaining minimap clearance.

### Player and demo tools

- Add profile name presets, player-model preview, and separate local/reported model controls.
- Add server favorites and a richer server browser with filtering, sorting, stable selection, compatibility status, and refresh/connect actions.
- Rebuild the demo library with metadata, search, classification, favorites, recoverable trash, and playback integration.
- Add demo transport controls and local playback visualization options, including X-RAY and hitbox rendering.
- Add safe autoexec profiles, automatic ignore rules, and typed utilities for Notepad, Scratchpad, and Paste Buffer.
- Add native game setup, campaign and bot-match flows, administration controls, and the map editing control center.

### HUD and presentation

- Add responsive chat, console, HUD, status dock, and scoreboard layouts for compact through ultrawide viewports.
- Add a semantic CC0 icon set and the optional Modern Metallic theme.
- Preserve existing Sauer configuration and bind semantics while applying interface changes live.

### Integrated maintenance work

- Carry forward the 1.0.1 release-candidate fixes for HiDPI rendering, display transitions, kill-streak lifecycle handling, and HD demos.
- Retain p1xbraten 9.0.0 detection and the existing Sauer network and demo format versions.

### Validation

- Build the Linux client and standalone server from the combined branch.
- Produce a Windows NSIS installer, Linux portable ZIP, and self-extracting Linux installer with the required RmlUi assets and SHA-256 checksums.
- Pass the interface regression suite, including app shell, settings, chat, console, demos, editor, game setup, HUD, scoreboard, server browser, and utilities coverage.
- Start the client with SDL dummy audio/offscreen video, initialize both Legacy and Lato faces, render migrated documents, and exit cleanly.

### Compatibility and remaining scope

- Retain p1xbraten 9.0.0 detection and the existing Sauer network and standard demo format versions.
- HD demos remain optional companion `.dmohd` files; normal `.dmo` playback remains supported.
- Cross-platform interactive validation remains ongoing; the documented release validation was performed on Linux.
- Audit runtime and compatibility routes for remaining legacy-drawn elements before the stable promotion. The engine-owned 3D texture palette and unknown third-party `showgui` fallback are intentional boundaries; any other legacy surface must be migrated or documented before 1.1.0 final.

## cowbraten 1.0.1-rc.3 — 2026-10-23

### HD Demos
- Add optional HD recording alongside existing HiFi client demos.
- Store high-precision camera samples in companion `.dmohd` files.
- Add HD playback with configurable interpolation.
- Preserve existing `.dmo` playback and advanced demo controls.
- Add experimental live HD spectating support for compatible L4ZY servers.
- Preserve compatibility with vanilla servers.

### Notes
- Live HD spectating requires further interoperability testing.
- HD server-side recording and relay support is deferred to a future release.

## cowbraten 1.0.1-rc.2 — 2026-09-11

Second release candidate with a client-side kill-streak lifecycle correction.

### Fixes

- Fixed multiplayer kill streaks carrying post-death kill credit into the next life.
- Kill streaks now belong to the confirmed local life opened by spawn and closed by authoritative death and other lifecycle transitions.
- Post-death kills still preserve the server-provided frag score but no longer increment the next-life HUD streak.

### Compatibility

- Keeps `p1xbratenversion` at 9.0.0.
- Does not change the Sauerbraten network protocol or demo format version.


## cowbraten 1.0.1-rc.1 — 2026-09-07

Release candidate focused on window handling, display reliability and proper HiDPI rendering.

### Fixes

- Added proper HiDPI rendering with separate logical window and physical drawable dimensions.
- Improved monitor preservation across windowed and fullscreen transitions.
- Fixed fullscreen state synchronization when SDL rejects a display-mode change.
- Fixed resizing while the window is maximized.
- Fixed cumulative window-size drift on Wayland fractional scaling.
- Updated viewport, PostFX, motion blur, glare, screenshots, movie capture, screen-relative render targets and model previews to use the physical drawable where required.
- Preserved native Exclusive Fullscreen resolution without additional DPI scaling.

### Compatibility

- Keeps `p1xbratenversion` at 9.0.0.
- Does not change the Sauerbraten network protocol or demo format version.


## cowbraten 1.0.0 — 2026-09-01

Initial cowbraten release, based on p1xbraten 9.0.0.

### Highlights

- Higher-fidelity client demos with more accurate local movement, aim and shot playback.
- Demo recording indicator and a complete playback HUD with accurate duration, timeline seeking, mouse controls and millisecond precision.
- Pause, rewind/forward, absolute seek, variable playback speed and recorded-frame stepping.
- Keyboard help, optional built-in shortcuts and commands suitable for custom binds.
- Followed-player POV preservation while rewinding demos.
- Insta rifle reload feedback through a red-to-ready crosshair transition and optional rotation.
- Local kill-streak indicators (`x2`, `x3`, etc.) with 5/10/15/20/25-kill milestone messages.
- Dynamic sound-pack discovery, live switching and fallback to original Sauerbraten samples.
- Documentation for the custom HUD, crosshair, sound and demo playback features.

### Compatibility

- Keeps `p1xbratenversion` at 9.0.0 for existing p1xbraten client detection.
- Adds the separate read-only `cowbratenversion` variable for fork releases.
- Does not change the Sauerbraten network protocol or demo format version.

### Validation

- Clean Linux client and standalone server builds.
- Manual validation of demo playback, seeking, frame stepping, speed controls, interactive HUD and POV preservation.
