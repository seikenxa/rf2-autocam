# rf2-autocam

Automatic camera control plugin for rFactor 2 (and Le Mans Ultimate — planned).

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

1. Copy `rf2autocam_x64.dll` to `<rFactor2>\Bin64\Plugins\`
2. Launch rFactor 2 — the plugin will create `rF2autocam.ini` in `UserData\player\` on first run
3. Edit the ini file to adjust behavior (see [Configuration](#configuration))

Recommended: set instant replay length to 180 seconds in rF2 settings.

## Configuration

The ini file is created at `UserData\player\rF2autocam.ini` on first launch.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `auto` | `1` | Auto camera on/off at startup (0=off, 1=on) |
| `autokey` | `0x41` | Hotkey to toggle auto mode (Ctrl + key). Default: Ctrl+A |
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

The plugin writes files to the path set in `filespath` (default: `C:\rF2stream\`).

| File | Content |
|------|---------|
| `driver.txt` | Current driver name and position |
| `time.txt` | Remaining time or lap count |
| `info.html` | Auto-refreshing HTML with sector times |
| `status.json` | Structured data for custom overlays (driver, position, camera, gap, session info) |

Add these as "Text (GDI+)" or "Browser" sources in OBS.

## Building from Source

**Requirements:**
- Visual Studio 2022 or later (Desktop development with C++ workload)
- Windows SDK

**Steps:**
1. Open `VC11\autocam.sln` in Visual Studio 2022
2. Select configuration: `Release` / `x64`
3. Build → the DLL will appear in `x64\Release\`

## Compatibility

| Game | Status |
|------|--------|
| rFactor 2 | ✅ Supported |
| Le Mans Ultimate | 🔄 Planned |

## Credits

Originally developed by **h0rcs4** — https://github.com/h0rcs4/rF2AutoCamPlugin

This fork continues development after the original project went dormant in 2015.

## License

The original project has no explicit license. Please refer to the original repository.
