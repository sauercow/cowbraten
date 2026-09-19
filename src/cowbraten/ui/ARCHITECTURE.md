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

`data/cowbraten/ui/default/` is the current default theme, not an engine-owned
layout contract. Concrete default screen and font paths are centralized in
`ui.cpp`; settings models and actions do not reference DOM ids, classes, RML
hierarchy, pixel positions, or style values. A future theme resolver may replace
those physical paths while retaining the same `ScreenId`, typed model fields,
and whitelisted action names. RML and RCSS remain responsible for arranging and
presenting that semantic contract.

Only implemented screens receive identifiers. Main, Servers, and Settings are
the current production identifiers, while Test remains a developer-only
identifier. Later screens are added to the table when their documents and
actions exist; disabled Main Menu items do not create placeholder screens.

At most one normal screen is active. Switching screens hides the previous
document without releasing menu input ownership, while closing the active
screen returns directly to gameplay. Navigation history and a modal stack will
belong to C++ when a production screen first needs them; they are intentionally
not generalized ahead of that requirement. Escape should eventually allow a
focused control to handle a local action, then close the top modal, then
navigate back or close the UI to gameplay.

Closing the UI hides the active document and all modals, releases RmlUi focus,
and returns cursor/grab ownership through the existing input bridge. Cached
documents remain loaded until shutdown. Reopening is an explicit request for a
screen rather than an implicit DOM operation.

The legacy Sauer command line temporarily takes presentation and text-input
ownership without closing the active RmlUi screen. Its normal `saycommand`,
`sayteamcommand`, and `inputcommand` key bindings are recognized by the legacy
input layer. RmlUi then releases control focus and hides its cached document so
Sauer can render and operate the command line normally. Closing the command
line reshows the same document while retaining the menu's existing cursor/grab
ownership throughout the transition.

Legacy `showgui` calls pass through an exact-name compatibility router before
the legacy menu table. The currently migrated names are `main` -> Main,
`servers` -> Servers, and `options` -> Settings. A match uses the same cached
`ScreenId` navigation path as the direct `/ui_*` commands; there is no command
parsing, partial matching, or DOM-based routing. More names are added only when
their production screen exists.

An existing, non-migrated legacy GUI keeps the original `showgui` path. If it
is requested while RmlUi owns the menu, RmlUi closes through its normal input
restoration path before the legacy GUI opens. An unknown name remains the
legacy no-op and does not disturb the active RmlUi screen. While RmlUi is
visible, the legacy GUI stack is not rendered, which prevents two menu layers
from competing while preserving legacy stack state for a later handoff.

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

There is no generic host command, ident lookup, or CubeScript evaluation in the
model. Favorites and history are intentionally absent until their persistence
and product semantics are designed.

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
refresh the Audio model. Sound enablement, channel count, sample frequency,
buffer length, and driver choice remain outside this first Audio slice because
they require `resetsound()` and additional runtime fallback/readback policy.

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

No keyboard binding API is exposed yet. A future binding milestone must own
capture state, cancellation, conflict policy, multiple bindings, readable key
names, and persistence explicitly; it must not pass arbitrary bind commands
from markup.

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
unexposed. No `showgui gameclock` alias is added; the standalone legacy
document remains reachable by that exact name.

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
exposed. `showgui "hudfragmessages"` remains the legacy standalone document;
Settings no longer needs that nested GUI.

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
match. `showgui "ammobar"` remains the legacy standalone document; Settings no
longer needs that nested GUI.

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
`showgui "scoreboard"` remains the legacy document.

Native `showgui gameclock`, `showgui hudscore`, and `showgui scoreboard` remain
on the legacy path because those slices inline Settings HUD rather than mapping
the adjust document names. No HUD or Crosshair compatibility alias is added
until the corresponding RmlUi surface is a sufficiently complete semantic
replacement.

The first Chat slice preserves Sauer's actual console-backed message model.
Public and team chat enter the shared console buffer as the distinct
`CON_CHAT` and `CON_TEAMCHAT` message types. Explicit typed writers toggle
only those two bits in the persistent `confilter` top-left feed and
`miniconfilter` bottom-left mini-console masks, preserving every unrelated or
custom filter bit. `teamcolorchat` remains a separate persistent boolean that
controls player-name coloring when chat lines are formatted. All five values
apply immediately and are read back from their fixed legacy idents after each
write, on Chat activation, on Settings activation, and after legacy-console
handoff. Message delivery, `saycommand`, `sayteamcommand`, input ownership,
and scrollback are unchanged.

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
