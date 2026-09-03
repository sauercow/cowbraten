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

Only test actions are registered during this foundation phase.

## Documents and navigation

Each top-level screen should be a separate RML document sharing the foundation
styles. A small C++ screen manager should own explicit screen identifiers and
asset paths. It should lazily load each document once, cache it for the RmlUi
context lifetime, and show or hide documents on transitions.

The initial screen identifiers will cover Main, Servers, Demos, Profile,
Settings, Console, and Credits. Test remains a developer-only screen. Defining
an identifier does not require loading its document.

The manager, not the DOM, should own a history of screen identifiers. At most
one normal screen is active. Modal documents are separate cached documents in a
small modal stack above the active screen. Escape should first allow a focused
control to handle a local action, then close the top modal, then navigate back
or close the UI to gameplay.

Closing the UI hides the active document and all modals, releases RmlUi focus,
and returns cursor/grab ownership through the existing input bridge. Cached
documents remain loaded until shutdown. Reopening is an explicit request for a
screen rather than an implicit DOM operation.

Application behavior must not depend on element IDs. IDs remain acceptable for
document-local relationships such as a label's `for` attribute. State reaches
RML through event-driven data models and stable actions; documents are not
rebuilt or copied every frame. When the UI is closed, the manager must preserve
the existing no-update/no-render fast path.
