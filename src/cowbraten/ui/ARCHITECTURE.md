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

Only implemented screens receive identifiers. Main and Servers are the first
production identifiers, while Test remains a developer-only identifier. Later
screens are added to the table when their documents and actions exist; disabled
Main Menu items do not create placeholder screens.

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
filtering and sorting to its copy without reordering Sauer's global list.

While Servers is active, Sauer's existing `refreshservers()` path continues to
drain ping responses. Snapshot comparison is throttled to 10 Hz, and the RmlUi
row variable is dirtied only when backend values, the filter, the sort order, or
selection actually change. Navigating away stops this work; the cached document
and owned snapshot remain available for the next visit.

The registered actions are explicit and typed:

- `navigation.servers` and `navigation.main` switch cached documents.
- `servers.select` selects the stable identifier attached to a generated row.
- `servers.filter` reads the text control's value.
- `servers.sort.*` changes a known sort key and direction.
- `servers.refresh` invokes the existing master-list refresh path.
- `servers.connect` resolves the selected identifier in the live Sauer list and
  uses the normal `cowui::close()` input-ownership path after connecting.

There is no generic host command, ident lookup, or CubeScript evaluation in the
model. Favorites and history are intentionally absent until their persistence
and product semantics are designed.
