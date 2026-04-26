/*
 * MQ2CF: MacroQuest Class Framework
 * Settings subsystem -- persists configuration to SQLite, exposes to Lua
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <sqlite3.h>

namespace CF {

// Create the settings_runtime table if it doesn't exist.
// Call once after opening the database.
void InitSettingsDB(sqlite3* db);

// Register Core.Settings.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterSettingsBindings(sol::table& core, sqlite3* db);

} // namespace CF
