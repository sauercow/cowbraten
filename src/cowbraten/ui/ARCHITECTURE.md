# Cowbraten UI foundation

This directory owns the native RmlUi integration. The first public boundary is
deliberately small: RML describes structure and names known actions, RCSS owns
presentation, and C++ owns state and behavior.

## UI API v1 boundary

The intended call path is:

    RML event -> RmlActionRouter -> typed Cowbraten UI API -> legacy bridge

Inline RML event attributes contain one exact registered action name, for
example `onclick="test.toggle-selection"`. `RmlActionRouter` resolves that name
to a C++ function registered during UI initialization.

- Unknown names are rejected when the document is loaded.
- Action values are not parsed as commands and cannot contain arguments.
- The router never evaluates CubeScript, Lua, or another scripting language.
- Production handlers should call a typed, RmlUi-free Cowbraten UI API. That API
  may use a narrow legacy-only bridge translation unit when Sauer state or
  behavior is required.

New actions should use a stable group prefix:

- `navigation.*`
- `game.*`
- `servers.*`
- `settings.*`
- `profile.*`
- `demos.*`
- `console.*`

The first production action is `navigation.close`. It uses the same typed
`cowui::close()` path as Escape and `/ui_close`, including Sauer input-state
restoration. Test-only actions retain the `test.*` prefix.

## Documents and navigation

Each top-level screen is a separate RML document sharing the foundation styles.
The small C++ screen table owns explicit screen identifiers and asset paths. It
lazily loads each document once, caches it for the RmlUi context lifetime, and
shows or hides documents on transitions.

All production documents instantiate the native `app-shell` RML template. The
template owns the fixed-width global sidebar, product header, gameplay return,
exit action, and status footer; each screen injects only its module content into
the `app-content` slot and declares an `app-page-*` class for the active item.
Consequently navigation geometry and hover/focus behavior cannot drift between
Main, Servers, Demos, Profile, and Settings. Module navigation remains local:
for example, Settings categories live inside Settings content, while Profile
does not duplicate links to unrelated modules.

`data/cowbraten/ui/default/` is the current default theme, not an engine-owned
layout contract. Concrete default screen and font paths are centralized in
`ui.cpp`; settings models and actions do not reference DOM ids, classes, RML
hierarchy, pixel positions, or style values. The theme resolver extends those
documents while retaining the same `ScreenId`, typed model fields, and
whitelisted action names. RML and RCSS remain responsible for arranging and
presenting that semantic contract.

The theme resolver uses RmlUi's native media-theme mechanism. Every document
combines its structural styles with the registered theme sheets; the persistent
`uitheme` setting activates one supported theme through
`Context::ActivateTheme()`. `default` leaves the foundation tokens unchanged,
while `modern-metallic` overrides semantic tokens and component presentation in
`data/cowbraten/ui/themes/modern-metallic/theme.rcss`. Theme selection never
changes a model, action name, or legacy setting. A new theme therefore adds an
RCSS sheet and any licensed font/assets, not engine-side UI behavior.

Only implemented screens receive identifiers. Game Menu, Profile, Game Setup,
Servers, Demos, Settings, Administration, About, Utilities, Editor, Console,
and Scoreboard are production identifiers, while Test remains a developer-only
identifier. The former branding-only Main document is not a
navigation target: `/ui_main` and legacy `showgui main` open the compact Game
Menu during a match and Profile / Identity while disconnected. Later screens
are added to the table when their documents and actions exist.

Profile / Identity is the first full screen built against the themed component
contract and is the disconnected menu destination. Its RML owns player-name editing,
saved-name selection, the character roster, controls, selection states,
responsive layout, and preview interaction. `RmlProfile` owns presentation
state and calls the typed `profile_api`; only `sauer_profile_bridge` knows
`game::switchname`, the persistent `profile_name_1` through `profile_name_5`
preset slots, the `playermodel`, `modelspoofing`, and `spoofplayermodel`
idents, and the engine model-preview renderer. Model spoofing keeps the local
model authoritative for rendering while `reportedplayermodel()` supplies the
configured model to the connection handshake and subsequent `N_SWITCHMODEL`
messages. Toggling it or changing its reported model while connected sends an
immediate update; Profile persists both settings immediately. The temporary
version-detection model packet also restores the reported model so it cannot
leak or overwrite the local choice on peers.
The 3D model is rendered into the content rectangle calculated by RmlUi after
the normal RML pass, so viewport coordinates are not duplicated in C++.
Legacy `showgui playermodel` routes to this native screen. The in-match Game
Menu exposes resume, team, spectator, expanded-menu, and disconnect actions
through a typed Sauer bridge without evaluating CubeScript.

At most one normal screen is active. Switching screens hides the previous
document without releasing menu input ownership, while closing the active
screen returns directly to gameplay. Opening the full application shell from
the Game Menu records that single origin so Escape and the shell Back action
return to the compact menu; Return to Game still closes the UI directly. This
small origin/return policy covers the current production flows; a general
navigation history and modal stack can be added if deeper nesting is needed.

Closing the UI hides the active document and all modals, releases RmlUi focus,
and returns cursor/grab ownership through the existing input bridge. Cached
documents remain loaded until shutdown. Reopening is an explicit request for a
screen rather than an implicit DOM operation.

The RmlUi context uses drawable pixels so geometry, text, clipping, and
textures remain sharp on HiDPI displays. RCSS geometry uses density-independent
pixels and typography uses `rem` tokens rooted at 16dp. The density ratio
combines SDL's drawable-to-window scale with responsive desktop scaling from a
1920x1080 design viewport, capped at 2x. Thus QHD and 4K displays scale the
whole interface even when operating-system scaling is disabled, without
double-scaling HiDPI windows. SDL pointer coordinates remain mapped into
drawable context space before dispatch.

The legacy Sauer command line temporarily takes text-input ownership without
closing or hiding the active RmlUi screen. Its normal `saycommand`,
`sayteamcommand`, and `inputcommand` key bindings are recognized by the legacy
input layer only when no RmlUi text input is active. While an editable control
has focus, RmlUi owns every keyboard and text event so printable bind keys are
inserted rather than opening the command line. RmlUi releases control focus
when a command line legitimately opens, while the engine keeps ownership of
editing, completion, history, and bind processing. Public and team input use
the gameplay Chat overlay. `toggleconsole` hides the cached screen and shows
the full RmlUi Console overlay, then restores the same screen when it closes
while retaining the menu's existing cursor/grab session.

Legacy `showgui` calls pass through an exact-name compatibility router before
the legacy menu table. Migrated names cover the main/profile flow, servers,
options, demos, administration, utilities, and named editor documents. A match
uses the same cached `ScreenId` navigation path as the direct `/ui_*` commands;
there is no command parsing, partial matching, or DOM-based routing. New names
are added only when their production screen exists.

An existing, non-migrated legacy GUI keeps the original `showgui` path. If it
is requested while RmlUi owns the menu, RmlUi closes through its normal input
restoration path before the legacy GUI opens. An unknown name remains the
legacy no-op and does not disturb the active RmlUi screen. While RmlUi is
visible, the legacy GUI stack is not rendered, which prevents two menu layers
from competing while preserving legacy stack state for a later handoff.

## Loading presentation

Blocking engine loads do not pass through the normal UI frame loop. A cached
`RmlLoading` document is therefore updated and rendered synchronously from
`renderbackground()` and `renderprogress()`. It owns captions, map metadata,
package-backed map previews, and determinate or indeterminate progress. The
document uses the same theme and interface preferences as the other cached
screens.

The integration is deliberately fail-open: if RmlUi has not initialized, the
document or theme cannot load, or the renderer is unavailable, the original
OpenGL loading code runs unchanged. Raw generated lightmap preview textures
also stay on that path because a GL texture handle is not a safe RML resource
name. The normal application-shell backdrop remains engine-owned for now.

## Text utilities model

The legacy `notepad`, `scratchpad`, and `pastebuffer` documents share one
native Utilities screen with three explicit tabs. Their exact `showgui` names
and `/ui_utilities`, `/ui_scratchpad`, and `/ui_pastebuffer` commands select the
corresponding cached tab. The global application shell exposes the same screen
through its Tools destination.

The Rml-facing model owns the three editable buffers, filename, active tab,
and operation status. File and clipboard behavior crosses `utility_api` into a
narrow `sauer_utility_bridge`: the bridge uses the engine's package-aware
`loadfile` and UTF-8 write stream, SDL clipboard functions, and explicit-buffer
`execute`. It rejects absolute paths, parent traversal, drive prefixes,
backslashes, and control characters. RML therefore cannot provide an ident
name or choose an engine command; only the user's visible buffer can be run by
the fixed Execute actions.

RmlUi multiline controls synchronize their values through the typed data model,
so file loads and clipboard mutations update cached documents and user edits
flow back through explicit callbacks. The utility stylesheet inherits the
selected interface face. `uifont` chooses between the bundled Lato family and
the Sauerbraten content package's `packages/fonts/default.ttf` Legacy face,
matching the classic console/FPS typography. The root font variable is updated
across cached documents so the choice applies live and persists with the other
interface preferences.

## Map editor model

The native Map Editor replaces the named CubeScript GUI surface for `editing`,
`maplighting`, `materials`, `triggers`, `trigger2`, `trigger3`, `newlight`,
`newteleport`, `mapmodels`, `monsters`, `postfx`, `brushes`, `quickedit`, and
`seltexinfo`. The entity GUIs generated from `enttypelist` also route to the
native Selection section, so context editing no longer falls through to a
dynamically generated legacy document. Exact routes select Edit, Entities,
Lighting, Materials, or Selection rather than opening a generic landing page.

The call path retains the normal boundary:

    screens/editor.rml -> RmlEditor -> editor_api -> sauer_editor_bridge

Geometry, brush, entity, material, and post-processing actions use integer IDs
validated against fixed command tables in the bridge. RML cannot supply a
command string or ident name. Mapmodel rows are enumerated through a visitor
over the engine's configured model list, and a click can create only an ID
present in that list. Custom lights and triggers clamp every numeric attribute
and validate the legacy trigger model, behavior, and tag domains. Map save and
load names allow only safe relative package names. Selected entity attributes
are read from Sauer's current entity group and applied through the established
`entset` path, retaining undo and cooperative-edit behavior.

Opening Lighting reads existing map variables into the model. Initial range
binding is guarded against same-value writes, so opening the document does not
dirty a map or replay map-variable callbacks. In-world mouse and keyboard edit
bindings remain authoritative. The engine-owned texture palette invoked by
`showtexgui` is not a CubeScript `newgui` document and remains the specialized
3D texture browser; all named legacy GUI documents route through RmlUi.

The former `cowbratenusage` document is folded into Administration. Each typed
client row includes the compatibility version reported by the same engine
source as `getclientp1xbratenversion`, alongside team and privilege state.

Application behavior must not depend on element IDs. IDs remain acceptable for
document-local relationships such as a label's `for` attribute. State reaches
RML through event-driven data models and stable actions; documents are not
rebuilt or copied every frame. When the UI is closed, the manager must preserve
the existing no-update/no-render fast path.

## Server Browser model

The production Server Browser keeps legacy networking and RmlUi presentation
on opposite sides of two narrow layers:

    Sauer serverinfo list
        -> sauer_server_bridge (legacy-only snapshot)
        -> RmlServerBrowser (owned view model)
        -> server_browser data model
        -> screens/servers.rml

The legacy bridge remains responsible for calling the existing master-list,
resolver, ping, and connection functions. It exports plain value snapshots and
accepts a stable `host:port` identifier for connection; an RmlUi document never
sees a `serverinfo` pointer. The Rml-facing model owns all strings used by the
document, retains selection by stable identifier, and applies presentation-only
filtering and sorting to its copy without reordering Sauer's global list. Text,
empty-server, and protocol-compatibility filters compose before the current
sort. Their state remains owned by the cached model for the process lifetime.

While Servers is active, Sauer's existing `refreshservers()` path continues to
drain ping responses. Snapshot comparison is throttled to 10 Hz, and the RmlUi
row variable is dirtied only when backend values, a filter, or the sort order
actually changes. Selection dirties only its stable identity and summary
variables. Navigating away stops backend work; the cached document and owned
snapshot remain available for the next visit.

The registered actions are explicit and typed:

- `navigation.servers` and `navigation.main` switch cached documents.
- `select_server(server.id)` is a model-bound generated-row callback that
  selects the row's stable value identity without reading DOM element IDs.
- `servers.filter` reads the text control's value.
- `servers.toggle.hide-empty` and `servers.toggle.compatible-only` change known
  model-owned filters.
- `servers.sort.*` changes a known sort key and direction.
- `servers.refresh` invokes the existing master-list refresh path.
- `servers.connect` resolves the selected identifier in the live Sauer list and
  uses the normal `cowui::close()` input-ownership path after connecting.
- `toggle_favorite(server.id)` changes only the generated row's stable server
  identity, persists it in `cowbraten-ui.cfg`, and rebuilds the presentation so
  favorites stay ahead of the selected column ordering.

There is no generic host command, ident lookup, or CubeScript evaluation in the
model. Favorites use stable `host:port` values; connection history remains
absent until its persistence and product semantics are designed.

## Settings model

Settings use a typed path for each supported value rather than exposing Sauer's
ident table to RML:

    screens/settings.rml
        -> RmlSettings
        -> settings_api
        -> sauer_settings_bridge (legacy-only)
        -> checked Sauer setter

The first supported value is FOV. Graphics adds the three presentation modes,
resolution, VSync, the rendered-frame limit, and multisample antialiasing
through the same boundary. The legacy bridge alone knows the fixed `fov`,
`fullscreen`, `fullscreendesktop`, `vsync`, `maxfps`, and `fsaa` ident names,
verifies their integer types, and writes them through `setvarchecked()`.
Resolution changes call the existing `screenres()` path with checked integer
dimensions. Neither the document nor the RmlUi-facing model can supply an
ident name or a CubeScript command. Additional settings should gain similarly
explicit typed API functions rather than a generic ident setter.

Resolution choices are value snapshots enumerated from SDL for the display
containing the game window. The Rml model owns, sorts, and de-duplicates those
values, then accepts clicks only for an advertised width/height pair. A current
custom window size may be displayed but cannot be selected as a fullscreen
mode. Desktop fullscreen keeps its existing Sauer semantics and disables the
resolution choices because that mode always uses the display's native size.
Windowed, exclusive fullscreen, and desktop fullscreen are separate typed
values. Their presentation state and current dimensions are read from the
actual SDL window flags and size, not inferred from markup or requested
`scr_w/scr_h` values.

VSync writes the fixed Sauer setting so the existing adaptive-tear policy and
callback remain authoritative, then reads `SDL_GL_GetSwapInterval()` through
the legacy bridge. An SDL failure synchronizes Sauer's `vsync` value back to
the effective interval instead of leaving requested and runtime state split.
Drivers may still report only the interval mode, not whether every displayed
frame is physically synchronized. The FPS limit uses the fixed `maxfps`
setting and its declared range; zero retains Cowbraten's existing unlimited
rendering meaning.

MSAA keeps requested and effective state distinct. RML may request only the
legacy menu values Off, 2x, 4x, 8x, or 16x. `fsaa` stores that request, while
the effective value is read from `SDL_GL_MULTISAMPLEBUFFERS` and
`SDL_GL_MULTISAMPLESAMPLES` after the context exists. Applying a changed value
uses the existing `resetgl()` lifecycle and its unsupported-mode fallback.
The reset is deferred until the originating RmlUi input dispatch has returned,
so a callback never destroys renderer resources while RmlUi is still handling
the event. The cached document and data model survive the reset, and the model
then reports the effective value even when it differs from the request.

The `settings` data model owns the displayed FOV value and range. Opening or
returning to the cached Settings document refreshes those values from Sauer.
Changing the range control invokes the whitelisted `settings.fov` action; the
handler writes through the typed API, immediately reads the accepted value
back, and dirties only model variables that changed. When the legacy console
temporarily suspends RmlUi, the model refreshes before Settings is shown again,
so a normal `/fov` change is reflected without per-frame ident polling.

The Game tab also exposes Capture radar numbering and teammate radar blips
through Sauer's persistent `basenumbers` and `radarteammates` booleans. Both
are 0/1 integer idents with no change callback, so they apply immediately on
the existing capture HUD, capture messages, and Capture/CTF/Collect teammate
radar paths. There is no `resetgl()`. Opening Settings, returning from the
legacy console, and activating the Game category only read these values. A
checkbox write goes through the typed API, then reads the accepted snapshot
back. The document never supplies ident names.

The Game tab also exposes player-model presentation through Sauer's persistent
`ragdoll`, `ragdollmillis`, `deadpush`, `hidedead`, `fullbrightmodels`,
`forceplayermodels`, and `teamskins` integer idents. None of these have a
change callback, so they apply immediately on the existing death, hit-push,
and player-model render paths. There is no `resetgl()`. Opening Settings,
returning from the legacy console, and activating the Game category only read
these values. Canonical native Settings writes are ragdoll 0/1, ragdollmillis
0/10000, hidedead 0/1, and fullbrightmodels 0/60 plus radios 60/100/150/200.
If the engine holds hidedead 2, a non-zero unmatched ragdollmillis, or a
fullbright intensity other than those radios, the UI presents the checkbox on
(and no radio selected) without rewriting until the user toggles or selects.
The deadpush range has no hardcoded RML value default; its change handler
ignores a requested value that already matches the model or a range element
that is not yet parented, matching the gamma rewrite guard. A checkbox or
intensity write goes through the typed API, then reads the accepted snapshot
back. The document never supplies ident names.

The Game tab also inlines the native Game-tab "adjust weapon trail colors"
document. The typed snapshot owns the six packed persistent HVARPs
`trailcolorshotgun`, `trailcolorchaingun`, `trailcolorrocketlauncher`,
`trailcolorrifle`, `trailcolorgrenadelauncher`, and `trailcolorpistol`, plus
the native per-channel slider range 0..255. None have a change callback, so
they apply immediately on the existing weapon trail particle paths. There is
no `resetgl()`. Opening Settings, returning from the legacy console, and
activating the Game category only read these values. RGB sliders pack as
`(red << 16) | (green << 8) | blue`, matching Sauer's IDF_HEX three-argument
assignment, then write the packed integer through `setvarchecked()`. Range
change handlers ignore a requested value that already matches the model or a
range element that is not yet parented, matching the gamma rewrite guard.
There is no hardcoded RML value default. The document never supplies ident
names or the `settrailcolor` command. `showgui trailcolors` now routes to the
Game category and scrolls directly to this native section.

The same activation and console-resume refresh updates presentation, current
resolution, the supported-mode snapshot, VSync, FPS limit, and effective MSAA.
Category selection remains owned by the cached model and only the active page
subtree is presented. Existing window resize and GL-reset callbacks refresh
the video model when Settings is active, keeping it event-driven without
per-frame polling.

Audio volume uses the same explicit boundary. `soundvol` and `musicvol` remain
the authoritative persistent Sauer mix values, with their declared `0..255`
ranges. Sound Volume controls gameplay effects and retains Sauer's existing
special behavior where zero stops effects and gates music; it is not presented
as an independent mathematical master gain. Music Volume updates the active
SDL_mixer music stream immediately whenever Sound Volume is non-zero. The
model also reports whether Sauer's audio backend is actually active through a
plain boolean snapshot from the legacy bridge. There is no separate legacy
master-volume ident, so the UI does not fabricate one.

Settings activation, local volume writes, and return from the legacy console
refresh the Audio model.

Soundpack choices come from the engine's existing `visitsoundpacks()` discovery
path, which also backs the legacy `listsoundpacks` command. Directory names are
stable semantic IDs; RML receives only those validated IDs, their existing
display names, and selection state. A selection is checked against a freshly
discovered backend choice before the fixed `soundpack` ident is changed. Its
existing callback synchronously stops gameplay sound channels, resets game
sound registrations and cached samples, loads the selected `sounds.cfg`, and
falls back to Original on validation or configuration failure. Music is not
restarted. The Rml model immediately reads back the effective ID and rebuilds
only its small choice snapshot on Settings/Audio activation, local selection,
or console return. No filesystem paths, CubeScript, DOM structure, or theme
presentation cross this typed boundary.

The remaining native Settings SOUND tab device controls share that typed
boundary. `soundchans` (1..128), `soundfreq` (declared 0..48000), and
`soundbufferlen` (128..4096) write through `setvarchecked` immediately. Their
VARF callbacks queue `initwarning("sound configuration", INIT_RESET,
CHANGE_SOUND)`. Opening Settings never rewrites those idents. Integer sliders
have no hardcoded RML value default and ignore a requested value that already
matches the model or a range that is not yet parented, matching the gamma
rewrite guard.

Native Settings presents frequency as a list slider of 11025, 22050, and
44100 Hz only. CowUI writes those three values and does not coerce another
effective ident (including 0 or 48000) when the page opens; radios select
only on exact match. After a write, `readaudio` reports the ident as
requested and the open mixer as effective: channel count from `maxchannels`,
frequency from `Mix_QuerySpec`. `Mix_QuerySpec` does not report chunk size,
so buffer length has no mixer readback. `usesound`, `audiodriver`, and
`fixwav` stay unexposed; they are not on the native Settings sound tab.

When Settings is the active screen, `ui.cpp` `finishinput` already calls
`RmlSettings::applypending()`. If `haschanges(CHANGE_SOUND)` is true,
applypending invokes the existing engine `resetsound()` path, which starts by
`clearchanges(CHANGE_SOUND)` and then reopens the mixer. That matches leaving
the native GUI and confirming the apply dialog. If `applydialog` is 0, native
`addchange` does not queue CHANGE_SOUND, so applypending will not reset the
mixer; the ident still updates immediately and is written to `init.cfg` on
exit.

`mumble` is a VARFP checkbox ("Mumble positional audio"). Its callback calls
`initmumble` or `closemumble` immediately. The default is 1 when the platform
builds the Mumble link (Windows, or POSIX shared memory) and 0 otherwise.
Mumble does not queue CHANGE_SOUND and does not call `resetsound`.

The first Mouse / Keyboard slice exposes only gameplay sensitivity and
vertical-look inversion. The bridge targets the fixed persistent
`sensitivity` float and `invmouse` integer idents; both affect camera input
immediately and require no SDL or renderer reset. Sensitivity retains the
legacy 0.01 decrement/increment behavior and adds a local text edit buffer for
exact values. Enter or loss of focus parses the buffer, invokes only the typed
sensitivity writer, reads the effective float back, and restores canonical
text. Invalid or out-of-range input never reaches the ident. External refresh
does not rewrite a temporary value while the field is focused. The control
retains Sauer's full declared range and does not alter `guisens`, which
belongs to the legacy 3D GUI and does not control RmlUi's absolute pointer.
Mouse acceleration, zoom-specific response, and sensitivity scaling remain
outside this basic slice.

The key binding panel exposes a curated set of movement, combat, and interface
actions through `keybindings_api`. The Rml model owns capture and cancellation
state, displays every current binding with a readable key name, removes one
exact binding at a time, and requires an explicit decision before replacing a
conflicting binding. The Sauer bridge owns command identity and persistence;
markup cannot supply or evaluate an arbitrary bind command.

The first Crosshair slice follows the same fixed-name boundary. Its value
snapshot contains the persistent `crosshairsize`, `crosshairfx`,
`crosshaircolors`, `teamcrosshair`, `hitcrosshair`, `crosshairreloadfade`, and
`crosshairreloadrotate` settings. Size retains Sauer's declared integer
`0..50` range and uses a local edit buffer, so incomplete text is not written
while typing. The hit marker remains a boolean UI operation and reuses the
legacy menu's standard 425 ms enabled value; this slice does not expose the
underlying duration as a new product control.

Crosshair selection is not an ident. The legacy bridge owns the same fixed
normal/hit texture pairs advertised by `menus.cfg`, gives RmlUi stable numeric
choice IDs and labels, and invokes the existing engine `loadcrosshair()` path
for slots 0 and 2. Only pairs resolvable through Sauer's package filesystem are
advertised, because a legacy menu catalog may outlive an optional asset pack.
An engine read-only helper reports the effective texture name so a custom
config is represented as Custom instead of being misidentified. Texture paths
never originate in RML, and Sauer's existing `writecrosshairs()` config
persistence remains authoritative. The bridge exposes each effective normal
texture path as semantic choice data. Theme RML decides whether and where to
render it. The generic Rml texture loader reads the same file through the
Sauer-backed `FileInterface`, decodes it with SDL_image, premultiplies its
pixels, and places it in the renderer's existing CPU-backed texture lifecycle.
The semantic `?cowbraten-crosshair:` resource prefix additionally identifies
legacy RGB mask images whose black background is transparent under Sauer's
additive crosshair blend; the loader converts that mask to premultiplied alpha
for RmlUi. This keeps previews valid across `resetgl()` without exposing engine
texture or OpenGL handles or encoding any Settings DOM structure in C++.

The HUD category exposes overlay stats and combat feedback. Overlay stats are
vanilla Sauer's persistent `showfps` and `wallclock` toggles, plus the
`wallclock24` and `wallclocksecs` format flags that the renderer consults only
while the clock is on. Combat feedback remains vanilla Sauer's persistent
`damagescreen` and `damagecompass` toggles, p1x's three-state
`hudfragmessages` mode, and Cowbraten's persistent `streakmilestonescale`
float. All apply immediately through their existing render paths without
resource or GL reset. Turning the wall clock off does not rewrite its 24-hour
or seconds flags; the default theme hides those subordinates until the clock
is on again, matching the legacy HUD tab. `showfpsrange` is not a production
HUD-tab control and is not exposed. The streak scale uses its declared
`1.0..2.0` range and the same local edit/commit/readback policy as exact mouse
sensitivity. Settings activation, category activation, local writes, and
console return refresh these snapshots; there is no frame-loop polling.

HUD guns add four independent persistent Sauer booleans on the same HUD page:
`hudgun`, `hudgunsway`, `muzzleflash`, and `muzzlelight`. Each has its own
typed writer. Toggling `hudgun` does not write the other three idents. Sway
only offsets the first-person weapon while `drawhudgun()` draws it. Muzzle
flash requires a valid muzzle tag; local first-person flashes therefore
disappear when HUD guns are off because that path invalidates the tag, while
other players and third-person view can still supply a world-model tag.
Muzzle light does not require that tag and still uses `hudgunorigin()` when
HUD guns are off. The default theme groups sway, flash, and light as
subordinates of HUD Guns without disabling or rewriting those independent
values. `hudgunsdir` remains unexposed.

Gameclock is a separate typed snapshot of Sauer's match overlay, not the
HUD wall clock. The master `gameclock` boolean is always shown. Size,
opacity, alignment, and x/y layout controls are visible only while the
clock is on, matching the native HUD tab's "adjust gameclock" panel.
`gameclockscale`, `gameclockx`, and `gameclocky` use the native `+fround`
0.01 stepper plus exact field commit; `gameclockalpha` is an integer
slider bound to the ident range with no hardcoded RML value default;
`gameclockalign` radios write only on user select and light only on the
exact ident values `-1`, `0`, and `1`. All six writes go through
`setvarchecked` / `setfvarchecked` and read back into the snapshot.
Opening Settings does not rewrite these idents. `gameclockcolour` and
`gameclocklowcolour` are not on the native Settings GUI and stay
unexposed. `showgui gameclock` now routes to the HUD category and scrolls
directly to this native section.

HUD score overlay inlines the native HUD-tab `hudscore` master toggle and the
`hudscore` adjust document. The migrated persistent idents are `hudscore`,
`hudscorescale`, `hudscorealpha`, `hudscoresep`, `hudscorealign`, `hudscorex`,
and `hudscorey`. None have a change callback, so they apply immediately on
`drawhudscore()` when the overlay is on. `hudscoreplayercolour` and
`hudscoreenemycolour` are not native Settings GUI controls and stay unexposed.
Opening Settings, returning from the legacy console, and activating the HUD
category only read these values. Integer opacity and spacing sliders bind
min/max from the ident and have no hardcoded RML value default; their change
handlers ignore a requested value that already matches the model or a range
element that is not yet parented, matching the gamma/deadpush rewrite guard.
Size, x, and y use local edit buffers and `+fround`-style 0.01 steppers, then
clamp through `setfvarchecked`. Anchor radios write only on user select and
light only on an exact `-1`/`0`/`1` match. A typed write then reads the
accepted snapshot back. The document never supplies ident names.

`hudfragmessages` is one p1x enum, not a generic per-game-mode policy: `0`
disables frag and Cowbraten streak messages, `1` shows them in all modes, and
`2` shows them outside instagib only. No equivalent exceptions exist for other
modes. The default theme therefore presents an Enabled toggle plus a subordinate
Show in Instagib toggle, while the typed settings API retains the exact engine
enum as its source of truth.

HUD frag-message layout inlines the native `hudfragmessages` adjust document
onto the Settings HUD page. The typed snapshot owns the layout idents that
native GUI already exposes: max (`maxhudfragmessages`, VARFP `1..10`;
`setvarchecked()` runs the buffer grow/shrink callback), duration
(`hudfragmessageduration`, `0..10` seconds, native `0.1` steps), the three
native `hudfragmessagefilter` bits only (`CON_FRAG_SELF` `0x800`,
`CON_FRAG_OTHER` `0x1000`, `CON_TEAMKILL` `0x2000`; every other bit is
preserved), size (`hudfragmessagescale`, `0.1..1.0`, native `0.1` steps),
fade (`hudfragmessagefade`), normalized position (`hudfragmessagex`,
`hudfragmessagey`, native `0.01` steps), and stack direction
(`hudfragmessagestackdir` as exact `-1` / `0` / `1`). Each control has a
typed writer through `setvarchecked()` / `setfvarchecked()`. All apply
immediately through `drawfragmessages()` with no GL reset. Filter writers skip
when the requested bit already matches, so opening Settings cannot rewrite the
default `0x2800`. `hudfragmessagefilter` is an `HVARP` registered as `ID_VAR`
with `IDF_HEX`; there is no separate hex ident type. Turning frag messages off
does not rewrite layout idents; the default theme hides those subordinates
until messages are on, matching `if $hudfragmessages`. The integer max slider
binds min/max from the ident, omits a hardcoded RML `value`, and ignores
programmatic range change during document construct or when the enabled
`data-if` reveals the extras. Float fields use the same local
edit/commit/readback policy as streak scale. Stack radios write only on user
select and select only on an exact `-1/0/1` match.
`hudfragmessageforcecolors` is not in the native Settings GUI and is not
exposed. `showgui "hudfragmessages"` routes to this native Settings section.

HUD ammo bar inlines the legacy `ammobar` adjust document onto the Settings HUD
page. The typed snapshot owns Sauer's persistent `ammobar` master switch plus
the layout idents that native GUI already exposes: orientation
(`ammobarhorizontal`, `ammobarflip`), size (`ammobarscale`), icon spacing
(`ammobarsep`), label size (`ammobarcountscale`), label offset
(`ammobarcountsep`), anchor (`ammobaralign` as exact `-1` / `0` / `1`),
normalized position (`ammobarx`, `ammobary`), and hide-empty
(`ammobarhideempty`, default on). Each control has a typed writer through
`setvarchecked()` / `setfvarchecked()`. All apply immediately through
`gamehud()` / `drawammobar()` with no ident callback and no GL reset. Turning
the master off does not rewrite layout idents; the default theme hides those
subordinates until the bar is on, matching `if $ammobar`. Integer sliders bind
min/max from the idents, omit a hardcoded RML `value`, and ignore programmatic
range change during document construct. Float fields use the same local
edit/commit/readback policy as streak scale, with native `0.01` steps.
Anchor radios write only on user select and select only on an exact `-1/0/1`
match. `showgui "ammobar"` routes to this native Settings section.

HUD scoreboard columns inline the native `newgui scoreboard` column checkboxes
onto Settings HUD. They are always presented; native `guibutton "adjust
scoreboard"` is not gated on `hudscore`, and this slice does not add that gate.
Show at death remains the Game-tab `deathscore` control and is not duplicated.
The typed snapshot owns Sauer's persistent column idents: `showflags`, inverted
`hidefrags` (UI "frags" on writes 0, off writes 1), `showdeaths`,
`showsuicides`, `showkpd`, `showaccuracy`, `showdamage` 0/1/2, `showdamagereceived`,
`showping` 0/1/2, `showpj`, and `showclientnum`. Each control has a typed writer
through `setvarchecked()`. Opening Settings, returning from the legacy console,
and activating HUD only read these values. Checkbox writes go through the typed
API, then read the accepted snapshot back. Native `showping` and `showdamage`
checkboxes write 0/1, but their ident maxima are 2. A non-zero unmatched value
presents the checkbox on without rewriting until the user toggles; enabling from
0 writes 1, disabling writes 0. Damage radios are exact-match 1 (dealt) and 2
(net) only, skip-if-equal, and leave an unmatched non-zero value with no radio
selected. `scoreboard2d`, `showservinfo`, `showspectators`, `highlightscore`,
`tiesort`, and related non-column scoreboard options stay unexposed.
`showgui "scoreboard"` routes to this native Settings section. The direct
`gameclock`, `hudscore`, `ammobar`, `hudfragmessages`, `crosshair`, `soundpacks`,
and `trailcolors` names use the same category-and-anchor compatibility path,
so existing binds reach the exact typed control group without a legacy GUI.

The held gameplay scoreboard is also native RmlUi. `showscores()` first asks
the UI overlay to present; if RmlUi is unavailable it retains the original
`g3d_gui` scoreboard as a startup-safe fallback. The typed scoreboard bridge
reuses `groupplayers()`, its sorted groups and spectator list, the effective
column idents, and Cowbraten's weapon-stat helpers. Holding the key updates the
overlay live without taking gameplay input. Opening a menu temporarily hides
the overlay while preserving held-key state, and releasing the key closes it.

The first Chat slice preserves Sauer's actual console-backed message model.
Public and team chat enter the shared console buffer as the distinct
`CON_CHAT` and `CON_TEAMCHAT` message types. Explicit typed writers toggle
only those two bits in the persistent `confilter` top-left feed and
`miniconfilter` bottom-left mini-console masks, preserving every unrelated or
custom filter bit. `teamcolorchat` remains a separate persistent boolean that
controls player-name coloring when chat lines are formatted. All five values
apply immediately and are read back from their fixed legacy idents after each
write, on Chat activation, on Settings activation, and after legacy-console
handoff. Message delivery, completion, history, and scrollback remain owned by
Sauer. The gameplay presentation is now an RmlUi overlay. The complete
filtered top-left feed and bottom-left mini-console are mirrored into separate
typed message lists with their native capacities and fade times; public and
team command-line state is read through a narrow bridge and rendered with a
channel label and caret. The legacy editor still processes text and keys and
performs the actual send, so protocol behavior is unchanged. Once the RmlUi
document is available, both passive legacy render masks are suppressed to
avoid duplicate lines. If it cannot load, those masks remain untouched as a
fallback. The full Console overlay retains the complete filtered scrollback,
reflects native `conskip`, and uses the same command editor for generic
`inputcommand`.

The Console overlay mirrors every engine console line into a bounded typed
archive, including tagged-line replacement semantics. Its visible window is
rebuilt from `fullconfilter` and `conskip`; `clearconsole` clears both the
engine queue and the mirror. Warning, error, echo, game, public-chat, and
team-chat types receive semantic presentation without changing log output.
Embedded Cube color controls are converted to escaped RmlUi spans, including
the save/restore stack used by player names, so native team and duplicate-name
colors survive without allowing player text to inject markup. The full-console
mouse wheel advances the same native `conskip` value in three-line steps; the
existing keyboard and bind paths remain available.
RmlUi only owns rendering: the engine remains authoritative for toggling,
scrolling, cursor movement, clipboard, completion, history, execution, and
chat delivery. If the Console document cannot load, the legacy renderer
remains available.

`confade` affects the complete top-left console feed, while `miniconfade`,
`miniconsize`, and `miniconwidth` affect every message type a user may route
into the mini-console. They are intentionally not represented as chat-only
duration or size settings. Chat highlight sounds are absent from the current
Cowbraten source, player ignore state is transient per-connection behavior,
and the server's `specchat` policy is not a client preference, so none are
exposed by this slice. There is no dedicated legacy Chat settings document to
replace; the console tab remains available through the existing legacy
`options` content, and no new `showgui` mapping is added.

The Console category owns the rest of the native Settings `console` tab. The
typed snapshot reads `confilter`, `fullconfilter`, and `consize`. Top-left
writers toggle only the remaining native `confilter` masks: important
information `CON_INFO|CON_INIT` (`0x11`), warnings `CON_WARN`, errors
`CON_ERROR`, script messages `CON_ECHO`, game events `CON_GAMEINFO`, self
frags `CON_FRAG_SELF`, other player frags `CON_FRAG_OTHER`, and team kills
`CON_TEAMKILL`. Each write uses the same `writefilterbit` path as Chat and
preserves every unrelated bit, including `CON_CHAT` and `CON_TEAMCHAT`.
`fullconfilter` includes those chat bits because Chat does not own the
full-console mask. The native combined `miniconfilter 0x300` "chat box
(bottom left)" checkbox stays on the Chat page as independent public and team
toggles; Console does not add a second copy. `consize` is a local integer
edit buffer with native `+/- 1` steps through `setvarchecked()`. Opening
Settings, returning from the legacy console, and activating the Console
category only read these values. A typed write then reads the accepted
snapshot back. `confade`, `miniconfade`, `miniconsize`, `miniconwidth`,
`CON_DEBUG`, and ignore lists are not native console-tab controls and stay
unexposed. No new `showgui` mapping is added.
