--[[
  autocam_obs.lua
  OBS Lua script — AutoCam (rFactor 2 / Le Mans Ultimate) integration

  Reads status.json written by the rf2-autocam plugin and fires configurable
  OBS actions (scene switch, source show/hide, media playback) whenever race
  events change: replay start/end, close battle, side-by-side, leader change,
  game phase transitions (formation, safety car, green flag, finished, …).

  Each trigger can independently:
    • Switch to a named OBS scene
    • Show one or more sources (comma-separated)
    • Hide one or more sources (comma-separated)
    • Restart / play a Media Source
    • Auto-restore show↔hide after N seconds (e.g. pop a banner then hide it)

  For two-PC streaming setups: point json_path to the shared network folder,
  e.g.  \\game-pc\rF2stream\status.json

  https://github.com/seikenxa/rf2-autocam
--]]

obs = obslua

-- ── Trigger registry ─────────────────────────────────────────────────────────
-- Order here determines display order in the settings panel.

local TRIGGER_KEYS = {
    "replay_start",
    "replay_end",
    "battle_start",
    "battle_end",
    "sbs_start",
    "sbs_end",
    "leader_change",
    "phase_formation",
    "phase_green",
    "phase_yellow",
    "phase_stopped",
    "phase_finished",
}

local TRIGGER_LABELS = {
    replay_start    = "Replay started  (on_replay: false → true)",
    replay_end      = "Replay ended    (on_replay: true → false)",
    battle_start    = "Battle started  (in_battle: false → true)",
    battle_end      = "Battle ended    (in_battle: true → false)",
    sbs_start       = "Side-by-side started  (sbs_active: false → true)",
    sbs_end         = "Side-by-side ended    (sbs_active: true → false)",
    leader_change   = "Leader changed  (leader field changes)",
    phase_formation = "Formation lap  (game_phase: formation)",
    phase_green     = "Green flag     (game_phase: green)",
    phase_yellow    = "Safety car / FCY  (game_phase: yellow)",
    phase_stopped   = "Red flag / stopped  (game_phase: stopped)",
    phase_finished  = "Race finished  (game_phase: finished)",
}

-- ── Runtime state ────────────────────────────────────────────────────────────

local cfg = { poll_ms = 500 }     -- settings, populated by script_update()
local prev = {}                    -- last-seen status.json values
local initialized   = false        -- true after first successful poll
local restore_timers = {}          -- key → one-shot restore timer function

-- ── Utility helpers ──────────────────────────────────────────────────────────

local function log(msg)
    obs.script_log(obs.LOG_INFO, "[AutoCam] " .. tostring(msg))
end

local function warn(msg)
    obs.script_log(obs.LOG_WARNING, "[AutoCam] " .. tostring(msg))
end

local function trim(s)
    return (s or ""):match("^%s*(.-)%s*$")
end

-- Split "foo, bar , baz" → {"foo", "bar", "baz"}
local function split_csv(s)
    local t = {}
    for part in (s or ""):gmatch("[^,]+") do
        local name = trim(part)
        if name ~= "" then t[#t + 1] = name end
    end
    return t
end

-- Read a file and parse it as JSON via OBS data API.
-- Returns an obs_data handle (caller must obs_data_release it), or nil.
local function read_json(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local s = f:read("*a")
    f:close()
    if not s or trim(s) == "" then return nil end
    return obs.obs_data_create_from_json(s)
end

-- ── OBS action helpers ───────────────────────────────────────────────────────

local function switch_scene(scene_name)
    if not scene_name or scene_name == "" then return end
    local scenes = obs.obs_frontend_get_scenes()
    if not scenes then return end
    for _, scene in ipairs(scenes) do
        if obs.obs_source_get_name(scene) == scene_name then
            obs.obs_frontend_set_current_scene(scene)
            log("Scene → " .. scene_name)
            break
        end
    end
    obs.source_list_release(scenes)
end

-- Set visibility of a named source in every scene that contains it.
-- This covers multi-scene setups where the same source appears in several scenes.
local function set_source_visible(source_name, visible)
    local scenes = obs.obs_frontend_get_scenes()
    if not scenes then return end
    for _, scene_src in ipairs(scenes) do
        local scene = obs.obs_scene_from_source(scene_src)
        local item  = obs.obs_scene_find_source(scene, source_name)
        if item then
            obs.obs_sceneitem_set_visible(item, visible)
        end
    end
    obs.source_list_release(scenes)
end

-- Apply visibility to every name in a comma-separated string.
local function apply_source_list(names_str, visible)
    for _, name in ipairs(split_csv(names_str)) do
        set_source_visible(name, visible)
    end
end

-- Restart (play from the beginning) a Media Source by name.
local function trigger_media(source_name)
    if not source_name or source_name == "" then return end
    local src = obs.obs_get_source_by_name(source_name)
    if src then
        obs.obs_source_media_restart(src)
        obs.obs_source_release(src)
        log("Media → " .. source_name)
    else
        warn("Media source not found: " .. source_name)
    end
end

-- Cancel a pending auto-restore timer for `key`.
local function cancel_restore(key)
    local fn = restore_timers[key]
    if fn then
        obs.timer_remove(fn)
        restore_timers[key] = nil
    end
end

-- ── Trigger execution ────────────────────────────────────────────────────────

local function fire_trigger(key)
    local scene     = cfg[key .. "_scene"]       or ""
    local show_str  = cfg[key .. "_show"]         or ""
    local hide_str  = cfg[key .. "_hide"]         or ""
    local media     = cfg[key .. "_media"]        or ""
    local restore_s = cfg[key .. "_restore_sec"]  or 0

    -- Any active auto-restore for this key is superseded by the new event.
    cancel_restore(key)

    switch_scene(scene)
    apply_source_list(show_str, true)
    apply_source_list(hide_str, false)
    trigger_media(media)

    log("Trigger: " .. key)

    -- Schedule auto-restore: swap show↔hide after restore_s seconds.
    if restore_s > 0 and (show_str ~= "" or hide_str ~= "") then
        local fn
        fn = function()
            obs.timer_remove(fn)
            restore_timers[key] = nil
            apply_source_list(show_str, false)
            apply_source_list(hide_str, true)
            log("Restore: " .. key)
        end
        restore_timers[key] = fn
        obs.timer_add(fn, restore_s * 1000)
    end
end

-- ── Poll loop ────────────────────────────────────────────────────────────────

local function poll()
    local path = cfg.json_path
    if not path or path == "" then return end

    local data = read_json(path)
    if not data then return end

    local on_replay  = obs.obs_data_get_bool(data,   "on_replay")
    local in_battle  = obs.obs_data_get_bool(data,   "in_battle")
    local sbs_active = obs.obs_data_get_bool(data,   "sbs_active")
    local leader     = obs.obs_data_get_string(data, "leader")     or ""
    local game_phase = obs.obs_data_get_string(data, "game_phase") or ""

    obs.obs_data_release(data)

    -- First successful poll: establish baseline without firing any triggers.
    if not initialized then
        prev.on_replay  = on_replay
        prev.in_battle  = in_battle
        prev.sbs_active = sbs_active
        prev.leader     = leader
        prev.game_phase = game_phase
        initialized     = true
        log("Baseline established (leader=" .. leader
            .. " phase=" .. game_phase .. ")")
        return
    end

    -- on_replay ---------------------------------------------------------------
    if on_replay ~= prev.on_replay then
        fire_trigger(on_replay and "replay_start" or "replay_end")
        prev.on_replay = on_replay
    end

    -- in_battle ---------------------------------------------------------------
    if in_battle ~= prev.in_battle then
        fire_trigger(in_battle and "battle_start" or "battle_end")
        prev.in_battle = in_battle
    end

    -- sbs_active --------------------------------------------------------------
    if sbs_active ~= prev.sbs_active then
        fire_trigger(sbs_active and "sbs_start" or "sbs_end")
        prev.sbs_active = sbs_active
    end

    -- leader change -----------------------------------------------------------
    -- Skip when prev.leader is empty (pre-session / not yet populated).
    if leader ~= prev.leader and prev.leader ~= "" then
        fire_trigger("leader_change")
    end
    prev.leader = leader

    -- game_phase --------------------------------------------------------------
    if game_phase ~= prev.game_phase and game_phase ~= "" then
        local trigger_key = "phase_" .. game_phase
        -- Fire only if this phase has any configuration (avoids log noise for
        -- phases the user has not set up, e.g. "garage", "warmup", "unknown").
        local has_cfg = (cfg[trigger_key .. "_scene"] or "") ~= ""
                     or (cfg[trigger_key .. "_show"]  or "") ~= ""
                     or (cfg[trigger_key .. "_hide"]  or "") ~= ""
                     or (cfg[trigger_key .. "_media"] or "") ~= ""
        if has_cfg then
            fire_trigger(trigger_key)
        end
        prev.game_phase = game_phase
    end
end

-- ── OBS script interface ─────────────────────────────────────────────────────

function script_description()
    return [[<h2>AutoCam OBS Integration</h2>
<p>Reads <b>status.json</b> from the rf2-autocam plugin and fires configurable
OBS actions on race events.</p>
<p>Each trigger can: switch a scene, show/hide sources, play a media source,
and optionally auto-restore after a delay.</p>
<p>For two-PC setups use a UNC path:<br>
<code>\\game-pc\rF2stream\status.json</code></p>
<p><a href="https://github.com/seikenxa/rf2-autocam">github.com/seikenxa/rf2-autocam</a></p>]]
end

function script_properties()
    local props = obs.obs_properties_create()

    -- General settings --------------------------------------------------------
    obs.obs_properties_add_text(props, "json_path",
        "status.json path", obs.OBS_TEXT_DEFAULT)

    obs.obs_properties_add_int(props, "poll_ms",
        "Poll interval (ms)", 100, 5000, 100)

    -- Collect scene names once to populate all dropdowns ----------------------
    local scene_names = {}
    local scenes = obs.obs_frontend_get_scenes()
    if scenes then
        for _, s in ipairs(scenes) do
            scene_names[#scene_names + 1] = obs.obs_source_get_name(s)
        end
        obs.source_list_release(scenes)
    end

    -- One collapsible group per trigger ---------------------------------------
    for _, key in ipairs(TRIGGER_KEYS) do
        local label = TRIGGER_LABELS[key] or key
        local grp   = obs.obs_properties_create()

        -- Scene switch dropdown
        local sl = obs.obs_properties_add_list(grp,
            key .. "_scene", "Switch scene",
            obs.OBS_COMBO_TYPE_LIST, obs.OBS_COMBO_FORMAT_STRING)
        obs.obs_property_list_add_string(sl, "(no switch)", "")
        for _, name in ipairs(scene_names) do
            obs.obs_property_list_add_string(sl, name, name)
        end

        -- Source visibility
        obs.obs_properties_add_text(grp, key .. "_show",
            "Show sources  (comma-separated)", obs.OBS_TEXT_DEFAULT)
        obs.obs_properties_add_text(grp, key .. "_hide",
            "Hide sources  (comma-separated)", obs.OBS_TEXT_DEFAULT)

        -- Media trigger
        obs.obs_properties_add_text(grp, key .. "_media",
            "Trigger media source", obs.OBS_TEXT_DEFAULT)

        -- Auto-restore
        obs.obs_properties_add_int(grp, key .. "_restore_sec",
            "Auto-restore after (sec, 0 = off)", 0, 3600, 1)

        obs.obs_properties_add_group(props, key, label,
            obs.OBS_GROUP_NORMAL, grp)
    end

    return props
end

function script_defaults(settings)
    obs.obs_data_set_default_string(settings, "json_path", "")
    obs.obs_data_set_default_int(settings, "poll_ms", 500)
    for _, key in ipairs(TRIGGER_KEYS) do
        obs.obs_data_set_default_string(settings, key .. "_scene", "")
        obs.obs_data_set_default_string(settings, key .. "_show",  "")
        obs.obs_data_set_default_string(settings, key .. "_hide",  "")
        obs.obs_data_set_default_string(settings, key .. "_media", "")
        obs.obs_data_set_default_int(settings,    key .. "_restore_sec", 0)
    end
end

function script_update(settings)
    -- Stop current poll timer before reconfiguring.
    obs.timer_remove(poll)

    -- General settings
    cfg.json_path = obs.obs_data_get_string(settings, "json_path")
    cfg.poll_ms   = math.max(100, obs.obs_data_get_int(settings, "poll_ms"))

    -- Per-trigger settings
    for _, key in ipairs(TRIGGER_KEYS) do
        cfg[key .. "_scene"]       = obs.obs_data_get_string(settings, key .. "_scene")
        cfg[key .. "_show"]        = obs.obs_data_get_string(settings, key .. "_show")
        cfg[key .. "_hide"]        = obs.obs_data_get_string(settings, key .. "_hide")
        cfg[key .. "_media"]       = obs.obs_data_get_string(settings, key .. "_media")
        cfg[key .. "_restore_sec"] = obs.obs_data_get_int(settings,    key .. "_restore_sec")
    end

    -- Reset state: next poll will re-establish baseline without firing triggers.
    initialized = false

    -- Restart poll timer.
    if cfg.json_path ~= "" then
        obs.timer_add(poll, cfg.poll_ms)
        log("Polling " .. cfg.json_path .. " every " .. cfg.poll_ms .. " ms")
    else
        log("json_path not set — polling paused")
    end
end

function script_load(settings)
    script_update(settings)
end

function script_unload()
    obs.timer_remove(poll)
    for key, _ in pairs(restore_timers) do
        cancel_restore(key)
    end
    log("Unloaded")
end
