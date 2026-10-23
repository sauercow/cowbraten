# Changelog

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
