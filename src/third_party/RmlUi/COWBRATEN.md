# Local update policy

Cowbraten calls `Context::Update(false)` to keep cached, hidden menus out of
per-frame element and layout updates. Upstream's default update visits every
document, including the thousands of elements in Settings, whenever the chat or
another overlay is visible. Opening Settings would therefore permanently add
work to gameplay frames until the UI stopped updating.

The optional argument defaults to `true` for compatibility. Data models still
update for all documents. `Show()` and explicit `UpdateDocument()` calls retain
their full update, so cached controls, pending styles, and viewport changes are
resolved before a reopened document receives focus. This policy is intended for
documents whose visibility is controlled with `Show()` and `Hide()`; it does not
skip hidden descendants inside a visible document.

Preserve this extension when updating the vendored library, or replace it with
an equivalent upstream API. `tests/ui_update_tests.cpp` exercises the behavior
against the actual library without requiring a GPU.
