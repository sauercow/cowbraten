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
buffer length, driver choice, and sound-pack selection are intentionally not
part of this immediate-volume slice: they require `resetsound()`, backend
fallback/readback policy, or separate asset-selection semantics.

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
