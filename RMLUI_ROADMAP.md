# RmlUi integration status and release plan

## Release-candidate baseline

Cowbraten **1.1.0-rc.3** is the current RmlUi release candidate. It contains the complete Cowbraten 1.0.1 maintenance history, the native interface described below, p1xbraten 9.0.0 detection compatibility, and the existing Sauerbraten network and standard demo format versions.

Cowbraten **1.0.1** is the stable classic-interface release. Its rc.3 tag remains historical and immutable.

The ordinary legacy-interface inventory is complete for RC3. The **1.1.0** stable designation is reserved for final cross-platform verification of this interface and packaging baseline.

## Completed in rc.3

- Migrate loading, gameplay status, secondary ammo, configurable ammo bar, demo recording, carried mode items, match score, frag feedback, and apply confirmation to RmlUi.
- Capture configurable `newgui` and Autoexec menus into typed snapshots and reconstruct their controls, editors, progress, images, previews, tabs, lifecycle hooks, and opaque engine-owned actions as compact native overlays.
- Layer compact overlays above active native screens and preserve function-key menu binds.
- Disable legacy menu declarations by default through a persistent startup switch while retaining the original files and explicit fallback.
- Add online-only automatic demo recording and finish native HUD/scoreboard behavior corrections.
- Cache unchanged HUD state, skip inactive feedback, and throttle passive reconciliation so the native overlay no longer imposes a per-frame layout cost.

## Completed in rc.2

### Platform and UI foundation

- Vendored RmlUi 6.3 with a Cowbraten OpenGL renderer and Sauer package-file interface.
- SDL input translation, text editing, cursor ownership, drawable-size synchronization, HiDPI rendering, and GL reset recovery.
- Cached documents, a shared application shell, semantic controls and icons, default and Modern Metallic themes, and responsive layouts.
- Independent interface and root text scaling, plus selectable Legacy and Lato typography. Legacy uses the existing Sauerbraten `packages/fonts/default.ttf` face.
- Whitelisted action routing and typed data models. RML cannot evaluate CubeScript or submit arbitrary ident names.
- Exact-name compatibility routing for migrated screens and safe legacy fallback for unknown third-party `showgui` documents.

### Player-facing screens

- Profile / Identity, including saved names, model preview, and separate local/reported model controls.
- Game setup for installed maps and modes, server voting, local bot matches, and campaign missions.
- Server browser with filtering, sorting, favorites, stable selection, refresh, compatibility status, and connection.
- Demo library with metadata, search, classification, favorites, recoverable trash, playback actions, and local visualization controls.
- Settings for Game, HUD, Graphics, Audio, Mouse and Keyboard, Crosshair, Chat, Console, themes, scaling, and typography.
- Administration and About, including master mode, player moderation, and compatible-client version reporting.
- Native Notepad, Scratchpad, and Paste Buffer with guarded relative-file access, clipboard actions, and explicit-buffer execution.
- Native map editing control center for geometry, brushes, entities, lights, triggers, mapmodels, materials, post-processing, map files, and selected-entity attributes.
- Compact in-game menu, gameplay chat, full console, responsive HUD/status presentation, and held-key scoreboard.
- Synchronous RmlUi loading and map presentation, with the original OpenGL path retained for early initialization, UI-load failure, and raw generated lightmap textures.

### Engine integration and persistence

- Sauer configuration and bind semantics remain authoritative; custom effective values are read without being silently rewritten when a document opens.
- Interface scale, text scale, theme, typeface, server/demo favorites, profile state, and feature settings persist through established configuration paths.
- Public/team chat, console editing, completion and history use the existing engine paths behind their native presentation.
- The top-left feed uses the available viewport width rather than an arbitrary 40–44% maximum, while preserving minimap clearance.

## Validation completed through rc.3

- Linux client and standalone server compilation.
- Focused regression coverage for the app shell, settings, profile, game setup, servers, demos, chat, console, editor, utilities, HUD, and scoreboard.
- SDL dummy-audio/offscreen-video startup and rendering checks, including both Legacy and Lato font loading.
- Responsive document checks at compact and desktop viewports and performance regression tooling for high-update interface paths.
- Full configurable-menu routing, startup-switch, loading, HUD cache, and passive-overlay regression coverage.
- Real FFA runtime profiling with legacy menu scripts disabled; UI update overhead measured at approximately 0.013 ms per frame in the Linux validation environment.

Windows and macOS packaging paths are retained, but interactive platform-specific display, input, and audio-device testing should continue as those environments are available.

## Stable-release verification gates

1. Complete: inventory every `showgui`, legacy GUI render, HUD draw, and compatibility handoff reachable during normal play, spectating, demos, administration, and editing.
2. Complete: migrate every ordinary player-facing legacy surface to RmlUi.
3. Complete: record intentional exceptions in this document with their reason and ownership.
4. Exercise navigation, focus, text input, scrolling, bindings, display transitions, persistence, audio devices, servers, demos, multiplayer, bot matches, and editing interactively across supported platforms.
5. Run the available Linux, Windows, and macOS build/package checks and record platform-specific limitations.
6. Re-run the full regression suite and runtime smoke checks from a clean release worktree.

## Currently intentional legacy or engine-owned boundaries

- The in-engine 3D texture palette is a spatial editing surface owned by the engine, not a CubeScript `newgui` document.
- Unknown third-party `showgui` names continue through the legacy GUI for mod compatibility.
- Loading that happens before RmlUi is ready, or while its document cannot be loaded, uses the original OpenGL presentation. Generated lightmap preview textures also remain engine-rendered because they are raw GL handles rather than package resources.
- HD recordings use optional `.dmohd` companion files; standard `.dmo` recording and playback remain supported.

## Completed migration inventory

- Custom `newgui` documents created by third-party or user `autoexec.cfg` files now use the general RmlUi recorder, including editor buffers, progress bars, package textures, 3D previews, tabs, init, and clear hooks. The recorder evaluates configured controls into typed snapshots and opaque activation indices, so user-defined menus retain their actions without exposing CubeScript to RML. Unknown future engine primitives still fail the capture as a whole instead of presenting a partial menu.
- The locally observed `autoignore`, `aliases`, and `ss` Autoexec menus now route to native Chat, Profile, and Graphics screens. Display actions remain user-owned `ss_*` aliases discovered through a typed bridge, so personal resolutions are not copied into product code and RML cannot submit script text.
- The ordinary engine main-menu backdrop is still drawn by the engine behind the native application shell.
- The compact secondary-ammo cluster, configurable full ammo bar, demo-recording indicator, carried flags, Hold timer, and Collect token count are part of the native status snapshot, with the original OpenGL drawing retained only when the native overlay cannot initialize. Mode radar/minimap and its world-relative blips remain an intentional spatial engine surface because they combine a generated minimap texture, camera rotation, and live world locations. Respawn placement remains coupled to that radar surface.
- Legacy menu declarations and configuration files remain available, but the persistent startup switch now skips their UI declarations by default without skipping gameplay configuration, binds, aliases, map scripts, or mod compatibility.

RC3 completes the target removal step: legacy *UI* declarations are not loaded by default, while their files and an explicit compatibility switch remain. CubeScript and the engine configuration layer are intentionally unaffected.

## Post-1.1 maintenance

1. Split the largest Settings implementation files by domain while preserving the typed boundary and current behavior.
2. Add broader automated runtime navigation and multiplayer interaction coverage beyond document/model regression tests.
3. Formalize accessibility, localization, keyboard/controller focus, and alternate-theme contracts before expanding the theme catalog.
4. Review vendored RmlUi updates independently from product interface changes, recording the upstream version and focused renderer validation each time.

## Tag and branch policy

- Keep `cow-v1.0.1-rc.3` and all other RC tags immutable.
- Keep the stable classic-interface tag `cow-v1.0.1` immutable.
- Tag this work as `cow-v1.1.0-rc.3` only after its release commit is prepared and verified.
- Create `cow-v1.1.0` only after the stable-release gates above are complete.
