-- MQ2CF Cleric Module
-- All healing decision logic for the cleric class
--
-- This module is loaded automatically when the local player is a cleric.
-- It registers callbacks with the C++ core for OnPulse, OnZoned, etc.
-- All eqlib access goes through Core.* API calls -- no direct memory access.

local Cleric = {}

-- Track whether we have printed the init message
local initPrinted = false

-- Settings defaults (loaded from INI/SQLite, these are fallbacks)
Cleric.Settings = {
    -- Heal thresholds
    MAQuickHeal = 99,
    NonMAQuickHeal = 75,
    MADissident = 50,
    NonMADissident = 50,
    MADurationHeal = 75,
    NonMADurationHeal = 75,
    MAPromise = 98,
    NonMAPromise = 85,
    MAIntervention = 99,
    NonMAIntervention = 85,
    MAHealsHeals = 60,
    NonMAHealsHeals = 60,
    HealNPC = 60,

    -- Feature toggles
    UseRez = true,
    UseRezOnRaid = true,
    UseDivineRez = true,
    UseEpicRez = true,
    UseYaulp = true,
    UseWardAA = true,
    UseXTargetHealing = false,
    UseSplash = true,
    UseCures = true,
    BattleMode = false,
    KeepHoTUp = false,
}

function Cleric.OnPulse()
    -- Print init message once on first pulse
    if not initPrinted then
        local gameState = Core.Util.GetGameState()
        local name = Core.Spawn.GetLocalName()
        Core.Util.WriteChatf("\ag[MQ2CF]\ax Cleric module loaded (player: " .. name .. ", state: " .. tostring(gameState) .. ")")
        initPrinted = true
    end

    -- TODO Phase 3: Implement heal triage
    -- 1. Check if we should heal (not paused, not casting, in combat)
    -- 2. Get heal target via Core.Group.GetHealTarget()
    -- 3. Select appropriate heal spell based on target HP% and thresholds
    -- 4. Validate and cast via Core.Cast.Cast()
end

function Cleric.OnZoned()
    -- Reset state on zone change
    initPrinted = false
end

return Cleric
