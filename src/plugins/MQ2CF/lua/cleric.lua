-- MQ2CF Cleric Module
-- All healing decision logic for the cleric class
--
-- This module is loaded automatically when the local player is a cleric.
-- It registers callbacks with the C++ core for OnPulse, OnZoned, etc.
-- All eqlib access goes through Core.* API calls -- no direct memory access.

local Core = require("Core")

local Cleric = {}

-- Spell Group IDs (to be filled from spell database)
local SPELL_GROUP_QUICK_HEAL = 1001
local SPELL_GROUP_INTERVENTION = 1002
local SPELL_GROUP_HEALS_HEALS = 1003
local SPELL_GROUP_DISSIDENT = 1004
local SPELL_GROUP_PROMISE = 1005
local SPELL_GROUP_DURATION_HEAL = 1006
local SPELL_GROUP_SPLASH = 1007
local SPELL_GROUP_DIVINE_REZ_AA = 1008
local SPELL_GROUP_BLESSING_REZ = 1009
local SPELL_GROUP_REZ_SPELL = 1010
local SPELL_GROUP_YAULP = 1011
local SPELL_GROUP_WARD_AA = 1012

-- Cached spell IDs (reset on zone)
local spellCache = {}
local initPrinted = false
local localSpawnID = 0

-- Settings defaults (matching decompiled INI loader from FUN_18014b830)
Cleric.Settings = {
    -- Heal thresholds (from CWTNCleric.ini)
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

-- Helper: Get spell ID from group, with caching
local function GetSpellID(groupID)
    if not spellCache[groupID] then
        spellCache[groupID] = Core.Spell.GetHighestLearnedByGroup(groupID)
    end
    return spellCache[groupID]
end

-- Helper: Get local player spawn ID
local function GetLocalSpawnID()
    if localSpawnID == 0 then
        local charBase = Core.LocalPlayer.GetCharacterBase()
        if charBase then
            localSpawnID = Core.Spawn.GetSpawnID(charBase)
        end
    end
    return localSpawnID
end

-- Heal Triage (from FUN_1800a5280 decompilation)
-- Returns spawn_id, hp_pct of target needing healing most
local function FindHealTarget()
    local maID = Core.Group.GetMainAssist()
    local groupMembers = Core.Group.GetGroupMembers()
    -- Add XTargets if enabled
    if Cleric.Settings.UseXTargetHealing then
        local xtargets = Core.Group.GetXTargets()
        for _, xtID in ipairs(xtargets) do
            table.insert(groupMembers, xtID)
        end
    end
    
    -- Evaluate each potential target
    local bestTarget = 0
    local bestHPPct = 100
    local isMABest = false
    
    for _, spawnID in ipairs(groupMembers) do
        if spawnID == 0 then goto continue end
        
        -- Skip dead/feigned (StandState 0x02 from FUN_1800a64e0)
        local standState = Core.Spawn.GetStandState(spawnID)
        if standState == 0x02 then goto continue end
        
        -- Get HP percentage
        local hpPct = Core.Spawn.GetHPPercent(spawnID)
        if hpPct >= 100 then goto continue end
        
        local isMA = (spawnID == maID)
        local isMerc = (Core.Spawn.GetClass(spawnID) == 0x11)  -- Mercenary class ID
        
        -- Merc cap at 88% (from decompiled merc cap logic)
        if isMerc and hpPct > 88 then
            goto continue
        end
        
        -- MA bias from FUN_1800a5280: when comparing MA vs non-MA,
        -- the non-MA's HP is treated as 5% lower (making it harder for
        -- non-MA to beat MA). A non-MA at 60% competes as if at 55%.
        local better = false
        if bestTarget == 0 then
            better = true
        elseif maID ~= 0 then
            if isMA and not isMABest then
                -- New candidate is MA, best is non-MA: penalize best
                better = (hpPct < bestHPPct - 5)
            elseif not isMA and isMABest then
                -- New candidate is non-MA, best is MA: penalize candidate
                better = (hpPct - 5 < bestHPPct)
            else
                -- Both MA or both non-MA: straight comparison
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

-- Heal Spell Selection (from FUN_1800a64e0 decompilation)
-- Returns spell_id to cast, or 0 if no heal needed
local function SelectHealSpell(targetID, hpPct, isMA)
    local settings = Cleric.Settings
    
    -- Determine thresholds based on MA status
    local quickHealThresh = isMA and settings.MAQuickHeal or settings.NonMAQuickHeal
    local interventionThresh = isMA and settings.MAIntervention or settings.NonMAIntervention
    local healsHealsThresh = isMA and settings.MAHealsHeals or settings.NonMAHealsHeals
    local dissidentThresh = isMA and settings.MADissident or settings.NonMADissident
    local promiseThresh = isMA and settings.MAPromise or settings.NonMAPromise
    local durationThresh = isMA and settings.MADurationHeal or settings.NonMADurationHeal
    
    -- 1. Quick Heal (from FUN_1800a64e0 priority 1)
    if hpPct < quickHealThresh then
        local spellID = GetSpellID(SPELL_GROUP_QUICK_HEAL)
        if spellID ~= 0 then
            local canCast, reason = Core.Cast.CanCast(spellID, targetID)
            if canCast then
                return spellID
            end
        end
    end
    
    -- 2. Intervention (priority 2)
    if hpPct < interventionThresh then
        local spellID = GetSpellID(SPELL_GROUP_INTERVENTION)
        if spellID ~= 0 then
            local canCast, reason = Core.Cast.CanCast(spellID, targetID)
            if canCast then
                return spellID
            end
        end
    end
    
    -- 3. Heals Heals line (Ardent/Merciful etc) (priority 3)
    if hpPct < healsHealsThresh then
        local spellID = GetSpellID(SPELL_GROUP_HEALS_HEALS)
        if spellID ~= 0 then
            local canCast, reason = Core.Cast.CanCast(spellID, targetID)
            if canCast then
                return spellID
            end
        end
    end
    
    -- 4. Dissident/Undying line (priority 4)
    if hpPct < dissidentThresh then
        local spellID = GetSpellID(SPELL_GROUP_DISSIDENT)
        if spellID ~= 0 then
            local canCast, reason = Core.Cast.CanCast(spellID, targetID)
            if canCast then
                return spellID
            end
        end
    end
    
    -- 5. Promise heal (only if buff not already on target)
    if hpPct < promiseThresh then
        local spellID = GetSpellID(SPELL_GROUP_PROMISE)
        if spellID ~= 0 then
            -- Check if buff already on target (from buff stacking logic)
            local groupID = Core.Spell.GetSpellGroup(spellID)
            if groupID ~= 0 and not Core.Buff.IsBuffOnTargetByGroup(targetID, groupID) then
                local canCast, reason = Core.Cast.CanCast(spellID, targetID)
                if canCast then
                    return spellID
                end
            end
        end
    end
    
    -- 6. Duration heal (HoT) - if KeepHoTUp or HP below threshold
    if settings.KeepHoTUp or hpPct < durationThresh then
        local spellID = GetSpellID(SPELL_GROUP_DURATION_HEAL)
        if spellID ~= 0 then
            -- Check if buff already on target
            local groupID = Core.Spell.GetSpellGroup(spellID)
            if groupID ~= 0 and not Core.Buff.IsBuffOnTargetByGroup(targetID, groupID) then
                local canCast, reason = Core.Cast.CanCast(spellID, targetID)
                if canCast then
                    return spellID
                end
            end
        end
    end
    
    -- 7. Splash heal (if enabled and multiple targets low)
    if settings.UseSplash then
        -- TODO: Implement multi-target splash logic
        -- For now, simple fallback
        local spellID = GetSpellID(SPELL_GROUP_SPLASH)
        if spellID ~= 0 and hpPct < 70 then
            local canCast, reason = Core.Cast.CanCast(spellID, targetID)
            if canCast then
                return spellID
            end
        end
    end
    
    return 0
end

-- Rez Logic (from FUN_180150b00 decompilation)
local function CheckRez()
    if not Cleric.Settings.UseRez then
        return false
    end
    
    -- Find nearby corpses
    -- TODO: Implement corpse search with range limit
    -- For now, use group members who are dead
    local groupMembers = Core.Group.GetGroupMembers()
    
    for _, spawnID in ipairs(groupMembers) do
        if spawnID == 0 then goto continue end
        
        -- Check if dead (StandState 0x02)
        local standState = Core.Spawn.GetStandState(spawnID)
        if standState == 0x02 then
            -- Validate as valid heal target (corpse type check from FUN_1800a64e0)
            if Core.Target.IsValidHealTarget(spawnID) then
                -- Priority 1: Divine Rez AA
                if Cleric.Settings.UseDivineRez then
                    local spellID = GetSpellID(SPELL_GROUP_DIVINE_REZ_AA)
                    if spellID ~= 0 then
                        local canCast, reason = Core.Cast.CanCast(spellID, spawnID)
                        if canCast then
                            Core.Cast.Cast(spellID, spawnID)
                            return true
                        end
                    end
                end
                
                -- Priority 2: Blessing of Resurrection
                local spellID = GetSpellID(SPELL_GROUP_BLESSING_REZ)
                if spellID ~= 0 then
                    local canCast, reason = Core.Cast.CanCast(spellID, spawnID)
                    if canCast then
                        Core.Cast.Cast(spellID, spawnID)
                        return true
                    end
                end
                
                -- Priority 3: Epic 1.0 (TODO: item click)
                if Cleric.Settings.UseEpicRez then
                    -- TODO: Implement epic clicky
                end
                
                -- Priority 4: Fallback rez spell
                spellID = GetSpellID(SPELL_GROUP_REZ_SPELL)
                if spellID ~= 0 then
                    local canCast, reason = Core.Cast.CanCast(spellID, spawnID)
                    if canCast then
                        Core.Cast.Cast(spellID, spawnID)
                        return true
                    end
                end
            end
        end
        
        ::continue::
    end
    
    return false
end

-- Downtime Logic (Yaulp, Ward AA)
local function CheckDowntime()
    local localID = GetLocalSpawnID()
    
    -- Yaulp (if enabled and not already buffed)
    if Cleric.Settings.UseYaulp then
        local spellID = GetSpellID(SPELL_GROUP_YAULP)
        if spellID ~= 0 then
            local groupID = Core.Spell.GetSpellGroup(spellID)
            if groupID ~= 0 and not Core.Buff.IsBuffOnTargetByGroup(localID, groupID) then
                local canCast, reason = Core.Cast.CanCast(spellID, localID)
                if canCast then
                    Core.Cast.Cast(spellID, localID)
                    return true
                end
            end
        end
    end
    
    -- Ward AA (if in combat)
    if Cleric.Settings.UseWardAA and Core.Utility.IsCombat() then
        local spellID = GetSpellID(SPELL_GROUP_WARD_AA)
        if spellID ~= 0 then
            local canCast, reason = Core.Cast.CanCast(spellID, localID)
            if canCast then
                Core.Cast.Cast(spellID, localID)
                return true
            end
        end
    end
    
    return false
end

-- Main Pulse Handler (from FUN_18009c200 dispatch pattern)
function Cleric.OnPulse()
    -- Check if paused (from Lifecycle system)
    if Core.Lifecycle.IsPaused() then
        return
    end
    
    -- Check gate flag (skip if set, from PcClient+0x1870)
    if Core.LocalPlayer.IsGateFlagSet() then
        return
    end
    
    -- Check if already casting
    local localID = GetLocalSpawnID()
    if Core.Spawn.IsCasting(localID) then
        return
    end
    
    -- Print init message once
    if not initPrinted then
        local name = Core.Spawn.GetName(localID)
        Core.Utility.Print(string.format("[MQ2CF] Cleric module loaded (player: %s)", name))
        initPrinted = true
        
        -- Load settings
        Core.Settings.LoadSettings("Cleric_" .. name)
    end
    
    -- Heal Triage
    local healTarget, hpPct = FindHealTarget()
    if healTarget ~= 0 then
        local isMA = (healTarget == Core.Group.GetMainAssist())
        local spellID = SelectHealSpell(healTarget, hpPct, isMA)
        
        if spellID ~= 0 then
            local success, err = Core.Cast.Cast(spellID, healTarget)
            if not success then
                Core.Utility.Print(string.format("Heal cast failed: %s", err))
            end
            return
        end
    end
    
    -- Rez Check
    if CheckRez() then
        return
    end
    
    -- Downtime Abilities
    CheckDowntime()
    
    -- TODO: Cure logic (from UseCures setting)
    if Cleric.Settings.UseCures then
        -- Implement cure detection and casting
    end
end

function Cleric.OnZoned()
    -- Reset cached state on zone
    spellCache = {}
    initPrinted = false
    localSpawnID = 0
    Core.Utility.Print("[MQ2CF] Cleric module reset for new zone")
end

-- Register with Core
Core.Lifecycle.RegisterPulse(Cleric.OnPulse, 100)
Core.Lifecycle.RegisterEvent("Zone", Cleric.OnZoned)

-- Load settings UI
Core.ImGui.RegisterWindow("Cleric Settings", function()
    ImGui.Text("Healing Thresholds")
    
    local settings = Cleric.Settings
    
    -- MA Thresholds
    ImGui.SeparatorText("Main Assist Thresholds")
    settings.MAQuickHeal = ImGui.SliderInt("MA Quick Heal %", settings.MAQuickHeal, 10, 100)
    settings.MAIntervention = ImGui.SliderInt("MA Intervention %", settings.MAIntervention, 10, 100)
    settings.MAHealsHeals = ImGui.SliderInt("MA Heals Heals %", settings.MAHealsHeals, 10, 100)
    settings.MADissident = ImGui.SliderInt("MA Dissident %", settings.MADissident, 10, 100)
    settings.MAPromise = ImGui.SliderInt("MA Promise %", settings.MAPromise, 10, 100)
    settings.MADurationHeal = ImGui.SliderInt("MA Duration Heal %", settings.MADurationHeal, 10, 100)
    
    -- Non-MA Thresholds
    ImGui.SeparatorText("Non-MA Thresholds")
    settings.NonMAQuickHeal = ImGui.SliderInt("Non-MA Quick Heal %", settings.NonMAQuickHeal, 10, 100)
    settings.NonMAIntervention = ImGui.SliderInt("Non-MA Intervention %", settings.NonMAIntervention, 10, 100)
    settings.NonMAHealsHeals = ImGui.SliderInt("Non-MA Heals Heals %", settings.NonMAHealsHeals, 10, 100)
    settings.NonMADissident = ImGui.SliderInt("Non-MA Dissident %", settings.NonMADissident, 10, 100)
    settings.NonMAPromise = ImGui.SliderInt("Non-MA Promise %", settings.NonMAPromise, 10, 100)
    settings.NonMADurationHeal = ImGui.SliderInt("Non-MA Duration Heal %", settings.NonMADurationHeal, 10, 100)
    
    -- Feature Toggles
    ImGui.SeparatorText("Features")
    settings.UseRez = ImGui.Checkbox("Use Rez", settings.UseRez)
    settings.UseDivineRez = ImGui.Checkbox("Use Divine Rez AA", settings.UseDivineRez)
    settings.UseEpicRez = ImGui.Checkbox("Use Epic Rez", settings.UseEpicRez)
    settings.UseYaulp = ImGui.Checkbox("Use Yaulp", settings.UseYaulp)
    settings.UseWardAA = ImGui.Checkbox("Use Ward AA", settings.UseWardAA)
    settings.UseXTargetHealing = ImGui.Checkbox("Heal XTargets", settings.UseXTargetHealing)
    settings.UseSplash = ImGui.Checkbox("Use Splash Heals", settings.UseSplash)
    settings.UseCures = ImGui.Checkbox("Use Cures", settings.UseCures)
    settings.BattleMode = ImGui.Checkbox("Battle Mode", settings.BattleMode)
    settings.KeepHoTUp = ImGui.Checkbox("Keep HoT Up", settings.KeepHoTUp)
    
    -- Save button
    if ImGui.Button("Save Settings") then
        Core.Settings.SaveSettings()
    end
end)

return Cleric
