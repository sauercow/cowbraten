# cowbraten

This repository contains **cowbraten**, a standalone-branded fork of [p1xbraten](https://github.com/sauerbraten/p1xbraten) with a native RmlUi interface and additional demo playback, HUD, crosshair, sound, and quality-of-life features. It retains p1xbraten network detection compatibility while versioning and packaging cowbraten separately.

Current fork version: **1.1.0-rc.3**, based on **p1xbraten 9.0.0**. See the [changelog](./CHANGELOG.md) for release highlights and the [RmlUi status document](./RMLUI_ROADMAP.md) for implementation details and remaining release work.

## Release plan

- **1.0.1** is the stable classic-interface line.
- **1.1.0-rc.3** is the current RmlUi release candidate. It includes the complete 1.0.1 line, migrates ordinary legacy menu paths, and keeps only explicit engine and compatibility boundaries.
- **1.1.0** stable is reserved for final cross-platform verification of the RC3 interface and packaging baseline.
- All of these lines continue to report p1xbraten 9.0.0 compatibility and keep the existing Sauerbraten network protocol and standard demo format.

## 1.1.0-rc.3 highlights

- A native, responsive RmlUi interface for the normal player flow: identity, game setup, servers, demos, settings, administration, utilities, map editing, in-game menus, chat, console, and scoreboard.
- A shared application shell, semantic icon set, alternate visual theme, independent interface/text scaling, and selectable **Legacy** or **Lato** interface typography. Legacy uses Sauerbraten's own `packages/fonts/default.ttf`, matching the console and FPS text from the classic client.
- A rebuilt demo library with metadata, search and classification, favorites, recoverable trash, playback transport controls, and local demo visualization options including X-RAY and hitboxes.
- Server favorites, profile presets and model-spoofing controls, safe autoexec profiles, automatic chat ignore rules, and typed administration actions.
- Responsive gameplay overlays and scoreboard, including compact-screen behavior and a top-left feed that uses the available width while preserving minimap clearance.
- Native compact reconstruction of configurable `newgui` and Autoexec menus, including their controls, previews, tabs, lifecycle hooks, and original actions.
- Native loading, status HUD, ammo displays, mode indicators, match score, frag feedback, and apply-confirmation presentation.
- Legacy menu scripts disabled by default through a persistent startup switch, with their files and explicit compatibility fallback retained.
- Cached passive overlays that avoid per-frame layout invalidation and substantially reduce gameplay UI overhead.
- A typed C++ boundary between documents and engine behavior: interface markup cannot execute arbitrary CubeScript or select arbitrary engine identifiers.

- [Installation](#installation)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
- [Menu](#menu)
- [Patches](#patches)
  - [upstreamed to SVN](#upstreamed-to-svn)
  - [moviehud.patch](#moviehudpatch)
  - [scoreboard.patch](#scoreboardpatch)
  - [hudfragmessages.patch](#hudfragmessagespatch)
  - [fullconsole.patch](#fullconsolepatch)
  - [hudscore.patch](#hudscorepatch)
  - [serverbrowser.patch](#serverbrowserpatch)
  - [listteams.patch](#listteamspatch)
  - [settex.patch](#settexpatch)
  - [decouple\_framedrawing.patch](#decouple_framedrawingpatch)
  - [crosshaircolor.patch](#crosshaircolorpatch)
  - [gamehud.patch](#gamehudpatch)
  - [modversion.patch](#modversionpatch)
  - [ignoreminimize.patch](#ignoreminimizepatch)
  - [playerspeed.patch](#playerspeedpatch)
  - [up\_down\_hover.patch](#up_down_hoverpatch)
  - [paused\_spec\_movement.patch](#paused_spec_movementpatch)
  - [clientdemo.patch](#clientdemopatch)
  - [colored\_weapon\_trails.patch](#colored_weapon_trailspatch)
  - [crosshairreloadfade.patch](#crosshairreloadfadepatch)
  - [Dynamic sound packs](#dynamic-sound-packs)
  - [better\_console.patch](#better_consolepatch)
  - [nextfollowteam.patch](#nextfollowteampatch)
  - [setfont.patch](#setfontpatch)
  - [fullscreenmonitor.patch](#fullscreenmonitorpatch)
- [Server Patches](#server-patches)
  - [server\_authservers.patch](#server_authserverspatch)
  - [server\_logging.patch](#server_loggingpatch)
  - [server\_slim\_ogzs.patch](#server_slim_ogzspatch)
  - [server\_managed\_games.patch](#server_managed_gamespatch)
  - [server\_autoauthdomains.patch](#server_autoauthdomainspatch)
  - [server\_proxy\_setip.patch](#server_proxy_setippatch)
  - [server\_botbalanceearly.patch](#server_botbalanceearlypatch)
  - [server\_bans.patch](#server_banspatch)
- [Credits](#credits)
- [Project Structure](#project-structure)
- [Building your own binary](#building-your-own-binary)
  - [Build dependencies](#build-dependencies)
  - [Building](#building)
  - [Using fresh upstream sources](#using-fresh-upstream-sources)
  - [Debugging](#debugging)


## Installation

Source tags for cowbraten are published at https://github.com/sauercow/cowbraten/tags. cowbraten uses the normal Sauerbraten game content, which must already be installed or otherwise available.

Packages from https://github.com/sauerbraten/p1xbraten/releases/latest contain upstream p1xbraten and do not include cowbraten-specific features or branding.

### Windows

The Windows target can be cross-compiled on Linux with Zig. The recommended Zig version is specified in `.zig-version`. With Zig, CMake, curl, and NSIS installed, run `./package.sh windows` to build `cowbraten_x86_64_client.exe` and create `dist/cowbraten_setup_<version>.exe`. The packaging script downloads and verifies the pinned FreeType source needed by the static RmlUi build. The installer adds the Cowbraten binary, launcher, and `data/cowbraten` interface resources alongside an existing Sauerbraten installation so the original game content remains available.

### macOS

Install the normal Sauerbraten application/content, install the build dependencies, then follow [Building your own binary](#building-your-own-binary). Keep a backup of `Sauerbraten.app/Contents/MacOS/` before replacing its client binary.

### Linux

Install the normal Sauerbraten runtime/content and the build dependencies, then follow [Building your own binary](#building-your-own-binary). From this repository, `make install` places the client in `bin_unix/`, and `./start.sh` launches it using the configured Sauerbraten content directory. Binary distributions include `bin_unix/`, the launcher, and the required `data/cowbraten` interface resources; keep that relative directory structure when extracting or updating manually.

Release builds provide both a portable ZIP and an executable `cowbraten_setup_linux_<version>.run` installer. Run the installer and select the root of an existing Sauerbraten installation; it copies the executable, launcher, and Cowbraten interface assets into the correct relative paths. It does not duplicate or replace Sauerbraten's original game content.


## Menu

Cowbraten 1.1 uses a native RmlUi layer for the normal player interface: Profile, Play and game setup, Servers, Demos, Settings, Administration and About, text utilities, map editing, the in-game menu, chat, console, and scoreboard. These screens use typed C++ APIs for engine state and preserve the existing Sauer configuration format.

The specialized in-engine 3D texture palette and spatial radar/minimap remain engine-owned. User and third-party `newgui` documents are reconstructed as RmlUi panels while keeping their evaluated labels, editor buffers, progress, package textures, 3D previews, configuration fields, tabs, lifecycle hooks, and original engine-owned actions. Legacy menu files remain installed for compatibility, but their UI declarations are not loaded by default.

If you do not want to use the cowbraten menus, run `/usecowbratenmenus 0`.

To restore the original legacy menu declarations for compatibility, enable **Settings → Advanced → Autoexec → Legacy menu fallback** and restart, or set `/loadlegacymenus 1` before the next launch.


## Patches

a.k.a. Features

### upstreamed to SVN

- "fixes" `nextfollow` to start at the first/last player when cycling forwards/backwards, instead of second or second to last respectively
- allows using <kbd>Ctrl</kbd>-<kbd>Left</kbd>/<kbd>Right</kbd> (<kbd>Option</kbd> on Mac) to jump over words in the console input
- allows using <kbd>Ctrl</kbd>-<kbd>Backspace</kbd>/<kbd>Delete</kbd> (<kbd>Option</kbd> on Mac) to delete entire words in the console input
- improves player name matching in `setmaster`, `setteam`, `ignore`, `kick`, `spectator`, `follow` and `goto` commands: after the normal full matching, (if `parseplayer`>0) the argument is tried to be matched as a case-insensitive prefix, then a case-insensitive substring of a player's name

### [moviehud.patch](./patches/moviehud.patch)

- adds `namesabovehead` toggle: when 0, hides the names above players' models (usually rendered as particle text), while keeping status icons for health boost, armor and quad

### [scoreboard.patch](./patches/scoreboard.patch)

- enables suicide and detailed, per-weapon damage recording

  Note that suicide and damage recording is only accurate in games you observed from the beginning. When you join a running game, all previous damage is unknown, and suicides are only initialized from negative frag counts.

- cleans up the scoreboard look (for example, right-justifies numeric scoreboard columns)

- adds scoreboard toggles:
    - `showflags`: when 1, shows the number of flags scored by a player; always hidden in non-flag modes
    - `showsuicides`: when 1, shows how often each player suicided
    - `showkpd`: when 1, shows each player's frags/death ratio
    - `showaccuracy`: when 1, shows each player's overall accuracy
    - `showdamage`: when 1, shows each player's overall damage dealt; when 2, shows each player's overall net damage (= dealt - received); always hidden in insta modes
    - `showdamagereceived`: when 1, shows each player's damage received

  ![example screenshots](https://i.imgur.com/hsCY56E.gif)

- makes the following built-in cubescript commands take an optional player CN as argument:
    - `getfrags`
    - `getflags`
    - `getdeaths`
    - `getaccuracy`
    - `gettotaldamage`
    - `gettotalshots`
    - `getweapon`

  Omitting the argument will return the information about `hudplayer`, i.e. yourself (if playing) or the player you're currently spectating.

- adds damage-related cubescript commands:
    - `getdamagepotential` (same as `gettotalshots` when not filtering by weapon)
    - `getdamagedealt` (same as `gettotaldamage` when not filtering by weapon)
    - `getdamagereceived`
    - `getdamagewasted` (= potential − dealt)
    - `getnetdamage` (= dealt − received)

  All of the new damage-related commands (as well as `getaccuracy`) default to showing `hudplayer`'s stats across all weapons. However, they all take two optional integer arguments to query stats of a certain player and optionally for a specific weapon: `/<cmd> [cn] [weapon]` (use -1 as CN to query specific weapon stats about `hudplayer`).

  To show comed-like statistics in the lower right corner, copy the following into your `autoexec.cfg`:
  ```
  gamehud = [
    format "^f7SG: ^f1%1%% ^f7CG: ^f1%2%% ^f7RL: ^f1%3%% ^f7RI: ^f1%4%% ^f7GL: ^f1%5%% ^n^f7frags: ^f0%6 ^f7deaths: ^f3%7 ^f7acc: ^f2%8%% ^f7kpd: ^f5%9" (
      round (getaccuracy -1 1) 0.1                    )(
      round (getaccuracy -1 2) 0.1                    )(
      round (getaccuracy -1 3) 0.1                    )(
      round (getaccuracy -1 4) 0.1                    )(
      round (getaccuracy -1 5) 0.1                    )(
      getfrags                                        )(
      getdeaths                                       )(
      round (getaccuracy) 0.1                         )(
      round (divf (getfrags) (max (getdeaths) 1)) 0.1	)
  ]
  ```
Use `/usecowbratenmenus 0` to use the default scoreboard.

### [hudfragmessages.patch](./patches/hudfragmessages.patch)

- enables frag messages showing the weapon used to complete the frag (on by default)
- adds the following variables:
    - `hudfragmessages`: 0 disables all frag messages, 1 enables them for all modes, 2 enables them for non-insta modes only
    - `maxhudfragmessages`: how many messages to show at most (between 1 and 10)
    - `hudfragmessageduration`: how long each message will be shown, in seconds, between 0 and 10
    - `hudfragmessagefade`: 0 or 1: whether or not to fade out messages
    - `hudfragmessagex`: horizontal position (between 0.0 and 1.0) where messages will appear
    - `hudfragmessagey`: vertical position (between 0.0 and 1.0) where the newest message will appear
    - `hudfragmessagescale`: size of the messages, between 0.0 and 1.0
    - `hudfragmessagestackdir`: direction in which to stack old messages: -1 to stack upwards, 1 to stack downwards, 0 to stack towards the closes edge of the screen depending on `hudfragmessagey`
    - `hudfragmessagefilter`: bitfield filter var (like confilter), e.g. 0x3800 shows all players' frags, suicides, and teamkills
    - `hudfragmessageforcecolors`: when 1 (default), uses colored names, even in non-team modes

You can easily configure the hud frag messages using the [improved menu](#menu) (options → hud → adjust hud frag messages).

#### Kill streak indicators

The frag-message HUD also tracks the local player's consecutive enemy kills:

- the second, third, fourth and subsequent kills replace the attacker name with a yellow `x2`, `x3`, `x4`, etc.;
- milestones at 5, 10, 15, 20 and 25 kills temporarily replace the regular frag message with a larger `5 STREAK`, `10 STREAK`, etc.;
- dying or starting a new game resets the streak;
- suicides and team kills do not increase it.

Streak messages reuse `hudfragmessageduration`, `hudfragmessagefade`, `hudfragmessagex`, `hudfragmessagey` and `hudfragmessagescale`, so they stay aligned with the rest of the cowbraten frag HUD. `streakmilestonescale` controls the additional size multiplier used by milestone messages (1.25 by default). Setting `hudfragmessages` to `0` hides both regular frag and streak messages.

![fragmessages](https://i.imgur.com/K4GL6oB.png)

### [fullconsole.patch](./patches/fullconsole.patch)

- removes the guiskin background of fullconsole in favor of a gradient background on the left
- improves interplay of scoreboard and fullconsole
- `fullconsize` is deprecated and unused, instead the console always takes up all available space
- adds `fullconalpha` var: sets the opacity of fullconsole's left edge in percent (from 1 to 100)

### [hudscore.patch](./patches/hudscore.patch)

- hides hudscore when there is just one team/player

### [serverbrowser.patch](./patches/serverbrowser.patch)

- styles the serverbrowser to be consistent with the cleaned up scoreboard
- divides 'players' column into 'players' and 'slots'
- right-justifies ping, players, slots, and port columns
- adds the `serverbrowsershow[ping|players|slots|mode|map|time|master|host|port|description]` variables: set to 0 to hide the respective column from the UI
- adds the `filterservers` string variable to only show servers in the serverbrowser whose mode, map or description matches (using case-insensitive substring matching)

The cowbraten menu gives you a simple search field on the server browser using this variable. Use `/usecowbratenmenus 0` to use the default server browser.

### [listteams.patch](./patches/listteams.patch)

- adds `listteams` command that returns a list of names of non-empty teams (according to filters) when two or more teams exist and an empty string otherwise (`listteams [includeself?] [includebots?] [includespecs?]`)
- extends `listclients` command to allow omitting spectators and only listing clients of a certain team (`listclients [includeself?] [includebots?] [includespecs?] [teamname?]`)

[The updated menu](#menu) includes a better master menu thanks to these changes:

![master menu](https://i.imgur.com/doC2IcB.png)

### [settex.patch](./patches/settex.patch)

- adds `settex <id>` command to apply a texture by ID

### [decouple_framedrawing.patch](./patches/decouple_framedrawing.patch)

- removes fps-induced limiting of the main loop (for example, input & network processing were affected by `maxfps`)
- introduce `maxtps` var to limit the main loop independently from `maxfps` (for example to save power): 0 to disable limiting (default), 100..1000 to set how many ticks per second are allowed; `maxfps` overrides `maxtps` if `maxfps > maxtps`

This patch allows using `maxfps` without compromising on the frequency of input polling and network event processing. In vanilla Sauerbraten, `maxfps` limits the whole game's main loop. This patch removes the main loop limiting and instead skips only frame drawing until a new frame is needed, but still processes network events and player input on every main loop iteration.

While vanilla Sauerbraten is truly unlimited after `/maxfps 0`, cowbraten's main loop can additionally be limited using the `maxtps` variable. Setting `maxfps` to any value other than 0 also forces a maximum tick frequency of 1000 (but respects lower settings), even if you set `maxtps 0`, so that cowbraten can prioritize frame draw timing. Set both `maxfps` and `maxtps` to 0 to get vanilla's unlimited behavior back.

Using `maxfps` and `maxtps`, you can optimize for different goals:

- for lowest average latency, use `/maxtps 0` and `/vsync 0`
- for lowest consistent latency, use `/maxfps 0`, `/vsync 0`, and set `maxtps` to the highest value that gives you a stable fps counter on your system
- to use fewer resources and save laptop battery, use `/maxfps 0`, and set `maxtps` to the lowest value that gives you smooth graphics

### [crosshaircolor.patch](./patches/crosshaircolor.patch)

- adds `crosshaircolor` variable to set a base crosshair color (for example, `/crosshaircolor 0 255 0` for pleasant green)

### [gamehud.patch](./patches/gamehud.patch)

- properly right-justifies gamehud, wallclock, showfps, and/or showfpsrange lines
- adds a useful playerlist showing who's alive vs. dead when spectating in the lower right corner
- adds `isdead <cn>` command to check if a player is currently dead (only works when you are spectating)
- adds the `hasflag <cn>` command: returns 1 if the specfied player carries a flag

<div align="center"><img alt="gamehud with player state" src="https://i.imgur.com/8kfmhSK.gif" /></div>

### [modversion.patch](./patches/modversion.patch)

- retains the `p1xbratenversion` variable (read-only) for compatibility with upstream p1xbraten detection
- adds the `cowbratenversion` variable (read-only): contains the separate cowbraten release version
- retains the `getclientp1xbratenversion <cn>` command: returns the compatible p1xbraten protocol version advertised by a player
- retains the `enablep1xbratendetection` variable: set to 0 to disable broadcasting the compatible version and detecting other compatible clients

This compatibility layer lets clients recognize cowbraten and p1xbraten users on the same server. It uses the existing p1xbraten packet prefix so cowbraten does not fragment the established protocol. Only the compatible client version is broadcast through the game server; it is not sent directly to the cowbraten or p1xbraten maintainers.

For an overview of compatible clients, run `/showgui cowbratenusage`.

### [ignoreminimize.patch](./patches/ignoreminimize.patch)

- adds the `ignoreminimize` toggle: when 1, the game will not stop playing sounds or drawing frames when minimized

### [playerspeed.patch](./patches/playerspeed.patch)

- adds the `playerspeed` variable (read-only): returns the hud player's speed (in units per second)

### [up_down_hover.patch](./patches/up_down_hover.patch)

- adds the `up`, `down` and `hover` movement commands:
  - `up`/`down` induce movement up or down relative to the player's view
  - `hover` prevents any change in height from forward/backward movement and making `up` and `down` work along the world's Z axis

  all three commands only work while editing or spectating

### [paused_spec_movement.patch](./patches/paused_spec_movement.patch)

- enables free movement as spectator while the game is paused

### [clientdemo.patch](./patches/clientdemo.patch)

- adds the `recordclientdemo <0|2>` command: 0 is the same as `stopclientdemo`; 2 starts recording a demo immediately
- adds the `stopclientdemo` command: stops an ongoing demo recording

Client-side demos record the game as you experience it, and work independently of server-side demo recording. They also work locally, without being connected to any server. Your own position, movement and view is recorded at 125 frames per second.

The Demo Manager Recording tab can automatically start a client demo after each real map load. Its optional **Online Matches Only** setting limits that automatic path to remote servers, skipping local maps and offline bot matches; manual `recordclientdemo 2` commands are unaffected.

The RmlUi Autoexec settings select `original` (`autoexec.cfg`), `none`, or a named `autoexecs/<name>.cfg` profile for startup. The selection is saved in `autoexec-profile.cfg` in the user configuration directory, so an older client sharing that directory cannot erase it when rewriting `config.cfg`. Existing selections migrate from `config.cfg` on the first normal save. Startup and live profile loading clear the previous `mapstart` hook before executing the selected profile; an explicit `recordclientdemo 2` hook in that profile still records independently of `autorecordclientdemo`.

The compatibility regression can be run from the repository root with `bash tests/autoexec_profile_integration.sh /path/to/sauerbraten /path/to/legacy/cowbraten_client`. It uses a temporary configuration directory and checks profile persistence across both clients, recording behavior across map changes, disabled profiles, and named configuration exports.

#### Demo status and playback controls

`huddemorecording` enables the blinking `REC` indicator while recording a client demo. `huddemoplayback` enables the playback HUD, and `huddemoplaybackscale` controls its size. The playback HUD shows the current position, accurate demo duration, playback state and speed. While paused or dragging the timeline, it displays millisecond precision.

The default playback controls are:

| Input | Action |
| --- | --- |
| <kbd>Space</kbd> | Pause or resume playback |
| <kbd>Left</kbd> / <kbd>Right</kbd> | Rewind or advance 5 seconds |
| <kbd>Shift</kbd> + <kbd>Left</kbd> / <kbd>Right</kbd> | Rewind or advance 30 seconds |
| <kbd>Home</kbd> / <kbd>End</kbd> | Seek to the beginning or end |
| <kbd>Alt</kbd> + <kbd>Left</kbd> / <kbd>Right</kbd> | Step to the previous or next recorded frame |
| <kbd>Alt</kbd> + <kbd>Up</kbd> / <kbd>Down</kbd> | Increase or decrease playback speed |
| Hold <kbd>Alt</kbd> | Use the timeline and playback buttons with the mouse |

The speed button cycles through 0.25x, 0.50x, 1.00x, 2.00x and 4.00x. Playback speed returns to 1.00x when the demo ends. Rewinding and frame stepping preserve the currently followed player's point of view.

Run `/demoplaybackhelp` to print the controls in-game. The following commands can also be used directly or in custom binds:

- `demoplaypause`
- `demoforward <seconds>` and `demorewind <seconds>`
- `demoseek <milliseconds>`
- `demostep <-1|1>`
- `demospeed <percent>`, `demospeedup` and `demospeeddown`

Set `demoplaybackkeys` to `0` to disable the built-in keyboard shortcuts before replacing them with custom binds. Mouse interaction with the playback HUD remains available.

### [colored_weapon_trails.patch](./patches/colored_weapon_trails.patch)

- adds the `trailcolor[shotgun|chaingun|rocketlauncher|rifle|grenadelauncher|pistol]` vars that change the respective weapon's projectile/particle trail
- adds the `settrailcolor <SG|CG|RL|RI|GL|PI> <color>` command (for convenience: changing the variables directly also works)

You can also adjust the trail colors in the options menu.

### [crosshairreloadfade.patch](./patches/crosshairreloadfade.patch)

- `crosshairreloadfade` uses red to indicate that the insta rifle is still reloading, then transitions back to the ready crosshair color as the reload completes; set it to `0` to disable the color transition
- `crosshairreloadrotate` rotates the insta rifle's crosshair from 45 degrees back to its normal orientation during the same reload period; set it to `0` to disable the rotation

The reload color is a weapon-readiness indicator; it does not represent player health. This is separate from cowbraten's inherited health-based crosshair colors in non-insta modes. The transition returns to the selected ready color and requires `crosshairfx` and `crosshaircolors` to be enabled. Rotation is independent and can be used by itself. Together they provide a visual reload timer without adding another HUD element.

### Dynamic sound packs

cowbraten can discover and switch game sound packs without restarting the client. Choose one from **options → sound → choose sound pack**, set it directly with `/soundpack <name>`, or return to Sauerbraten's sounds with `/soundpack original`. The selection is saved in the client configuration.

Custom packs use this layout:

```text
soundpacks/
└── <pack-name>/
    ├── sounds.cfg
    └── sounds/
        └── ... .wav/.ogg files
```

`/listsoundpacks` returns every valid installed pack, with `original` first. A pack is listed only when it has both `sounds.cfg` and a non-empty `sounds/` directory. When a requested sample is absent from the selected pack, cowbraten falls back to the matching file under Sauerbraten's original `packages/sounds/`. If the selected pack becomes invalid or cannot load its configuration, the client restores the original sounds automatically.

### [better_console.patch](./patches/better_console.patch)

- adds name completion to chat console (press <kbd>Tab</kbd>)

Name completion tries old-style prefix matching first, then will also try substring matching to suggest more names. In addition to vanilla's matching at the end of the line, it also works in the middle of the input, as long as the cursor is placed in front of a space character.

If you want to keep using <kbd>Tab</kbd> to complete commands in the chat prompt (without explicitly typing '/' at the beginning), use this bind:
```
bind "T" [inputcommand "" "" "" "c"] // instead of saycommand
```

Also, you can put the following into your autoexec.cfg for cleaner command prompts (replace the keys to your liking):
```
bind "SLASH"     [inputcommand "" [$commandbuf] "/" "cx"]
bind "BACKQUOTE" [inputcommand "" [$commandbuf] "/" "cx"]
bind "CARET"     [inputcommand "" [$commandbuf] "/" "cx"] // BACKQUOTE on German keyboards
bind "BACKSLASH" [inputcommand "" [servcmd $commandbuf] "#"] // works for #commands on zeromod, spaghetti, remod and p1xbraten servers
bind "HASH"      [inputcommand "" [servcmd $commandbuf] "#"] // only works with German keyboard layout
```

### [nextfollowteam.patch](./patches/nextfollowteam.patch)

- adds the `nextfollowteam` command that will cycle through players grouped by team and falls back to vanilla's `nextfollow` in non-team modes (which just iterates through CNs from low to high)

To use it, put the following lines into your autoexec.cfg (and remember you'll have to change them back to `nextfollow` before using the autoexec.cfg with other clients!):
```
specbind "MOUSE1" [nextfollowteam]
specbind "MOUSE2" [nextfollowteam -1]
```

### [setfont.patch](./patches/setfont.patch)

- adds the `setfont <name>` command to set the UI font after registering the font using `font <name> ...` (`setfont "default"` to reset)

### [fullscreenmonitor.patch](./patches/fullscreenmonitor.patch)

- adds the `fullscreenmonitor` variable: specifies on which display the game runs on when in fullscreen mode, 0 being the primary display
- adds the `getnummonitors` command: returns the number of connected displays
- adds the `getmonitorname <id>` command: returns the name of the monitor with the specified index

## Server Patches

These are the patches that make p1x.pw different from other servers.

### [server_authservers.patch](./patches/server_authservers.patch)

- adds `addauthserver <key_domain> <hostname> <port> <privilege>` server command to add an additional auth backend (other than the master server)

For example, you can put `addauthserver "p1x.pw" "p1x.pw" 28787 "m"` into your `server-init.cfg` to allow users registered with my master server to claim auth on your server.

### [server_logging.patch](./patches/server_logging.patch)

Improves logging when running a dedicated server:

- adds `logtime` variable: if 1, all log messages are prepended with ISO date and time (e.g. `[2021-04-06 17:12:05]`)
- includes CN in connect, disconnect, chat, team chat log messages
- logs a "join" message including CN and player name
- logs map changes
- logs all privilege changes

### [server_slim_ogzs.patch](./patches/server_slim_ogzs.patch)

- allows using slim .ogz files (see https://github.com/sauerbraten/genserverogz) on the server without getting `checkmaps` errors

### [server_managed_games.patch](./patches/server_managed_games.patch)

- adds the `#competitive [MM[:SS]]` remote command, that will (for the next match only):
  - enable server demo recording and request client demo recording from p1xbraten users
  - optionally, set the time to MM:SS
  - wait for all players to have loaded the map before starting
  - respawn all players on game start
  - auto-pause the game when a player disconnects or goes to spec
  - show a count down before resuming
  - accept client demo uploads at intermission
  - save all demo files of the match to disk
  - not auto-load the next map
- adds the `autolockedcompetitive` variable: if 1, games will automatically start in competitive mode while master mode is locked or higher
- adds the `#specmute` remote variable: when 1, spectators can't send any chat messages (requires auth or admin)
- adds the `#tourney [MM:[SS]]` remote command, which is the same as `#competitive`, but also enables specmute for the next match
- adds the (admin-only) `#do` remote command to evaluate arbitrary cubescript on the server, for example `#do persistteams 1`, `#do servermotd "happy fragging"` or `#do concat $serverauth`
- adds the server-side `sendservmsg` cubescript command, so admins can broadcast something using `#do sendservmsg foo`

### [server_autoauthdomains.patch](./patches/server_autoauthdomains.patch)

- adds the `addautoauthdomain` command to request an auth key automatically right after a player joined (*not* before joining; this won't let the player connect in private mode: only the server auth domain can be used for that!)

### [server_proxy_setip.patch](./patches/server_proxy_setip.patch)

- adds the `addtrustedproxyip` command to declare a packet source IP as belonging to a trusted proxy

This patch adds support for the [SauerDuels proxy](https://github.com/sauerduels/sauer-proxy) which can be used to obfuscate the IP of tournament servers. The `setip` protocol extension allows the proxy to forward the client's real IP, replacing the proxy's IP.

### [server_botbalanceearly.patch](./patches/server_botbalanceearly.patch)

- adds the `serverbotbalanceearly` variable: if 1 (and `serverbotbalance` is also enabled), the server will equalize the player count of all teams by adding/reassigning/removing bots

This is a more aggressive form of auto-balance than vanilla's `serverbotbalance`, which only reassigns bots, and never adds or removes bots on its own. It's intended for public play and disables manually adding/removing bots.

### [server_bans.patch](./patches/server_bans.patch)

- adds the `getip <cn>` command to return a client's IP
- adds the `ban <ip> <minutes>` command to ban an IP for the specified duration (or 30 minutes if not specified)
- adds the `unban <ip>` command to remove any ban created earlier
- adds the `listbans` command to list all currently active IP bans

All of these commands are admin-only. You can use these from cubescript or as a remote command (e.g. `#listbans`).


## Credits

Cowbraten is derived from [p1xbraten](https://github.com/sauerbraten/p1xbraten), created and maintained by **p1x**, and from [Cube 2: Sauerbraten](http://sauerbraten.org/). The engine, game, content, and the substantial upstream history remain the work of their respective authors and contributors. Cowbraten development also includes contributions present in this repository from **sauercow**, **Lazy-Apathy**, and **27thLiz**.

The 1.1 interface is built with [RmlUi](https://github.com/mikke89/RmlUi) 6.3 and includes its upstream license in `src/third_party/RmlUi/LICENSE.txt`. Its Lato faces were designed by **Łukasz Dziedzic** and are distributed under the SIL Open Font License 1.1. The Legacy option uses the existing Sauerbraten `packages/fonts/default.ttf` asset (DejaVu Sans Condensed) rather than redistributing another copy. Cowbraten's semantic UI icons are original project assets released under CC0 1.0.

Thank you to the players and testers who exercised the new interface, demo paths, display modes, and multiplayer behavior during the release-candidate cycle.


## Project Structure

- `patches/` contains the patches that build cowbraten on top of vanilla Sauerbraten
- `src/` contains the cowbraten source tree
- `src/cowbraten/ui/` contains the typed RmlUi integration and its engine bridges
- `src/third_party/RmlUi/` contains the pinned RmlUi 6.3 source and upstream license
- `Makefile` has commands to work with the patches and an SVN checkout of vanilla
- `data/` contains cowbraten-specific CubeScript and RmlUi documents, styles, fonts, icons, themes, and their licenses
- `tests/` contains focused interface and behavior regression tests
- `bin_unix/`, `bin64/`, and `sauerbraten.app/` contain OS-specific runtime dependencies
- `cowbraten.bat` and `cowbraten.sh` are launch scripts for Windows/Linux
- `server.sh` and `start.sh` are custom launch scripts that use content from a separate Sauerbraten installation

## Building your own binary

*You don't have to do this if you already followed the installation instructions above and just want to play!*

### Build dependencies

- development headers for the usual Sauerbraten build dependencies, including zlib, FreeType, SDL2, SDL2_image, and SDL2_mixer
- CMake (used to configure and build the vendored RmlUi dependency)
- libarchive-zip-perl / perl-Archive-Zip
- development headers of libcurl (when checkupdate.patch is included and you're building targets other than linux64)
- Zig for cross-compiling Windows or macOS targets; the recommended version is specified in `.zig-version`.
- NSIS for creating the Windows installer

### Building

On Linux or macOS, run `make install` after installing the usual Sauerbraten dependencies. Native compilation on Windows is not supported, but the Windows target can be cross-compiled from Linux with `make windows`.

This will put the resulting binary into the usual place inside this repo. To use it, you have to copy it over to the same place in your actual Sauerbraten installation. On Linux, the [start.sh](./start.sh) script will launch the new binary from inside this repository, using the Sauerbraten files in $SAUER_DIR and `~/.cowbraten` as user data directory.

### Using fresh upstream sources

You can build my client using fresh vanilla sources. Set $SAUER_DIR to the path of your Sauerbraten directory, then use `make from-patches` and `make install`:

```
export SAUER_DIR=~/sauerbraten-code
make from-patches
make install
```

A fresh source tree will be copied from the Sauerbraten directory, then all my patches are applied on top of that before buildling and installing a new binary.

### Debugging

On Linux, I replace `-O3` with `-g -rdynamic` in the [Makefile](./src/Makefile), and `exec` with `exec gdb --args` in [start.sh](./start.sh) to set up debugging. Then:

```
make clean && make && make install
./start.sh
```

In gdb, typing `run` starts cowbraten.
