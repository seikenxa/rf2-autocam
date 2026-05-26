# Plugin Architecture

## Overview

rf2-autocam is a C++ DLL plugin that hooks into the rFactor 2 / Le Mans Ultimate game engine via the **InternalsPlugin API** (V07). It runs in-process with the game and receives callbacks every frame or every scoring update, making real-time camera decisions based on race state.

The same DLL works for both games. At startup, the plugin detects which game it is running in and switches its camera control method accordingly.

---

## InternalsPlugin Callback System

The plugin implements the `InternalsPluginV07` interface. The game calls these methods directly:

| Callback | Frequency | Purpose |
|---|---|---|
| `SetEnvironment` | Once at startup | Detect rF2 vs LMU; read ini settings |
| `StartSession` | Each session start | Reset all per-session state |
| `UpdateScoring` | ~2 Hz | Main decision loop: orchestrates all sub-routines |
| `WantsToViewVehicle` | Every frame | rF2 camera control (return target slot ID) |
| `CheckHWControl` | Every frame | Keyboard toggle (Ctrl+key) |
| `UpdateTelemetry` | Per vehicle per frame | Not used |

---

## Camera Control: rF2 vs LMU

### rF2 — `WantsToViewVehicle`

In rF2, the plugin controls the camera by returning a vehicle slot ID from `WantsToViewVehicle`. The game calls this every rendered frame and moves the camera to whatever slot ID is returned. The plugin maintains `aktveh` (the confirmed active slot) and simply returns it.

```
UpdateScoring (~2 Hz)
  → evaluate gaps, incidents, SBS, session phase
  → set needveh (desired slot ID)

WantsToViewVehicle (every frame)
  → if needveh != aktveh: commit the switch (aktveh = needveh)
  → return aktveh  →  game moves camera to that vehicle
```

> **Note:** `WantsToViewVehicle` is only called when the player is in spectator mode
> or has not yet driven (no throttle/brake input). Once a driver has driven,
> rF2 stops calling it and the plugin cannot control the camera for that driver.

### LMU — REST API

LMU does not call `WantsToViewVehicle`. Camera switching is done via HTTP:

```
PUT http://localhost:6397/rest/watch/focus/{slotId}
```

When running in LMU, every camera switch fires an asynchronous WinHTTP request
from a detached `std::thread`, so the game loop is never blocked.

```
UpdateScoring (~2 Hz)
  → evaluate gaps, incidents, SBS, session phase
  → if needveh != aktveh: aktveh = needveh
      └─ SwitchCameraViaREST(aktveh)
           └─ std::thread::detach → WinHTTP PUT /rest/watch/focus/{slotId}
```

---

## UpdateScoring — Sub-routine Structure

`UpdateScoring` is the main entry point (~2 Hz). Since v1.2.0 it is an
orchestrator that delegates to focused private methods:

```
UpdateScoring()
  ├─ ScanVehicles()           — reset minbehind; find leader, bestlap, allfinished
  ├─ SelectCameraQualifying() — practice/quali: track cars on hot laps        [session < 10]
  ├─ SelectCameraRace()       — race: gap priority, SBS, pit, last lap        [session ≥ 10]
  ├─ DetectIncidents()        — parse mResultsStream diff for new collisions
  ├─ ResolveTargetVehicle()   — needpos → needveh + camera type (onboard/trackside/rearview)
  └─ WriteSessionOutputs()    — build time string; write txt/html/json files
```

### ScanVehicles

Runs every cycle before session-specific logic. Resets `minbehind` and
`pontosminbehind`, then walks all vehicles to find:
- `elso` — P1 driver name (leader)
- `bestlapT` / `best1T` / `best2T` — best lap and sector times for qualifying
- `allfinished` — whether all cars have crossed the finish line

### SelectCameraQualifying  _(session < 10)_

Scans for cars currently on a flying lap. Priority:
1. Car completing fastest S1+S2 combination (delta lap candidate)
2. Car with fastest S1
3. Any car with a sector in progress

### SelectCameraRace  _(session ≥ 10)_

Gap-weighted priority scan across all running cars:

1. **Green flag** — find the car with the smallest `aktbehind` (gap to car ahead,
   weighted to penalise cars outside the `interest` range)
2. **SBS override** — if `sbscount`+ cars are within `sbsdist` metres at the same
   track position, `needpos` is overridden to the SBS cluster
3. **Pit coverage** — if no close battle and a car is in pits, switch to it
   (governed by `showinpit` setting)
4. **Last lap focus** — if a car is on its last lap and not yet finished, hold focus
5. **Random** — 20 % chance of random switch when `pontosminbehind > interestsec`
6. **Formation lap** — walk-through cycle (if `walkthrough = 1`)
7. **Safety car / other** — hold P1

### DetectIncidents

Monitors `mResultsStream` using a diff approach: only newly appended text is
examined each cycle. When `"Incident"` + `"Immovable"` or `"vehicle"` is found,
extracts the incident vehicle ID and severity score. Triggers `needreplay = true`
when severity exceeds `lowinc` (qualifying) or `highinc` (race) thresholds.

### ResolveTargetVehicle

Converts `needpos` (target position) to `needveh` (slot ID). Also decides camera
type: if `pontosminbehind` is within the `[0.05, onboarddiff]` range, randomly
selects onboard or rearview instead of trackside.

### WriteSessionOutputs

Builds the `time_display` string (remaining time, lap count, "Last lap", etc.)
then writes all output files and optionally appends to `debug.log`.

---

## Camera Decision Variables

| Variable | Role |
|---|---|
| `needpos` | Target car position (1 = leader). Set by Select* methods |
| `needveh` | Target slot ID. Resolved from needpos by ResolveTargetVehicle |
| `aktveh` | Currently confirmed camera slot (committed on view switch) |
| `needcam` | Camera type: `kCamTrackside` (4), `obcam`, or `rvcam` |
| `pontosminbehind` | Gap (sec) of the closest battle found in ScanVehicles. Sentinel `obtime+1` = no battle |
| `minbehind` | Weighted closeness score used to rank candidates |
| `camvalttime` | Session time of the last camera switch |
| `camvalthat` | Minimum hold time before next switch (`waitsec` + random 0–5 s) |

---

## State Reset

`ResetSessionState()` is called from both `Startup()` and `StartSession()` to
ensure clean state at game launch and at every new session. It resets ~28 variables
including lap timing, replay flags, incident tracking, and camera state.

---

## OBS / Streaming Output

Every `UpdateScoring` call writes files to the `filespath` directory:

| File | Content |
|---|---|
| `driver.txt` | Driver name and position |
| `time.txt` | Remaining time or lap count |
| `info.html` | Auto-refreshing HTML with sector times |
| `status.json` | Structured JSON — see below |

### status.json fields  _(since v1.3.0)_

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

| Field | Source variable | Notes |
|---|---|---|
| `driver` | `aktname` / `replayname` | Incident driver during replay |
| `position` | `aktpos` | `0` during replay |
| `camera` | `needcam` | `trackside`, `onboard`, `rearview`, etc. |
| `on_replay` | `onreplay` | |
| `autocam` | `automatic` | |
| `session_type` | `info.mSession` | `practice` / `qualifying` / `race` |
| `game_phase` | `info.mGamePhase` | `garage` / `warmup` / `formation` / `green` / `yellow` / `stopped` / `finished` |
| `time_display` | computed | Remaining time, lap count, "Last lap", "REPLAY", etc. |
| `gap_to_next` | `pontosminbehind` | Tightest gap on track (sec). Sentinel `obtime+1` when no battle |
| `in_battle` | `pontosminbehind` vs `obtime` | `true` when `0.05 ≤ pontosminbehind ≤ obtime` |
| `sbs_active` | `maxsbs` vs `sbscount` | `true` when side-by-side cluster detected |
| `leader` | `elso` | P1 driver name |

An OBS Lua script that reacts to these fields is provided in `obs-script/autocam_obs.lua`.

---

## LMU Detection

Detection runs once in `SetEnvironment` (guarded by `lmuDetected`). The plugin
sends a synchronous `GET /rest/watch/sessionInfo` to `localhost:6397` with a
2-second timeout. HTTP 200 → `isLMU = true`. Any other response or connection
failure → `isLMU = false` (rF2 mode).

```
SetEnvironment (once)
  └─ GET http://localhost:6397/rest/watch/sessionInfo  (2 s timeout)
       ├─ HTTP 200 → isLMU = true
       └─ error / timeout → isLMU = false
```

The `isLMU` flag gates all REST API calls. When false, the plugin behaves
identically to the h0rcs4 original (pure InternalsPlugin callbacks).

---

## Threading Model

The main game thread calls all InternalsPlugin callbacks. REST API calls are
always dispatched to detached threads to avoid introducing latency into the render loop:

```cpp
void rF2autocam::SwitchCameraViaREST(int slotId) {
    std::thread([slotId]() {
        // WinHTTP PUT /rest/watch/focus/{slotId}
    }).detach();
}
```

No shared mutable state is accessed from the REST thread beyond the `slotId`
integer passed by value.
