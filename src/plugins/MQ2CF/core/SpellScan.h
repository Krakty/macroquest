/*
 * MQ2CF: MacroQuest Class Framework
 * SpellScan subsystem -- scans spellbook, AAs, and discs into SQLite
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <sqlite3.h>

namespace CF {

// Create the scanned_spells and scanned_aas tables if they don't exist.
// Call once after opening the database.
void InitSpellScanDB(sqlite3* db);

// Register Core.SpellScan.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterSpellScanBindings(sol::table& core, sqlite3* db);

// Update the DB pointer used by SpellScan bindings.
// Call after opening/changing the character database.
void UpdateSpellScanDB(sqlite3* db);

} // namespace CF
