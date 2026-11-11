# Changelog

## cowbraten 1.1.0-rc.1 — 2026-11-11

This release candidate combines the 1.0.1 maintenance line with the first production RmlUi interface.

### RmlUi

- Add the native RmlUi runtime, renderer, input integration, Sauer-backed file loading, shared theme, and typed action routing.
- Replace the main menu, server browser, and Settings entry points while retaining the legacy GUI for screens that have not been migrated.
- Add server filtering, sorting, selection, refresh, and connection through snapshots of the existing Sauer server list.
- Add typed Settings pages for gameplay, HUD, graphics, audio, mouse and keyboard, crosshair, chat, and console options.
- Add key binding capture with multiple bindings, exact removal, readable key names, conflict detection, and explicit replacement.
- Add RmlUi controls for Cowbraten and p1x features including weapon trails, feedback, radar, player models, clocks, score overlay, ammobar, frag messages, scoreboard columns, console filters, soundpacks, and audio devices.
- Preserve configuration persistence, legacy console handoff, GL reset recovery, drawable-size updates, and exact-name `showgui` compatibility for migrated screens.

### Integrated maintenance work

- Carry forward the 1.0.1 release-candidate fixes for HiDPI rendering, display transitions, kill-streak lifecycle handling, and HD demos.
- Retain p1xbraten 9.0.0 detection and the existing Sauer network and demo format versions.

### Validation

- Build the Linux client and standalone server from the combined branch.
- Start the client with SDL's offscreen video driver, load the Settings document at 800x600, render a screenshot, and exit cleanly.

### Known scope

- Demos, profile, console, about, and exit actions on the new main menu are still placeholders.
- Game setup, map selection, player model selection, master controls, campaigns, bot matches, and editing tools continue to use the legacy GUI.
- Server favorites and history, navigation history, modal stacking, and a complete interactive platform matrix remain future work.
- See [RMLUI_ROADMAP.md](./RMLUI_ROADMAP.md) for priorities and release gates.

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
