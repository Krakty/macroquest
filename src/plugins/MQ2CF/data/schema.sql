-- MQ2CF Database Schema
-- Spell lines, AA priorities, loadouts, and settings

CREATE TABLE IF NOT EXISTS spell_lines (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,       -- EQ class enum (2=cleric, 10=bard, etc.)
    line_name TEXT NOT NULL,          -- e.g. "QuickHeal", "Intervention", "Aria"
    spell_group_id INTEGER NOT NULL,  -- passed to GetHighestLearnedSpellByGroupID
    category INTEGER,                 -- EQ_Spell+0x94
    subcategory INTEGER,              -- EQ_Spell+0x98
    priority INTEGER DEFAULT 0,       -- higher = checked first
    gem_slot INTEGER DEFAULT -1       -- preferred gem slot, -1 = any
);

CREATE TABLE IF NOT EXISTS aa_abilities (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,
    ability_name TEXT NOT NULL,       -- e.g. "Burst of Life", "Divine Arbitration"
    priority INTEGER DEFAULT 0,       -- cast order within the AA chain
    condition TEXT,                    -- Lua expression for when to use (e.g. "hp < 30")
    category TEXT DEFAULT 'combat'    -- combat, burn, heal, cure, utility
);

CREATE TABLE IF NOT EXISTS loadouts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,
    loadout_id INTEGER NOT NULL,     -- 0=heal, 1=hybrid, 2=dps (cleric); 0-4 (bard)
    loadout_name TEXT NOT NULL,
    gem_slot INTEGER NOT NULL,
    spell_line_id INTEGER REFERENCES spell_lines(id)
);

CREATE TABLE IF NOT EXISTS settings_defaults (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER NOT NULL,       -- 0 = shared across all classes
    key TEXT NOT NULL,
    value TEXT NOT NULL,
    value_type TEXT DEFAULT 'bool',  -- bool, int, float, string
    description TEXT,
    ini_section TEXT DEFAULT 'General'
);

CREATE TABLE IF NOT EXISTS immune_list (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spawn_name TEXT NOT NULL,
    immune_type TEXT NOT NULL,        -- 'mez', 'charm'
    zone_id INTEGER,
    added_timestamp INTEGER DEFAULT (strftime('%s', 'now'))
);

CREATE TABLE IF NOT EXISTS buff_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spell_group_id_1 INTEGER NOT NULL,
    spell_group_id_2 INTEGER NOT NULL,
    conflict_type TEXT DEFAULT 'overwrite'  -- overwrite, block, stack
);
