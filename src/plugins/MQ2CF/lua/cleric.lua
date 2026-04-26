-- MQ2CF Cleric Module
-- All healing decision logic for the cleric class
--
-- This module is loaded automatically when the local player is a cleric.
-- C++ host calls Cleric.OnPulse / Cleric.OnZoned directly via the Cleric global.
-- All eqlib access goes through Core.* API calls -- no direct memory access.

-- Core table is registered as a global by the C++ host (no require needed)
local logPath = (Core and Core.Util and Core.Util.GetResourcePath and Core.Util.GetResourcePath() or ".") .. "\\MQ2CF\\MQ2CF_errors.log"
local logFile = io.open(logPath, "a")
local function logError(msg)
    if logFile then
        logFile:write(os.date("[%H:%M:%S] ") .. tostring(msg) .. "\n")
        logFile:flush()
    end
    if Core and Core.Util and Core.Util.WriteChatf then
        Core.Util.WriteChatf("[MQ2CF] ERROR: " .. tostring(msg))
    end
end

if not Core then
    local f = io.open(logPath, "a")
    if f then f:write(os.date("[%H:%M:%S] ") .. "FATAL: Core table not found in Lua state\n"); f:close() end
    error("MQ2CF: Core table not registered in Lua state")
end

local Cleric = {}

-- ---------------------------------------------------------------------------
-- Mode System (universal 0-8, cleric uses 0-3 only)
-- ---------------------------------------------------------------------------
local MODE = {
    MANUAL_NOCAMP  = 0,  -- No automation, no camp
    ASSIST_CAMP    = 1,  -- Heal group, DPS on MA, camp enforced
    ASSIST_CHASE   = 2,  -- Follow MA + assist
    ASSIST_NOCAMP  = 3,  -- Assist without camp leash
    TANK_CAMP      = 4,  -- Tank-focused (not available for cleric)
    PULLER_TANK    = 5,  -- Pre-pull + tank (not available for cleric)
    PULLER_ASSIST  = 6,  -- Pre-pull + group (not available for cleric)
    MANUAL_TANK    = 7,  -- Manual tank (not available for cleric)
    ROAMING_TANK   = 8,  -- Roaming tank (not available for cleric)
}

local MODE_NAMES = {
    [0] = "Manual/NoCamp", [1] = "Assist/Camp", [2] = "Assist/Chase",
    [3] = "Assist/NoCamp", [4] = "Tank/Camp", [5] = "Puller Tank",
    [6] = "Puller Assist", [7] = "Manual Tank", [8] = "Roaming Tank",
}

Cleric.mode = MODE.ASSIST_CAMP  -- default

local function IsCampMode()
    local m = Cleric.mode
    return m == MODE.ASSIST_CAMP
end

local function IsDPSMode()
    local m = Cleric.mode
    return m == MODE.ASSIST_CAMP or m == MODE.ASSIST_CHASE or m == MODE.ASSIST_NOCAMP
end

-- ---------------------------------------------------------------------------
-- Spell Group IDs (mapped from scan_results_Madue.log)
-- These are the EQ spell_group column values, stable across level ranges
-- ---------------------------------------------------------------------------
local SG = {
    REMEDY          = 4301,  -- Earnest Remedy, Devout Remedy, Sacred Remedy
    INTERVENTION    = 4348,  -- Celestial Intervention, Holy Intervention
    CONTRAVENTION   = 4350,  -- Celestial Contravention (big DI line)
    HEALS           = 4311,  -- Earnest Light, Solemn Light (main heal line)
    PROMISE         = 4314,  -- Promised Resurgence, Promised Restoration
    WORD            = 4342,  -- Word of Resurgence, Word of Vivacity (group HoT)
    SPLASH          = 4356,  -- Healing Splash
    RENEWAL         = 4340,  -- Frenzied Renewal, Frantic Renewal (fast heal)
    YAULP           = 4320,  -- Yaulp XI, Yaulp X
    SYMBOL          = 4302,  -- Symbol of Ealdun
    WARD            = 4303,  -- Ward of the Earnest (single AC buff)
    HAMMER          = 4331,  -- Devout Hammer of Zeal
    MARK_DS         = 4317,  -- Mark of the Devout (damage shield)
    BLOOD_DS        = 4333,  -- Blood of the Devout
    RALLIED         = 4330,  -- Rallied Rampart of Vie
    SHINING         = 4355,  -- Shining Rampart
    RETORT          = 4347,  -- Fintar's Retort
    UNDEAD          = 4351,  -- Annihilate the Undead
    MAGIC_DD        = 4338,  -- Chromarend
    SILENT          = 4325,  -- Silent Proclamation (stun)
    STUN            = 4312,  -- Sound of Fury
    ELIXIR          = 4329,  -- Elixir of the Ardent (cure)
    CENSURE         = 4349,  -- Glorious Censure (consequence line)
    VEIL            = 4326,  -- Armor of the Earnest (HP aura)
    MARK_AURA       = 4327,  -- Ealdun's Mark
}

-- AA ability IDs (from scanned_aas, used with /alt activate)
local AA = {
    DIVINE_REZ      = 36,    -- Divine Resurrection (rank 4/7)
    BLESSING_REZ    = 3800,  -- Blessing of Resurrection (rank 7/7)
    WARD_OF_PURITY  = 506,   -- Ward of Purity (rank 15/25)
    CELESTIAL_HAMMER = 391,  -- Celestial Hammer (rank 21/45)
    BURST_OF_LIFE   = 7689,  -- Burst of Life (rank 6/36)
    CELESTIAL_REGEN = 38,    -- Celestial Regeneration (rank 24/50)
    TURN_UNDEAD     = 558,   -- Turn Undead (rank 22/54)
}

-- Cached spell IDs (reset on zone)
local spellCache = {}
local initPrinted = false
local localSpawnID = 0

-- Settings defaults (from decompiled CWTNCleric.ini loader FUN_18014b830)
Cleric.Settings = {
    -- Heal thresholds (MA = main assist gets priority)
    MAQuickHeal = 99,
    NonMAQuickHeal = 75,
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

-- ---------------------------------------------------------------------------
-- Helpers
-- ---------------------------------------------------------------------------

local function GetSpellID(groupID)
    if not groupID then return 0 end
    if not spellCache[groupID] then
        spellCache[groupID] = Core.Spell.GetHighestLearnedByGroup(groupID) or 0
    end
    return spellCache[groupID]
end

local function GetLocalSpawnID()
    if localSpawnID == 0 then
        localSpawnID = Core.LocalPlayer.GetCharacterBase() or 0
    end
    return localSpawnID
end

local function GetSetting(key)
    return Cleric.Settings[key]
end

-- ---------------------------------------------------------------------------
-- Heal Triage (from FUN_1800a5280 decompilation)
-- Returns spawn_id, hp_pct of target needing healing most
-- ---------------------------------------------------------------------------
local function FindHealTarget()
    local maID = Core.Group.GetMainAssist()
    local groupMembers = Core.Group.GetGroupMembers()

    if Cleric.Settings.UseXTargetHealing then
        local xtargets = Core.Group.GetXTargets()
        for _, xtID in ipairs(xtargets) do
            table.insert(groupMembers, xtID)
        end
    end

    local bestTarget = 0
    local bestHPPct = 100
    local isMABest = false

    for _, spawnID in ipairs(groupMembers) do
        if spawnID == 0 then goto continue end

        -- Skip dead (StandState 0x02 = feigned or dead)
        local standState = Core.Spawn.GetStandState(spawnID)
        if standState == 0x02 then goto continue end

        local hpPct = Core.Spawn.GetHPPercent(spawnID)
        if hpPct >= 100 then goto continue end

        local isMA = (spawnID == maID)
        local isMerc = (Core.Spawn.GetClass(spawnID) == 0x11)

        -- Merc healing cap at 88% (from decompiled merc cap logic)
        if isMerc and hpPct > 88 then
            goto continue
        end

        -- MA bias: non-MA HP treated as 5% lower, making it harder
        -- for non-MA to beat MA in triage priority
        local better = false
        if bestTarget == 0 then
            better = true
        elseif maID ~= 0 then
            if isMA and not isMABest then
                better = (hpPct < bestHPPct - 5)
            elseif not isMA and isMABest then
                better = (hpPct - 5 < bestHPPct)
            else
                better = (hpPct < bestHPPct)
            end
        else
            better = (hpPct < bestHPPct)
        end

        if better then
            bestTarget = spawnID
            bestHPPct = hpPct
            isMABest = isMA
        end

        ::continue::
    end

    return bestTarget, bestHPPct
end

-- ---------------------------------------------------------------------------
-- Heal Spell Selection (from FUN_1800a64e0 decompilation)
-- 6-tier priority: Remedy > Intervention > Heals > Renewal > Promise > HoT
-- Returns spell_id to cast, or 0
-- ---------------------------------------------------------------------------
local function SelectHealSpell(targetID, hpPct, isMA)
    local quickHealThresh = GetSetting(isMA and "MAQuickHeal" or "NonMAQuickHeal")
    local interventionThresh = GetSetting(isMA and "MAIntervention" or "NonMAIntervention")
    local healsThresh = GetSetting(isMA and "MAHealsHeals" or "NonMAHealsHeals")
    local promiseThresh = GetSetting(isMA and "MAPromise" or "NonMAPromise")
    local durationThresh = GetSetting(isMA and "MADurationHeal" or "NonMADurationHeal")

    -- 1. Remedy (fast single target heal)
    if hpPct < quickHealThresh then
        local spellID = GetSpellID(SG.REMEDY)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, targetID)
            if canCast then return spellID end
        end
    end

    -- 2. Intervention (big heal)
    if hpPct < interventionThresh then
        local spellID = GetSpellID(SG.INTERVENTION)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, targetID)
            if canCast then return spellID end
        end
    end

    -- 3. Main heals line (Earnest Light etc)
    if hpPct < healsThresh then
        local spellID = GetSpellID(SG.HEALS)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, targetID)
            if canCast then return spellID end
        end
    end

    -- 4. Frenzied Renewal (fast complete heal variant)
    if hpPct < healsThresh then
        local spellID = GetSpellID(SG.RENEWAL)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, targetID)
            if canCast then return spellID end
        end
    end

    -- 5. Promise heal (only if buff not already on target)
    if hpPct < promiseThresh then
        local spellID = GetSpellID(SG.PROMISE)
        if spellID ~= 0 then
            -- Skip if promise buff already active (local player only for now)
            if not Core.Buff.HasBuffByGroup(targetID, SG.PROMISE) then
                local canCast = Core.Cast.CanCast(spellID, targetID)
                if canCast then return spellID end
            end
        end
    end

    -- 6. HoT (Word of Resurgence) - if KeepHoTUp or HP below threshold
    if Cleric.Settings.KeepHoTUp or hpPct < durationThresh then
        local spellID = GetSpellID(SG.WORD)
        if spellID ~= 0 then
            -- Skip if HoT already active (local player only for now)
            if not Core.Buff.HasBuffByGroup(targetID, SG.WORD) then
                local canCast = Core.Cast.CanCast(spellID, targetID)
                if canCast then return spellID end
            end
        end
    end

    -- 7. Splash heal (AoE, when enabled and target is low)
    if Cleric.Settings.UseSplash and hpPct < 70 then
        local spellID = GetSpellID(SG.SPLASH)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, targetID)
            if canCast then return spellID end
        end
    end

    return 0
end

-- ---------------------------------------------------------------------------
-- Rez Logic (from FUN_180150b00 decompilation)
-- Priority: Divine Rez AA > Blessing of Rez AA > fallback spell
-- ---------------------------------------------------------------------------
local function CheckRez()
    if not Cleric.Settings.UseRez then
        return false
    end

    local groupMembers = Core.Group.GetGroupMembers()

    for _, spawnID in ipairs(groupMembers) do
        if spawnID == 0 then goto continue end

        -- Check if dead (IsDead checks HP <= 0 or STANDSTATE_DEAD)
        if not Core.Spawn.IsDead(spawnID) then goto continue end

        -- Validate target is in range and LOS
        if not Core.Target.IsValidHealTarget(spawnID) then goto continue end

        -- Priority 1: Divine Rez AA
        if Cleric.Settings.UseDivineRez then
            local canActivate, reason = Core.AA.CanActivate(AA.DIVINE_REZ)
            if canActivate then
                Core.AA.Activate(AA.DIVINE_REZ)
                Core.Util.WriteChatf("[MQ2CF] Casting Divine Rez on " .. Core.Spawn.GetName(spawnID))
                return true
            end
        end

        -- Priority 2: Blessing of Resurrection AA
        do
            local canActivate, reason = Core.AA.CanActivate(AA.BLESSING_REZ)
            if canActivate then
                Core.AA.Activate(AA.BLESSING_REZ)
                Core.Util.WriteChatf("[MQ2CF] Casting Blessing of Rez on " .. Core.Spawn.GetName(spawnID))
                return true
            end
        end

        -- Priority 3: Epic 1.0 clicky (TODO: item click support)

        -- Priority 4: Spellbook rez (Reviviscence, Resurrection)
        -- These have spell_group 0 (legacy), so we look up by name
        local rezNames = {"Reviviscence", "Resurrection", "Resuscitate", "Renewal"}
        for _, rezName in ipairs(rezNames) do
            local rezId = Core.Spell.GetNameByString(rezName)
            if rezId ~= 0 then
                local canCast = Core.Cast.CanCast(rezId, spawnID)
                if canCast then
                    Core.Cast.Cast(rezId, spawnID)
                    Core.Util.WriteChatf("[MQ2CF] Casting " .. rezName .. " on " .. Core.Spawn.GetName(spawnID))
                    return true
                end
            end
        end

        ::continue::
    end

    return false
end

-- ---------------------------------------------------------------------------
-- Downtime Logic
-- ---------------------------------------------------------------------------
local function CheckDowntime()
    local localID = GetLocalSpawnID()
    if localID == 0 then return false end

    -- Yaulp self-buff (skip if already active)
    if Cleric.Settings.UseYaulp then
        local spellID = GetSpellID(SG.YAULP)
        if spellID ~= 0 and not Core.Buff.HasBuffByGroup(localID, SG.YAULP) then
            local canCast = Core.Cast.CanCast(spellID, localID)
            if canCast then
                Core.Cast.Cast(spellID, localID)
                return true
            end
        end
    end

    -- Ward of Purity AA
    if Cleric.Settings.UseWardAA then
        local canActivate = Core.AA.CanActivate(AA.WARD_OF_PURITY)
        if canActivate then
            Core.AA.Activate(AA.WARD_OF_PURITY)
            return true
        end
    end

    return false
end

-- ---------------------------------------------------------------------------
-- Cure Logic
-- Cure priority from decompiled FUN_1800fb730:
--   1. Cure Corruption (most dangerous)
--   2. Cure Curse
--   3. Cure Poison
--   4. Cure Disease
-- Uses Core.Buff.GetDebuffCount to detect, elixir line to cure
-- ---------------------------------------------------------------------------
local CURE_SPELL_GROUPS = {
    { group = SG.ELIXIR,  name = "Elixir" },      -- Elixir of the Ardent (cure all)
}

local function CheckCures()
    if not Cleric.Settings.UseCures then
        return false
    end

    local localID = GetLocalSpawnID()
    if localID == 0 then return false end

    -- Check if we have any debuffs
    local debuffCount = Core.Buff.GetDebuffCount()
    if debuffCount == 0 then
        return false
    end

    -- Try elixir (cure-all) first
    for _, cure in ipairs(CURE_SPELL_GROUPS) do
        local spellID = GetSpellID(cure.group)
        if spellID ~= 0 then
            local canCast = Core.Cast.CanCast(spellID, localID)
            if canCast then
                Core.Cast.Cast(spellID, localID)
                Core.Util.WriteChatf("[MQ2CF] Curing with " .. cure.name)
                return true
            end
        end
    end

    return false
end

-- ---------------------------------------------------------------------------
-- Buff Cycle
-- Maintain key self-buffs when idle (no healing/rez/cure needed)
-- ---------------------------------------------------------------------------
local SELF_BUFFS = {
    { group = SG.SYMBOL,    name = "Symbol" },
    { group = SG.VEIL,      name = "HP Aura" },
}

local buffCycleTick = 0
local BUFF_CYCLE_INTERVAL = 50  -- check every ~50 pulses

local function CheckBuffCycle()
    buffCycleTick = buffCycleTick + 1
    if buffCycleTick < BUFF_CYCLE_INTERVAL then
        return false
    end
    buffCycleTick = 0

    local localID = GetLocalSpawnID()
    if localID == 0 then return false end

    -- Self buffs
    for _, selfBuff in ipairs(SELF_BUFFS) do
        if not Core.Buff.HasBuffByGroup(localID, selfBuff.group) then
            local spellID = GetSpellID(selfBuff.group)
            if spellID ~= 0 then
                local canCast = Core.Cast.CanCast(spellID, localID)
                if canCast then
                    Core.Cast.Cast(spellID, localID)
                    return true
                end
            end
        end
    end

    return false
end

-- ---------------------------------------------------------------------------
-- Puller Heal Safety (Task 11)
-- Don't heal puller until their mob is inside camp radius
-- ---------------------------------------------------------------------------
local campRadius = 100  -- configurable

local function IsPullerIncoming(spawnId)
    if not IsCampMode() then return false end
    if not Core.Camp.HasCamp() then return false end
    local targetId = Core.Spawn.GetTarget(spawnId)
    if targetId == 0 then return false end
    -- Puller's target (mob) outside camp radius = puller is incoming
    if Core.Camp.IsSpawnInRadius(targetId, campRadius) then
        return false  -- mob is in camp, safe to heal
    end
    return true  -- mob outside camp, wait
end

-- ---------------------------------------------------------------------------
-- Buff Tracker (Task 12) -- Lua-side cast-and-track for group members
-- ---------------------------------------------------------------------------
local buffTracker = {}  -- buffTracker[spawnId][spellGroupId] = {castTime, duration}

local function RecordBuff(spawnId, spellGroupId, durationSec)
    if not buffTracker[spawnId] then buffTracker[spawnId] = {} end
    buffTracker[spawnId][spellGroupId] = {
        castTime = os.time(),
        duration = durationSec,
    }
end

local function HasTrackedBuff(spawnId, spellGroupId)
    if not buffTracker[spawnId] then return false end
    local entry = buffTracker[spawnId][spellGroupId]
    if not entry then return false end
    return os.time() < entry.castTime + entry.duration
end

-- ---------------------------------------------------------------------------
-- Spell Gem Memorization Loop (Task 9)
-- ---------------------------------------------------------------------------
local DEFAULT_LOADOUT = {
    [0] = SG.REMEDY,
    [1] = SG.INTERVENTION,
    [2] = SG.HEALS,
    [3] = SG.RENEWAL,
    [4] = SG.WORD,
    [5] = SG.PROMISE,
    [6] = SG.SPLASH,
    [7] = SG.YAULP,
    [8] = SG.ELIXIR,
}

local memCooldownUntil = 0
local MEM_COOLDOWN_SEC = 2

local function CheckMemorization()
    if os.time() < memCooldownUntil then return false end
    if not Core.Gem then return false end
    if Core.Gem.NeedsMemorization() == false then return false end

    local slotCount = Core.Gem.GetAvailableSlotCount()
    for slot = 0, slotCount - 1 do
        if Core.Gem.IsReserved(slot) then goto continue end
        local current = Core.Gem.GetCurrentSpell(slot)
        local desiredGroup = DEFAULT_LOADOUT[slot]
        if not desiredGroup then goto continue end
        local desired = GetSpellID(desiredGroup)
        if desired == 0 or desired == current then goto continue end

        local ok, reason = Core.Gem.Memorize(slot, desired)
        if ok then
            memCooldownUntil = os.time() + MEM_COOLDOWN_SEC
            return true
        end
        ::continue::
    end
    return false
end

-- ---------------------------------------------------------------------------
-- Event Registration (Task 10)
-- ---------------------------------------------------------------------------
local eventsRegistered = false

local function RegisterEvents()
    if eventsRegistered then return end
    if not Core.Event then return end

    Core.Event.Register("summon", "has been summoned", function(line)
        Core.Util.WriteChatf("[MQ2CF] Summon detected -- running to camp")
        -- TODO: trigger movement back to camp via nav
    end)

    Core.Event.Register("evac", "begins to cast Evacuate", function(line)
        Core.Camp.ClearCamp()
        Core.Util.WriteChatf("[MQ2CF] Evac detected -- camp cleared, will reset on landing")
    end)

    Core.Event.Register("coth", "Call of the Hero", function(line)
        Core.Camp.ClearCamp()
        Core.Util.WriteChatf("[MQ2CF] CotH detected -- camp will reset")
    end)

    eventsRegistered = true
end

-- ---------------------------------------------------------------------------
-- Main Pulse Handler (from FUN_18009c200 dispatch pattern)
-- ---------------------------------------------------------------------------
function Cleric:OnPulse()
    -- Mode 0 = manual, do nothing
    if self.mode == MODE.MANUAL_NOCAMP then
        return
    end

    if Core.Lifecycle.IsPaused() then
        return
    end

    if Core.LocalPlayer.IsGateFlagSet() then
        return
    end

    local localID = GetLocalSpawnID()
    if localID == 0 then return end

    -- One-time init
    if not initPrinted then
        local name = Core.Spawn.GetName(localID)
        Core.Util.WriteChatf(string.format(
            "[MQ2CF] Cleric module active (player: %s, mode: %s)",
            name, MODE_NAMES[self.mode] or "Unknown"))
        RegisterEvents()
        initPrinted = true
    end

    -- Complete pending cast from previous pulse (two-phase targeting)
    if Core.Cast.HasPendingCast() then
        local ok, reason = Core.Cast.CompletePendingCast()
        if not ok and reason == "waiting" then
            return  -- still waiting for target settle
        end
        -- Cast completed or failed, continue
        return
    end

    -- Skip if already casting
    if Core.Spawn.IsCasting(localID) then
        return
    end

    -- Heal Triage
    local healTarget, hpPct = FindHealTarget()
    if healTarget ~= 0 then
        -- Puller safety: wait for mob in camp before healing
        local emergencyThreshold = 30
        if IsPullerIncoming(healTarget) and hpPct > emergencyThreshold then
            -- Don't heal yet, mob not in camp
        else
            local isMA = (healTarget == Core.Group.GetMainAssist())
            local spellID = SelectHealSpell(healTarget, hpPct, isMA)

            if spellID ~= 0 then
                if Core.Humanize.ShouldSkipTick(0.05) then
                    return
                end

                local success, err = Core.Cast.Cast(spellID, healTarget)
                if not success and err ~= "targeting" then
                    logError("Heal cast failed: " .. tostring(err))
                end
                return
            end
        end
    end

    -- Rez Check
    if CheckRez() then return end

    -- Downtime
    if CheckDowntime() then return end

    -- Cures
    if CheckCures() then return end

    -- Buff Cycle
    if CheckBuffCycle() then return end

    -- DPS (Assist modes only, when healing stable)
    -- TODO: DPS targeting via Core.Spawn.GetTarget(maSpawnID)

    -- Gem Memorization (lowest priority)
    CheckMemorization()
end

-- ---------------------------------------------------------------------------
-- Zone Change Handler
-- ---------------------------------------------------------------------------
function Cleric:OnZoned()
    spellCache = {}
    initPrinted = false
    eventsRegistered = false
    localSpawnID = 0
    buffCycleTick = 0
    buffTracker = {}
    memCooldownUntil = 0
    Core.Camp.ClearCamp()
    Core.Util.WriteChatf("[MQ2CF] Cleric module reset for new zone")
end

-- TODO: Epic 1.0 clicky for rez (item click support)
-- TODO: DPS targeting (Core.Spawn.GetTarget on MA's target)
-- TODO: One-off cast command (/cf cast)
-- TODO: Group buff cycle via buffTracker

return Cleric
