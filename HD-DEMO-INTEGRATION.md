# Cowbraten rc3 – HD demo integration (stage 1 only)

This stage adds L4ZY-compatible **local `.dmohd` sidecar recording** without replacing the existing `.dmo` writer or its HiFi aim/shot messages.

## Usage

In the game console, run `demohdrecord 1`, then `recordclientdemo 2` on a multiplayer server. Stop with `stopclientdemo`. The existing `.dmo` file is unchanged in format; a sibling `.dmohd` is written when HD is enabled. `demohdrecord` defaults to `0`, and changing it takes effect at the next recording start. If sidecar creation fails, the normal `.dmo` keeps recording.

Captures up to one camera sample every 4 ms (maximum 250 Hz), including precise camera yaw, pitch, roll, eye position, zoom (five-bit quantized), weapon and player state. The actual cadence depends on the rate at which the game updates its camera.

## Limitations (intentional)

- Stage 1 **does not read or play back `.dmohd`**; no live HD spectating, server handshake, server-side demo transfer, or client protocol changes yet.
- Server-created `.dmo` and other-player HD tracks remain future stages.
- Compile and runtime testing must be done in the full Cowbraten tree; the supplied archive contains selected sources only.

## Credits

The sidecar file layout follows the L4ZY-source HD demo v1 extension by Lazy-Apathy and contributors, under the zlib license. Attribution to those authors is required where the adapted implementation is distributed; retain upstream notices and review the final combined distribution license texts.

## Stage 2 – Optional HD playback

Stage 2 adds an optional reader for the L4ZY `SAUERHD` v1 `.dmohd` sidecar. It does not replace Cowbraten's `N_P1X_DEMO_AIM` or `N_P1X_DEMO_SHOT_AIM` handling; **only tracks flagged HD_OWN (exact aim) override the existing HiFi display**. Missing, corrupt, or unsupported HD tracks simply use the existing demo decoder. The reader caps inputs at 96 MiB and requires chronological samples per client. Timestamps use `server::gamemillis`, so pause/seek/rewind and speed changes reuse Cowbraten's current timeline. No protocol additions in this stage.

Commands:

- `demohdplay 1` / `demohdplay 0`: enable or disable the HD overlay.
- `toggledemohd`: toggle HD overlay on the current demo.
- `demohdloaded`: 1 when a readable sidecar contains exact aim frames.
- `demohdrecorder`: CN reported in HD header, or -1 if not loaded.
- `demohdstatus`: `HD` only while the followed player has a precise track and HD playback is on.
- `hdcurve 1` / `hdcurve 0`: monotone cubic Hermite / linear frame interpolation.

To test: play a recorded `<name>.dmo` with its sibling `<name>.dmohd`, follow the recorded player (`/follow <CN>`), then toggle `demohdplay`. Try pausing, seeking backward and forward, and changing `demospeed`. The regular .dmo remains the single source of network events, kills, and demo time. Stage 2 has not been built or gameplay-tested outside the user's full repo.
