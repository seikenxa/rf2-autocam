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

1. Navigate to `<LeMansUltimate>\Bin\Plugins\` — create the folder if it does not exist
2. Copy `rf2autocam_x64.dll` into it
3. In LMU: **Settings → Gameplay → Enable Plugins: ON**
4. Launch a session — the plugin will create `rF2autocam.ini` in `UserData\player\` on first run

> **Note:** LMU camera control uses the game's built-in REST API (`localhost:6397`).  
> The plugin switches cameras automatically via `PUT /rest/watch/focus/{slotId}`.  
> Instant replay is not supported in LMU (the replay API works differently from rF2).

## Configuration

The ini file is created at `UserData\player\rF2autocam.ini` on first launch.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `auto` | `1` | Auto camera on/off at startup (0=off, 1=on) |
| `autokey` | `0xBE` | Virtual-key code for the toggle hotkey (Ctrl + key). Default: Ctrl+`.`. See [virtual-key codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) |
| `waitsec` | `15` | Minimum seconds between camera changes (actual time adds up to +5s randomly) |
| `interest` | `12` | Positions 1–N receive priority weighting |
| `interestdiff` | `3` | Gap threshold (seconds) for "interesting" battle; beyond this, switches to random mode |
| `onboarddiff` | `0.4` | Gap threshold (seconds) for onboard camera trigger |
| `onboardcam` | `0` | Onboard camera type (0=TV cockpit, 1=cockpit, 2=nosecam, 3=swingman) |
| `rearview` | `0` | Percentage chance (0–100) to use rear-view instead of onboard |
| `rearviewcam` | `6` | Rear-view camera type |
| `walkthrough` | `1` | Walk-through mode on formation lap and after finish (0=off, 1=on) |
| `showinpit` | `interestdiff` | When to show pit stops: `never`, `always`, `interestdiff`, `onboarddiff`, or a position number |
| `lowinc` | `0.4` | Incident severity threshold for replay in practice/qualifying |
| `highinc` | `0.8` | Incident severity threshold for replay in all sessions |
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
  "time_display": "01:23:45",
  "gap_to_next":  0.312
}
```

| Field | Type | Description |
|-------|------|-------------|
| `driver` | string | Driver name currently on camera. During replay: name of the incident driver |
| `position` | number | Race/quali position of the current driver. `0` during replay |
| `camera` | string | Active camera type: `tvcockpit`, `cockpit`, `nosecam`, `swingman`, `rearview`, `onboard`, `trackside` |
| `on_replay` | boolean | `true` while an instant replay is playing |
| `autocam` | boolean | `true` = auto camera is active; `false` = manual / autocam toggled off |
| `session_type` | string | `practice`, `qualifying`, or `race` |
| `time_display` | string | Remaining time (`HH:MM:SS`), lap count (`n / N`), `"Last lap"`, `"Race finished"`, `"REPLAY"`, or `""` |
| `gap_to_next` | number | Gap in seconds to the car ahead of the current subject. `0.0` during replay |

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

### LMU technical notes

LMU does not call the `WantsToViewVehicle` plugin callback that rF2 uses for camera control.  
Instead, this plugin uses LMU's built-in REST API to switch cameras:

- **Vehicle focus**: `PUT http://localhost:6397/rest/watch/focus/{slotId}`
- **Camera type**: `POST http://localhost:6397/rest/replay/CameraController/setCamera`

The REST API is only accessible from the same machine (loopback only).  
The plugin calls it from a background thread to avoid any impact on frame timing.

## Credits

Originally developed by **h0rcs4** — https://github.com/h0rcs4/rF2AutoCamPlugin

This fork continues development after the original project went dormant in 2015.

## License

MIT License — see [LICENSE](LICENSE) for details.

Copyright (c) 2026 h0rcs4  
Copyright (c) 2026 seikenxa
