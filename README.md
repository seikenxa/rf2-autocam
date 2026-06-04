# rf2-autocam

Automatic camera control plugin for rFactor 2 and Le Mans Ultimate.

Enables automated broadcast-style camera switching for live race streams — no human operator required, ideal for endurance races or unattended streams.

## Features

- **Practice / Qualifying**: Follows the car currently on a fast lap (best sector detection)
- **Race**: Tracks the closest on-track battle; higher positions get priority
- **Side-by-side detection**: Switches to a wider/trackside view when cars run side by side
- **Pit stop coverage**: Optionally cuts to pit lane action when no close gaps exist
- **Incident replay**: Detects collisions and triggers instant replay automatically
- **Rear view / onboard cameras**: Randomized camera type switching for variety
- **Formation lap / post-race**: Walk-through mode to cycle through cars
- **OBS integration**: Writes current driver name, session time, and lap info to text/HTML files for use as OBS sources

## Installation

The same `rf2autocam_x64.dll` works for both rFactor 2 and Le Mans Ultimate.  
The plugin detects which game it is running in automatically — no configuration needed.

### rFactor 2

1. Copy `rf2autocam_x64.dll` to `<rFactor2>\Bin64\Plugins\`
2. Launch rFactor 2 — the plugin will create `rF2autocam.ini` in `UserData\player\` on first run
3. Edit the ini file to adjust behavior (see [Configuration](#configuration))

Recommended: set instant replay length to 180 seconds in rF2 settings.

### Le Mans Ultimate

1. Copy `rf2autocam_x64.dll` to `<LeMansUltimate>\Plugins\` — create the folder if it does not exist
2. Add the following entry to `<LeMansUltimate>\UserData\player\CustomPluginVariables.JSON`:
   ```json
   "rf2autocam_x64.dll": {
     " Enabled": 1
   }
   ```
   Set `" Enabled": 0` to disable the plugin without removing the DLL.
3. Launch a session — the plugin will create `rF2autocam.ini` in `UserData\player\` on first run

> **Note:** LMU camera control uses the game's built-in REST API (`localhost:6397`).  
> The plugin switches cameras automatically via `PUT /rest/watch/focus/{slotId}`.  
> Instant replay is not supported in LMU (the replay API works differently from rF2).

## Configuration

The ini file is created at `UserData\player\rF2autocam.ini` on first launch.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `auto` | `1` | Auto camera on/off at startup (0=off, 1=on) |
| `autokey` | `0x41` | Virtual-key code for the toggle hotkey (Ctrl + key). Default: Ctrl+`A`. See [virtual-key codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) |
| `waitsec` | `15` | Minimum seconds between camera changes (actual time adds up to +5s randomly) |
| `interest` | `12` | Positions 1–N receive priority weighting |
| `interestdiff` | `3` | Gap threshold (seconds) for "interesting" battle; beyond this, switches to random mode |
| `onboarddiff` | `0.4` | Gap threshold (seconds) for onboard camera trigger |
| `onboardcam` | `0` | Onboard camera type (0=TV cockpit, 1=cockpit, 2=nosecam, 3=swingman) |
| `rearview` | `0` | Percentage chance (0–100) to use rear-view instead of onboard |
| `rearviewcam` | `6` | Rear-view camera type |
| `walkthrough` | `1` | Walk-through mode on formation lap and after finish (0=off, 1=on) |
| `showinpit` | `interestdiff` | When to show pit stops: `never`, `always`, `interestdiff`, `onboarddiff`, or a position number |
| `lowinc` | `500` | Incident severity threshold for practice/qualifying. The severity is the contact magnitude from the results stream — roughly 500 = light contact, 2000+ = heavy, 3000+ = big crash |
| `highinc` | `2000` | Incident severity threshold for races (and the bar in all sessions). Lower it to catch smaller race incidents |
| `livecut` | `0` | Manual incident replay mode (0=off → auto replay as before; 1=on → live-focus the incident car and let the operator replay it manually with the Instant Replay key). See [Live Cut](#live-cut-manual-incident-replay) |
| `replaykey` | `auto` | Key polled for manual replay in `livecut` mode. `auto` reads the sim's own Instant Replay binding (rF2 `Controller.JSON` / LMU `keyboard.json`); set a hex virtual-key code (e.g. `0x52` for R) to override |
| `incidenthold` | `7` | Seconds to hold the live focus on the incident car, and how long the `incident` signal stays true. **For LMU `livecut`, set this to ~20** so the focus stays on the incident car long enough to watch a native replay; `7` is fine for rF2 (where the plugin drives the replay) |
| `camtest` | `no` | Camera test mode: `no`, `ob` (force onboard), `rv` (force rearview) |
| `sbsdist` | `1.5` | Side-by-side detection distance in meters (cars within this distance trigger SBS camera) |
| `sbscount` | `2` | Minimum number of cars at the same track position to trigger SBS camera |
| `replayduration` | `20` | How long (seconds) instant replay plays before returning to live |
| `replayoffset` | `5.0` | How many seconds before the incident to start the replay |
| `debug` | `0` | Write debug log to `filespath\debug.log` (0=off, 1=on) |

### Camera type numbers

| Value | Camera |
|-------|--------|
| 0 | TV cockpit |
| 1 | Cockpit |
| 2 | Nosecam |
| 3 | Swingman |
| 4 | Trackside (nearest) |
| 5–1004 | Onboard 000–999 |

## Live Cut (manual incident replay)

By default (`livecut=0`) the plugin reacts to a big incident by automatically
triggering an instant replay (rF2 only — LMU has no plugin-controllable replay).

With `livecut=1` the plugin instead **cuts the live camera to the incident car**
and hands replay control to you:

1. An incident is detected → the camera holds on the incident car for
   `incidenthold` seconds and an on-screen message appears (`Incident - press R for replay`).
2. Press your **Instant Replay key** (R by default — the plugin reads your actual
   binding, see `replaykey`) to replay the incident.
   - **rF2:** the plugin drives the replay — it seeks to the incident, and a second
     press of the key returns to autocam.
   - **Le Mans Ultimate:** the plugin only holds the live focus; you use LMU's own
     native instant replay. Press R to replay (the camera is already on the incident
     car, so the replay shows it) and click the **LIVE** marker at the right end of the
     replay seek bar to return to the live session. The plugin does not
     track or control LMU's replay, so set `incidenthold` long enough to cover watching
     it (the camera stays locked on the incident car for that whole window).

`livecut` is recommended for a human director who wants to decide when to replay,
rather than having replays fire automatically.

### Two incident signals (for OBS)

| Signal | Fires on | Use for |
|--------|----------|---------|
| `incident` (→ `incident_start`/`incident_end`) | rF2 **and** LMU, on every detected incident | Banners / alerts — sim-independent |
| `on_replay` (→ `replay_start`/`replay_end`) | rF2 only, while a replay actually plays | Replay scene switches |

On rF2, `incident_start` fires a few seconds **before** `replay_start`; if you wire
both to a scene switch you will get two cuts — pick one.

## OBS / Streaming Integration

The plugin writes files to the path set in `filespath` (default: `<rF2 install>\UserData\player\rF2stream\`).

| File | Content |
|------|---------|
| `driver.txt` | Current driver name and position |
| `time.txt` | Remaining time or lap count |
| `info.html` | Auto-refreshing HTML with sector times |
| `status.json` | Structured data for custom overlays — see below |

Add these as "Text (GDI+)" or "Browser" sources in OBS.

### status.json fields

Updated every ~0.5 seconds. Example:

```json
{
  "driver":       "Verstappen",
  "position":     3,
  "camera":       "trackside",
  "on_replay":    false,
  "autocam":      true,
  "session_type": "race",
  "game_phase":   "green",
  "time_display": "01:23:45",
  "gap_to_next":  0.312,
  "in_battle":    true,
  "sbs_active":   false,
  "leader":       "Hamilton"
}
```

| Field | Type | Description |
|-------|------|-------------|
| `driver` | string | Driver name currently on camera. During replay: name of the incident driver |
| `position` | number | Race/quali position of the current driver. `0` during replay |
| `camera` | string | Active camera type: `tvcockpit`, `cockpit`, `nosecam`, `swingman`, `rearview`, `onboard`, `trackside` |
| `on_replay` | boolean | `true` while an instant replay is playing |
| `incident` | boolean | `true` for `incidenthold` seconds after an incident is detected (both rF2 and LMU, independent of replay mode) — drives a banner/alert in OBS |
| `autocam` | boolean | `true` = auto camera is active; `false` = manual / autocam toggled off |
| `session_type` | string | `practice`, `qualifying`, or `race` |
| `game_phase` | string | Race phase: `garage`, `warmup`, `formation`, `green`, `yellow` (safety car/FCY), `stopped` (red flag), `finished` |
| `time_display` | string | Remaining time (`HH:MM:SS`), lap count (`n / N`), `"Last lap"`, `"Race finished"`, `"REPLAY"`, or `""` |
| `gap_to_next` | number | Tightest gap on track (seconds between any two cars). Reflects the closest active battle, not necessarily the car currently on camera |
| `in_battle` | boolean | `true` when there is a close battle on track (gap within `onboarddiff` threshold) |
| `sbs_active` | boolean | `true` when two or more cars are running side-by-side (within `sbsdist` meters at the same track position) |
| `leader` | string | Driver name of the current race/session leader (P1) |

## OBS Lua Script

`obs-script/autocam_obs.lua` is an OBS Lua script that reads `status.json`
and fires configurable actions on race events — no additional software required.

### Setup

1. In OBS: **Tools → Scripts → +** → select `autocam_obs.lua`
2. In the script settings, set **status.json path**:
   - Same-PC: `C:\...\rF2stream\status.json`
   - Two-PC (network share): `\\game-pc\rF2stream\status.json`
3. Configure actions for each trigger (all fields are optional — leave blank to skip)

### Triggers and actions

Each trigger exposes five settings:

| Setting | Description |
|---------|-------------|
| **Switch scene** | Dropdown — OBS scene to activate (leave blank for no switch) |
| **Show sources** | Comma-separated source names to make visible |
| **Hide sources** | Comma-separated source names to hide |
| **Trigger media source** | Media Source name to restart/play from the beginning |
| **Auto-restore after (sec)** | Seconds until show↔hide is reversed (0 = no auto-restore) |

Available triggers:

| Trigger | Fires when… |
|---------|------------|
| **Replay started** | `on_replay` becomes `true` |
| **Replay ended** | `on_replay` becomes `false` |
| **Incident detected** | `incident` becomes `true` (fires on rF2 + LMU) |
| **Incident cleared** | `incident` becomes `false` |
| **Battle started** | `in_battle` becomes `true` |
| **Battle ended** | `in_battle` becomes `false` |
| **Side-by-side started** | `sbs_active` becomes `true` |
| **Side-by-side ended** | `sbs_active` becomes `false` |
| **Leader changed** | `leader` field changes to a different driver |
| **Formation lap** | `game_phase` becomes `"formation"` |
| **Green flag** | `game_phase` becomes `"green"` |
| **Safety car / FCY** | `game_phase` becomes `"yellow"` |
| **Red flag / stopped** | `game_phase` becomes `"stopped"` |
| **Race finished** | `game_phase` becomes `"finished"` |

### Example: replay scene switch with auto-return

**Replay started**
- Switch scene: `Replay`
- Show sources: `Replay Overlay`

**Replay ended**
- Switch scene: `Live`
- Hide sources: `Replay Overlay`

### Example: leader-change banner (auto-hide after 5 seconds)

**Leader changed**
- Show sources: `Leader Change Banner`
- Auto-restore after: `5`

### Two-PC setup

Share the `rF2stream` folder on the game PC (standard Windows file sharing),
then point the script at the UNC path from the OBS PC. No extra software needed.

## Building from Source

**Requirements:**
- Visual Studio 2022 or later (Desktop development with C++ workload)
- Windows SDK

**Steps:**
1. Open `VC11\autocam.sln` in Visual Studio 2022
2. Select configuration: `Release` / `x64`
3. Build → the DLL will appear in `x64\Release\`

## Compatibility

| Game | Status | Notes |
|------|--------|-------|
| rFactor 2 | ✅ Supported | Full feature set |
| Le Mans Ultimate | ✅ Supported | Camera switching via REST API. Instant replay not supported |
| Saved replay files | ⛔ Not supported | Plugin callbacks are not active during replay playback. Live sessions only. See [docs/replay-not-supported.md](docs/replay-not-supported.md) |

### LMU technical notes

LMU does not call the `WantsToViewVehicle` plugin callback that rF2 uses for camera control.  
Instead, this plugin uses LMU's built-in REST API to switch cameras:

- **Vehicle focus**: `PUT http://localhost:6397/rest/watch/focus/{slotId}`
- **Camera type**: `POST http://localhost:6397/rest/replay/CameraController/setCamera`

The REST API is only accessible from the same machine (loopback only).  
The plugin calls it from a background thread to avoid any impact on frame timing.

For a full technical overview of how the plugin works internally, see [docs/plugin-architecture.md](docs/plugin-architecture.md).

## Credits

Originally developed by **h0rcs4** — https://github.com/h0rcs4/rF2AutoCamPlugin

This fork continues development after the original project went dormant in 2015.

## License

MIT License — see [LICENSE](LICENSE) for details.

Copyright (c) 2026 h0rcs4  
Copyright (c) 2026 seikenxa
