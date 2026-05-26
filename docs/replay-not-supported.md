# Why Replay Playback Is Not Supported

This document explains the technical reasons why rf2-autocam cannot control cameras during saved replay playback in rFactor 2 or Le Mans Ultimate.

**TL;DR:** Both games stop delivering vehicle state data to plugins during replay. There is no supported mechanism to access real-time vehicle positions or timing data while a replay is playing. This is a game engine constraint, not a plugin limitation.

---

## rFactor 2

### How InternalsPlugin works during a live session

During a live session, the game calls plugin callbacks continuously:

- `UpdateScoring` — ~2 Hz, delivers full `ScoringInfoV01` (positions, gaps, lap distances, results stream)
- `UpdateTelemetry` — per vehicle per frame, delivers `TelemInfoV01` (3D position, velocity, impact magnitude)
- `WantsToViewVehicle` — every frame, the plugin returns the slot ID it wants the camera to follow

### What happens during replay playback

All three callbacks stop when a replay file is loaded and playing.

| Callback | During replay | Notes |
|---|---|---|
| `UpdateScoring` | Called once only, at t=0 | Returns the session state frozen at replay start |
| `UpdateTelemetry` | Not called | Confirmed via debug log: no entries appear |
| `WantsToViewVehicle` | Not called | Camera is not plugin-controlled during replay |

**Verified empirically** via debug log inspection during replay playback (2026-05-26). The debug log, which logs every `UpdateScoring` and `UpdateTelemetry` call, showed no entries after replay started — only the single t=0 `UpdateScoring` from session load.

### Why this is the case

rF2 replays work by re-simulating recorded vehicle states from a replay file. The game does not run the full physics or scoring pipeline during replay — it reads pre-recorded data and renders it. The plugin callback system is part of the live simulation pipeline and is not invoked when that pipeline is inactive.

### What was investigated and ruled out

- **XML result files** (`UserData\Log\Results\*.xml`): These files contain `<Incident et="...">` events and `<Score et="...">` sector timestamps, which could theoretically be used to build a camera timeline. However, since `WantsToViewVehicle` is never called during replay, there is no mechanism to act on that timeline in real time. *(Investigated 2026-05-26)*

- **Telemetry buffer approach**: Accumulating `TelemInfoV01` from `UpdateTelemetry` into an in-memory map for proximity detection — not viable because `UpdateTelemetry` is not called during replay. *(Confirmed 2026-05-26)*

---

## Le Mans Ultimate

### LMU's additional REST API

LMU exposes a local HTTP REST API at `localhost:6397` that can control camera focus and camera type independently of the plugin callback system:

```
PUT  http://localhost:6397/rest/watch/focus/{slotId}   — switch camera to vehicle
POST http://localhost:6397/rest/replay/CameraController/setCamera — change camera type
```

This API is used by rf2-autocam for all camera switching during live sessions (because LMU does not call `WantsToViewVehicle`). It is also used by third-party tools such as **LMU-Replay-GUI**, which suggested that the API might remain active during replay.

### What was tested during LMU replay playback

All tests were performed with a replay actively playing (not paused) on a 32-car ELMS race at Autodromo Enzo e Dino Ferrari (2026-05-26).

**`GET /rest/watch/sessionInfo`**

Responds during replay. However:
- `currentEventTime` is `0.0` and does not change over time — the current playback position cannot be read from this endpoint.
- `gamePhase` is `9` (post-race state), `inRealtime` is `false`.

**`GET /rest/watch/standings`**

Responds and returns data for all 32 vehicles (slot IDs, driver names, 3D positions, gaps). However:
- `carVelocity` is `{velocity: 0.0}` for every vehicle — confirmed while replay was actively playing.
- `carPosition` does not change between polls — vehicle positions are frozen at the replay start state.
- The standings data is static and does not reflect the current playback position.

**`GET /rest/watch/focusInfo`** → `404 Not Found`

**`GET /rest/replay/getState`** → `404 Not Found`

### Conclusion

The LMU REST API remains reachable during replay, but the vehicle state data it returns is frozen. The game engine does not update the standings or session data while replay is playing, for the same underlying reason as rF2: the physics/scoring pipeline that generates this data is not running during replay playback.

LMU-Replay-GUI controls playback position (seek, play, pause) via the REST API — it does not track live vehicle positions. Those are different operations that happen to use the same API server.

---

## Summary

| Feature | rF2 replay | LMU replay |
|---|---|---|
| Plugin callbacks active | ❌ | ❌ |
| REST API reachable | N/A | ✅ |
| Real-time vehicle positions | ❌ | ❌ (frozen) |
| Current playback time | ❌ | ❌ (frozen at 0.0) |
| Camera control possible | ❌ | ❌ |

Replay support would require the game to expose a real-time stream of vehicle state during replay — either through plugin callbacks or through the REST API. Neither game currently does this. This is a game engine design decision and cannot be worked around at the plugin level.

rf2-autocam is designed for **live sessions only**.
