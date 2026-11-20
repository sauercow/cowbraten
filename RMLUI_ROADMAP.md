# RmlUi integration status and release plan

## Release-candidate baseline

Cowbraten **1.1.0-rc.2** is the current RmlUi release candidate. It contains the complete Cowbraten 1.0.1 maintenance history, the native interface described below, p1xbraten 9.0.0 detection compatibility, and the existing Sauerbraten network and standard demo format versions.

Cowbraten **1.0.1-rc.3** remains the classic-interface release candidate. The intended stable **1.0.1** release is an exact promotion of rc.3 after final verification; the rc.3 tag will remain historical and will not be moved.

The **1.1.0** stable designation is reserved until every interface path that still draws through the legacy GUI has been found and either migrated to RmlUi or explicitly documented as an intentional compatibility/engine boundary.

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

## Validation completed for rc.2

- Linux client and standalone server compilation.
- Focused regression coverage for the app shell, settings, profile, game setup, servers, demos, chat, console, editor, utilities, HUD, and scoreboard.
- SDL dummy-audio/offscreen-video startup and rendering checks, including both Legacy and Lato font loading.
- Responsive document checks at compact and desktop viewports and performance regression tooling for high-update interface paths.

Windows and macOS packaging paths are retained, but interactive platform-specific display, input, and audio-device testing should continue as those environments are available.

## Stable-release gates

1. Inventory every `showgui`, legacy GUI render, HUD draw, and compatibility handoff reachable during normal play, spectating, demos, administration, and editing.
2. Migrate every remaining ordinary player-facing legacy surface to RmlUi.
3. Record intentional exceptions in this document with their reason and ownership; they must not remain accidental fallbacks.
4. Exercise navigation, focus, text input, scrolling, bindings, display transitions, persistence, audio devices, servers, demos, multiplayer, bot matches, and editing interactively.
5. Run the available Linux, Windows, and macOS build/package checks and record platform-specific limitations.
6. Re-run the full regression suite and runtime smoke checks from a clean release worktree.

## Currently intentional legacy or engine-owned boundaries

- The in-engine 3D texture palette is a spatial editing surface owned by the engine, not a CubeScript `newgui` document.
- Unknown third-party `showgui` names continue through the legacy GUI for mod compatibility.
- Loading that happens before RmlUi is ready, or while its document cannot be loaded, uses the original OpenGL presentation. Generated lightmap preview textures also remain engine-rendered because they are raw GL handles rather than package resources.
- HD recordings use optional `.dmohd` companion files; standard `.dmo` recording and playback remain supported.

## Remaining migration inventory

- Custom `newgui` documents created by third-party or user `autoexec.cfg` files still hand presentation to the compatibility GUI. Migration is now based on a general recorder for the legacy GUI DSL: it evaluates configured controls into typed snapshots and opaque activation indices, so a compact RmlUi renderer can preserve user-defined menus without exposing CubeScript to RML. Until that renderer covers every used control primitive, the original GUI remains the compatibility fallback.
- The locally observed `autoignore`, `aliases`, and `ss` Autoexec menus now route to native Chat, Profile, and Graphics screens. Display actions remain user-owned `ss_*` aliases discovered through a typed bridge, so personal resolutions are not copied into product code and RML cannot submit script text.
- The ordinary engine main-menu backdrop is still drawn by the engine behind the native application shell.
- Remaining direct HUD drawing must be audited by gameplay mode. The compact secondary-ammo cluster and configurable full ammo bar are now part of the native status snapshot, with the original OpenGL drawing retained only when the native overlay cannot initialize. Mode-owned flag, token, capture, and spectator indicators still need comparison against the native HUD before their legacy branches can be disabled.
- Legacy menu declarations and configuration files must remain available until a runtime switch can skip their UI declarations without skipping gameplay configuration, binds, aliases, map scripts, or mod compatibility.

The target removal step is therefore to stop loading and rendering the legacy *UI* by default while retaining its files and an explicit compatibility switch. It is not to remove CubeScript or the engine configuration layer.

## Post-1.1 maintenance

1. Split the largest Settings implementation files by domain while preserving the typed boundary and current behavior.
2. Add broader automated runtime navigation and multiplayer interaction coverage beyond document/model regression tests.
3. Formalize accessibility, localization, keyboard/controller focus, and alternate-theme contracts before expanding the theme catalog.
4. Review vendored RmlUi updates independently from product interface changes, recording the upstream version and focused renderer validation each time.

## Tag and branch policy

- Keep `cow-v1.0.1-rc.3` and all other RC tags immutable.
- After verification, tag the exact rc.3 code promotion as `cow-v1.0.1`; do not add feature work to that line.
- Tag this work as `cow-v1.1.0-rc.2` only after its release commit is prepared and verified.
- Create `cow-v1.1.0` only after the stable-release gates above are complete.
