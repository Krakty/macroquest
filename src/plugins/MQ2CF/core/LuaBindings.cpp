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
}

// ---------------------------------------------------------------------------
// Core.Spawn bindings
// ---------------------------------------------------------------------------
static void RegisterSpawnBindings(sol::table& core)
{
	sol::table spawn = core["Spawn"].get_or_create<sol::table>();

	spawn.set_function("GetName", [](int spawnId) -> std::string {
		SPAWNINFO* pSpawn = GetSpawnByID(spawnId);
		if (pSpawn)
			return pSpawn->Name;
		return "";
	});

	spawn.set_function("GetLocalName", []() -> std::string {
		if (pLocalPlayer)
			return pLocalPlayer->Name;
		return "";
	});

	spawn.set_function("GetLocalId", []() -> int {
		if (pLocalPlayer)
			return pLocalPlayer->SpawnID;
		return 0;
	});
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void RegisterLuaBindings(sol::state& lua)
{
	// Create the top-level Core table
	sol::table core = lua["Core"].get_or_create<sol::table>();

	RegisterUtilBindings(core);
	RegisterSpawnBindings(core);
}

} // namespace CF
