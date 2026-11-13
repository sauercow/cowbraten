# RmlUi integration status and roadmap

## Release baseline

Cowbraten 1.1.0-rc.1 is the convergence point for the previously separate release and RmlUi work. The `release/1.1.0` branch contains:

- the complete Cowbraten 1.0.1-rc.3 line, including HiDPI and display fixes, kill-streak lifecycle handling, and HD demo support;
- the final sequential RmlUi Settings series through audio device controls;
- p1xbraten 9.0.0 compatibility and the existing Sauer network and demo format versions.

This is the recommended shared development baseline. The `cow-v1.0.1-rc.3` tag remains the stable pre-RmlUi reference.

## Completed scope

### Platform and UI foundation

- Vendored RmlUi with a Cowbraten OpenGL renderer and Sauer package-file interface.
- SDL input translation, text input, cursor ownership, logical-window and drawable-size synchronization, and GL reset recovery.
- Cached screen documents, shared RCSS controls, font assets, and a typed data-model boundary.
- Whitelisted action routing. RML cannot evaluate CubeScript or submit arbitrary ident names.
- Legacy console handoff and exact-name compatibility routing for migrated screens and standalone Settings documents.
- Legacy fallback for every non-migrated `showgui` document.

### Production screens

- Main and in-game menus with working Play, Profile, Servers, Demos, Settings, Administration, About, Return, Disconnect, and Exit routes.
- Native game setup for installed maps and modes, server voting, local bot matches, and campaign missions.
- Native Administration and About screens, including master mode and player moderation controls.
- Server browser backed by the existing master list, resolver, ping, and connection paths.
- Server text filtering, empty-server and compatibility filters, sortable columns, stable selection, refresh, and connection.
- In-game public and team chat presentation backed by Sauer's existing editor, completion, history, and network send paths.
- Full Console presentation with filtered scrollback, native scrolling, and the existing command editor and execution paths.
- Settings categories for Game, HUD, Graphics, Audio, Mouse and Keyboard, Crosshair, Chat, and Console.

### Settings coverage

The typed Settings layer currently covers:

- field of view and gameplay feedback;
- weapon trail colors, capture radar options, and player-model presentation;
- FPS and wall-clock overlays, HUD guns, gameclock, score overlay, ammobar, frag-message layout, and scoreboard columns;
- windowed, exclusive fullscreen, and desktop fullscreen modes; resolution, VSync, frame limit, gamma, MSAA, and depth buffer;
- shadows, water and reflection effects, dynamic lights, particles, decals, T-joint repair, grass, motion blur, glare, textures, shader detail, and anisotropic filtering;
- sound and music volume, soundpack selection, channel count, frequency, buffer length, Mumble, and audio device settings;
- gameplay mouse sensitivity and inverted look;
- curated movement, combat, and interface bindings with capture, multiple bindings, exact removal, conflict inspection, and explicit replacement;
- crosshair selection, size, dynamic effects, colors, teammate and hit markers, reload fade, and reload rotation;
- public and team chat routing, team-colored names, top-left console filters, full-console filters, and console size.

Settings continue to use Sauer's established persistence and reset paths. Unsupported or custom effective values are read without silently rewriting them when a page opens.

## Validation completed for the combined branch

- The 1.0.1-rc.3 and final RmlUi histories merge without content conflicts.
- The Linux client and standalone server build from the combined source.
- An SDL dummy-audio/offscreen-video smoke test starts the client, initializes RmlUi, opens Settings at 800x600, renders a screenshot, and exits cleanly.
- Both `cow-v1.0.1-rc.3` and the final `feat/rmlui-audio-device` commit are ancestors of `release/1.1.0`.

The smoke test proves integration and basic rendering. It does not replace interactive input, display-mode, audio-device, or multiplayer testing.

## Release gates

### Required before promoting 1.1.0 from release candidate to stable

- Exercise mouse, keyboard, text editing, focus, scrolling, key capture, conflict replacement, exact unbinding, Escape, and legacy-console handoff interactively.
- Verify windowed, desktop fullscreen, and exclusive fullscreen transitions on single-monitor and multi-monitor systems, including HiDPI and fractional scaling where available.
- Verify Settings readback and persistence after restart, especially GL-reset options, audio reset and device fallback, soundpack fallback, custom resolutions, and custom configuration values.
- Exercise server refresh, filtering, sorting, selection retention, connection, disconnect, and return to the menu against live and unreachable servers.
- Verify every RmlUi-to-legacy handoff used by normal play, including game mode and map selection, player model selection, master controls, bot match, campaign, and editing.
- Run the client on Linux, Windows, and macOS builds when those environments are available. Record platform-specific limitations instead of assuming parity from the Linux smoke test.
- Confirm that the disabled main-menu entries cannot trap input and that every available route returns cleanly to gameplay.

### Suitable for 1.1.0-rc.1

- Main, Servers, and Settings are coherent production slices with a safe legacy fallback.
- The combined client and server compile and the UI passes the offscreen startup smoke test.
- Remaining screens can be developed on the shared branch without splitting RmlUi from release fixes again.

## Prioritized implementation roadmap

### P0: stabilize the current production slice

1. Complete the interactive release-gate matrix and fix any input, focus, reset, persistence, or handoff regressions it exposes.
2. Add a small repeatable runtime smoke harness that opens each migrated screen, captures renderer or document-load errors, and exits with a useful status.
3. Audit all user-visible RmlUi copy and the architecture document whenever a Settings slice changes so implemented behavior and documentation remain aligned.
4. Define an unsaved-edit confirmation policy for the already enabled native Exit action.

### P1: complete the normal player menu flow

1. Add C++ navigation history and a modal stack, with predictable Escape behavior for focused controls, modals, previous screens, and gameplay return.
2. Continue hardening the migrated Demos library and playback paths.
3. Continue hardening the migrated Profile identity and preview paths.
4. Continue hardening the migrated About/Credits and Exit actions.
5. Validate the full RmlUi Console interactively across command, chat, completion, history, clipboard, and scrollback flows.
6. Add focused multiplayer and bot-match interaction coverage to the migrated game setup screen.

### P1: close high-value Settings and server gaps

1. Design a safe Restore Defaults flow with a clear scope and confirmation. The legacy action clears all settings and must not be copied as an unlabelled generic command.
2. Decide whether the `autoexec.cfg` editor belongs in RmlUi or remains an explicitly documented legacy tool.
3. Review intentional Settings omissions against current product needs, including GUI sensitivity, mouse acceleration and zoom response, advanced scoreboard presentation, HUD colors, console fade and mini-console dimensions, and expert audio options.
4. Add server favorites and history only after defining their storage, identity, expiration, and unavailable-server behavior.
5. Add focused compatibility routes for standalone adjust screens only when their RmlUi equivalents preserve the complete semantics of those screens.

### P2: migrate the remaining legacy surface

- Notepad, scratchpad, paste buffer, and other utility screens according to actual user demand.
- The live gameplay scoreboard renderer.
- Editing menus and entity tools as a separate project. Their dynamic CubeScript behavior and large surface make them a poor first target for generalizing the player-facing API.

Each migration should retain the existing legacy route until the replacement covers its semantic behavior and has passed an interactive handoff test.

### P3: reduce maintenance cost

- Split `rml_settings.cpp`, `rml_settings.h`, `sauer_settings_bridge.cpp`, `settings_api.cpp`, and `settings.rml` by domain. They currently combine many unrelated settings and make review and regression isolation difficult.
- Keep the typed boundary while extracting small testable value converters, snapshots, and apply/readback helpers.
- Add a theme resolver so alternate themes can replace physical documents and assets without changing screen IDs or typed actions.
- Define keyboard/controller navigation, focus indicators, accessibility expectations, localization boundaries, and scalable text behavior before adding theme variants.
- Revisit vendored RmlUi updates separately from product UI changes, with an explicit upstream version and focused renderer validation.

## Definition of done for the migration

The RmlUi migration is complete when:

- every normal player flow is available without an unexpected legacy GUI transition;
- remaining legacy tools are deliberately classified and documented rather than accidentally unreachable;
- navigation, modals, focus, and Escape follow one tested policy;
- all exposed values use typed APIs with accepted-value readback and preserve custom configuration state;
- the interactive platform matrix passes for display, input, audio, persistence, server browsing, and GL reset behavior;
- the large Settings implementation is divided into reviewable domains with repeatable runtime coverage;
- no placeholder on the main menu is presented as an available action.

## Branch policy

Use `release/1.1.0` as the shared convergence branch for the release-candidate cycle. Develop bounded roadmap items on short-lived branches and merge them back after their relevant checks pass. Keep the 1.0.1 tag as the maintenance baseline; avoid maintaining a second long-running RmlUi line after this convergence.
