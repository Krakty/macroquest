/*
 * MQ2CF: MacroQuest Class Framework
 * Lua bindings -- exposes eqlib primitives to Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

#include "SpawnPrimitives.h"
#include "SpellData.h"
#include "GroupEngine.h"
#include "LocalPlayer.h"
#include "TargetValidation.h"
#include "CastEngine.h"
#include "Lifecycle.h"
#include "Humanize.h"
#include "Settings.h"
#include "SpellScan.h"
#include "Camp.h"
#include "ModeSystem.h"
#include "BuffEngine.h"
#include "AAEngine.h"
#include "GemManager.h"
#include "EventBus.h"

namespace CF {

// Register all Core.* bindings on the given Lua state.
// Call once after creating the sol::state.
// The db parameter is passed through to subsystems that need SQLite access.
// globalDb = shared settings, buff rules, immune lists
// charDb = per-character spells, AAs, char-specific settings (may be null at startup)
void RegisterLuaBindings(sol::state& lua, sqlite3* globalDb, sqlite3* charDb);

// Individual subsystem registrations -- called by RegisterLuaBindings.
// Each expects the Core table to already exist.
void RegisterSpawnBindings(sol::table& core);
void RegisterSpellBindings(sol::table& core);
void RegisterGroupBindings(sol::table& core);
void RegisterLocalPlayerBindings(sol::table& core);
void RegisterTargetBindings(sol::table& core);
void RegisterCastBindings(sol::table& core);
void RegisterLifecycleBindings(sol::table& core);
void RegisterHumanizeBindings(sol::table& core);
void RegisterSettingsBindings(sol::table& core, sqlite3* db);
void RegisterSpellScanBindings(sol::table& core, sqlite3* db);
void RegisterCampBindings(sol::table& core);
void RegisterModeBindings(sol::table& core);
void RegisterBuffBindings(sol::table& core);
void RegisterAABindings(sol::table& core);
void RegisterGemBindings(sol::table& core, sqlite3* db);
void RegisterEventBindings(sol::table& core, sqlite3* db);

} // namespace CF
