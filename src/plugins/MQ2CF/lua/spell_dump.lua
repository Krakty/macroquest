-- MQ2CF Spell Dumper
-- Run via: /lua run MQ2CF/spell_dump
-- Dumps all spells, AAs, and discs for the current character to a file

local mq = require('mq')

local charName = mq.TLO.Me.Name() or "unknown"
local charClass = mq.TLO.Me.Class.ShortName() or "UNK"
local charLevel = mq.TLO.Me.Level() or 0

local outPath = mq.configDir .. "\\..\\resources\\MQ2CF\\spell_dump_" .. charName .. ".txt"
local f = io.open(outPath, "w")
if not f then
    printf("[SpellDump] Cannot open: %s", outPath)
    return
end

local function w(line)
    f:write(line .. "\n")
end

w("=== MQ2CF Spell Dump ===")
w("Character: " .. charName)
w("Class: " .. charClass)
w("Level: " .. tostring(charLevel))
w("Date: " .. os.date())
w("")

-- Spellbook: Me.Book(slot)() returns spell name, Me.Book("name")() returns slot
w("=== SPELLBOOK ===")
w("slot|name|level|mana|castTime|recastTime|range|aeRange|targetType|category|subcategory|spellGroup|duration")
local bookCount = 0
for slot = 1, 1920 do
    local name = mq.TLO.Me.Book(slot)()
    if name and name ~= "" then
        local spell = mq.TLO.Spell(name)
        if spell() then
            w(string.format("%d|%s|%d|%d|%.1f|%.1f|%.1f|%.1f|%s|%s|%s|%d|%s",
                slot,
                name,
                spell.Level() or 0,
                spell.Mana() or 0,
                (spell.MyCastTime() or 0) / 1000.0,
                (spell.RecastTime() or 0) / 1000.0,
                spell.Range() or 0,
                spell.AERange() or 0,
                spell.TargetType() or "",
                spell.Category() or "",
                spell.Subcategory() or "",
                spell.SpellGroup() or 0,
                tostring(spell.Duration() or "0")
            ))
            bookCount = bookCount + 1
        end
    end
end
w("Total: " .. bookCount)
w("")

-- Memorized gems: Me.Gem(n)() returns spell name
w("=== MEMORIZED GEMS ===")
w("gem|name|ready")
for gem = 1, 15 do
    local name = mq.TLO.Me.Gem(gem)()
    if name and name ~= "" then
        local ready = mq.TLO.Me.SpellReady(gem)() or false
        w(string.format("%d|%s|%s", gem, name, tostring(ready)))
    end
end
w("")

-- AAs: Me.AltAbility(name_or_id)
w("=== ALTERNATE ABILITIES ===")
w("id|name|rank|maxRank|myReuseTime|spellName|ready")
local aaCount = 0
for i = 1, 1000 do
    local aa = mq.TLO.Me.AltAbility(i)
    if aa() then
        local name = aa.Name()
        if name and name ~= "" then
            local spellName = ""
            if aa.Spell() then
                spellName = aa.Spell.Name() or ""
            end
            local ready = mq.TLO.Me.AltAbilityReady(i)() or false
            w(string.format("%d|%s|%d|%d|%.1f|%s|%s",
                aa.ID() or i,
                name,
                aa.Rank() or 0,
                aa.MaxRank() or 0,
                (aa.MyReuseTime() or 0) / 1000.0,
                spellName,
                tostring(ready)
            ))
            aaCount = aaCount + 1
        end
    end
end
w("Total: " .. aaCount)
w("")

-- Combat abilities (discs): Me.CombatAbility(n)() returns name
w("=== COMBAT ABILITIES ===")
w("slot|name|level|recastTime|ready")
local discCount = 0
for slot = 1, 30 do
    local name = mq.TLO.Me.CombatAbility(slot)()
    if name and name ~= "" then
        local spell = mq.TLO.Spell(name)
        local ready = mq.TLO.Me.CombatAbilityReady(name)() or false
        w(string.format("%d|%s|%d|%.1f|%s",
            slot,
            name,
            spell() and spell.Level() or 0,
            spell() and (spell.RecastTime() or 0) / 1000.0 or 0,
            tostring(ready)
        ))
        discCount = discCount + 1
    end
end
w("Total: " .. discCount)
w("")

-- Active buffs: Me.Buff(n)
w("=== ACTIVE BUFFS ===")
w("slot|name|duration|caster")
for slot = 1, 97 do
    local buff = mq.TLO.Me.Buff(slot)
    if buff() then
        w(string.format("%d|%s|%s|%s",
            slot,
            buff.Name() or "",
            tostring(buff.Duration() or "0"),
            buff.Caster() or ""
        ))
    end
end
w("")

-- Heal spell analysis for healer classes
if charClass == "CLR" or charClass == "DRU" or charClass == "SHM" then
    w("=== HEAL SPELL ANALYSIS ===")
    w("name|spellGroup|category|subcategory|mana|castTime|level|range|base1")
    for slot = 1, 1920 do
        local name = mq.TLO.Me.Book(slot)()
        if name and name ~= "" then
            local spell = mq.TLO.Spell(name)
            if spell() then
                local cat = spell.Category() or ""
                local subcat = spell.Subcategory() or ""
                if cat == "Heals" or cat == "HP Buffs"
                   or subcat == "Heal" or subcat == "HoT" or subcat == "Cure"
                   or subcat == "Resurrection" or subcat == "HP" then
                    w(string.format("%s|%d|%s|%s|%d|%.1f|%d|%.1f|%d",
                        name,
                        spell.SpellGroup() or 0,
                        cat, subcat,
                        spell.Mana() or 0,
                        (spell.MyCastTime() or 0) / 1000.0,
                        spell.Level() or 0,
                        spell.Range() or 0,
                        spell.Base(1)() or 0
                    ))
                end
            end
        end
    end
    w("")
end

f:close()
printf("[SpellDump] Done: %s", outPath)
printf("[SpellDump] %d spells, %d AAs, %d discs", bookCount, aaCount, discCount)
