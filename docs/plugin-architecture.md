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
| `StartSession` | Each session start | Reset all state |
| `UpdateScoring` | ~2 Hz | Main decision loop: scoring, gaps, incidents |
| `WantsToViewVehicle` | Every frame | rF2 camera control (return target slot ID) |
| `CheckHWControl` | Every frame | Keyboard toggle (Ctrl+key) |
| `UpdateTelemetry` | Per vehicle per frame | Not used in current version |

---

## Camera Control: rF2 vs LMU

### rF2 — `WantsToViewVehicle`

In rF2, the plugin controls the camera by returning a vehicle slot ID from `WantsToViewVehicle`. The game calls this every rendered frame and moves the camera to whatever slot ID is returned. The plugin maintains an internal `aveh` variable (the current target slot) and simply returns it.

```
UpdateScoring (2 Hz)
  → evaluate gaps, incidents, SBS, session phase
  → update aveh (target slot ID)

WantsToViewVehicle (every frame)
  → return aveh
  → game moves camera to that vehicle
```

### LMU — REST API

LMU does not call `WantsToViewVehicle`. Camera switching is done via HTTP:

```
PUT http://localhost:6397/rest/watch/focus/{slotId}
```

The plugin detects LMU at startup by attempting a connection to `localhost:6397`. When running in LMU, every camera switch that would normally call `WantsToViewVehicle` instead fires an asynchronous WinHTTP request from a detached `std::thread`, so the game loop is never blocked.

```
UpdateScoring (2 Hz)
  → evaluate gaps, incidents, SBS, session phase
  → if target changed: SwitchCameraViaREST(newSlotId)
      └─ std::thread::detach → WinHTTP PUT /rest/watch/focus/{slotId}
```

Camera type can also be changed independently:

```
POST http://localhost:6397/rest/replay/CameraController/setCamera
```

---

## Camera Decision Logic

All decisions are made in `UpdateScoring`, which receives a full `ScoringInfoV01` snapshot including positions, gaps, lap distances, and the results stream.

### Session phases

| Phase | Behavior |
|---|---|
| Practice / Qualifying | Follow the car currently on a hot lap (best sector detection) |
| Formation lap | Walk-through: cycle through all cars |
| Race | Gap-based priority (see below) |
| Post-race | Walk-through |

### Race mode priority

1. **Incident replay** — if a new collision is detected in `mResultsStream`, trigger an instant replay on the incident vehicle. After `replayduration` seconds, return to live.
2. **Side-by-side (SBS)** — if two or more cars are within `sbsdist` meters at the same track position, switch to a wider/trackside camera.
3. **Close battle** — if the gap between adjacent cars in `interest` positions is within `interestdiff` seconds, follow the battle pair.
4. **Pit stop coverage** — configurable via `showinpit`.
5. **Random** — weighted random selection among top-`interest` cars.

### Incident detection

Incidents are detected by monitoring `mResultsStream` for the keyword `"Incident"`. To prevent re-firing on the same event across multiple `UpdateScoring` calls, the plugin tracks the last-seen buffer length (`prevResultsStream`) and only processes newly appended text each cycle.

---

## OBS / Streaming Output

Every `UpdateScoring` call writes files to the `filespath` directory:

| File | Content |
|---|---|
| `driver.txt` | Driver name and position |
| `time.txt` | Remaining time or lap count |
| `info.html` | Auto-refreshing HTML with sector times |
| `status.json` | Structured JSON for custom overlays |

`status.json` is the recommended integration point for browser-based overlays.

---

## LMU Detection

```cpp
// SetEnvironment — called once at game startup
void rF2autocam::SetEnvironment(const EnvironmentInfoV01 &info) {
    // Try connecting to LMU REST API on localhost:6397
    // If it responds, set isLMU = true
}
```

The `isLMU` flag gates all REST API calls. When false, the plugin behaves identically to the h0rcs4 original (pure InternalsPlugin). When true, `WantsToViewVehicle` is ignored and all camera changes go through WinHTTP.

---

## Threading Model

The main game thread calls all InternalsPlugin callbacks. REST API calls are always dispatched to detached threads to avoid introducing latency into the render loop:

```cpp
void rF2autocam::SwitchCameraViaREST(int slotId) {
    std::thread([slotId]() {
        // WinHTTP PUT /rest/watch/focus/{slotId}
    }).detach();
}
```

No shared mutable state is accessed from these threads beyond the slot ID integer passed by value.
