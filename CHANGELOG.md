# Changelog

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
