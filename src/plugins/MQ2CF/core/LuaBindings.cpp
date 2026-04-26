/*
 * MQ2CF: MacroQuest Class Framework
 * Lua bindings -- exposes eqlib primitives to Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "LuaBindings.h"

namespace CF {

// ---------------------------------------------------------------------------
// Core.Util bindings
// ---------------------------------------------------------------------------
static void RegisterUtilBindings(sol::table& core)
{
	sol::table util = core["Util"].get_or_create<sol::table>();

	util.set_function("GetGameState", []() -> int {
		return GetGameState();
	});

	util.set_function("IsInGame", []() -> bool {
		return GetGameState() == GAMESTATE_INGAME;
	});

	util.set_function("IsPluginLoaded", [](const std::string& name) -> bool {
		return IsPluginLoaded(name.c_str());
	});

	util.set_function("WriteChatf", [](const std::string& msg) {
		WriteChatf("%s", msg.c_str());
	});

	util.set_function("GetResourcePath", []() -> std::string {
		return std::string(gPathResources);
	});
}

// Subsystem bindings in separate files:
// Core.Spawn     -> SpawnPrimitives.cpp
// Core.Spell     -> SpellData.cpp
// Core.Group     -> GroupEngine.cpp
// Core.Lifecycle  -> Lifecycle.cpp
// Core.Humanize  -> Humanize.cpp
// Core.Settings  -> Settings.cpp
// Core.SpellScan -> SpellScan.cpp
// Core.Camp      -> Camp.cpp
// Core.Buff      -> BuffEngine.cpp
// Core.AA        -> AAEngine.cpp
// Core.Event     -> EventBus.cpp

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void RegisterLuaBindings(sol::state& lua, sqlite3* globalDb, sqlite3* charDb)
{
	// Create the top-level Core table
	sol::table core = lua["Core"].get_or_create<sol::table>();

	RegisterUtilBindings(core);
	RegisterSpawnBindings(core);
	RegisterSpellBindings(core);
	RegisterGroupBindings(core);
	RegisterLocalPlayerBindings(core);
	RegisterTargetBindings(core);
	RegisterCastBindings(core);
	RegisterLifecycleBindings(core);
	RegisterHumanizeBindings(core);
	RegisterSettingsBindings(core, globalDb);
	RegisterSpellScanBindings(core, charDb ? charDb : globalDb);
	RegisterCampBindings(core);
	RegisterModeBindings(core);
	RegisterBuffBindings(core);
	RegisterAABindings(core);
	RegisterGemBindings(core, charDb ? charDb : globalDb);
	RegisterEventBindings(core, globalDb);
}

} // namespace CF
