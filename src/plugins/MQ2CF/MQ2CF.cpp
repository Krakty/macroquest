/*
 * MQ2CF: MacroQuest Class Framework
 * Open-source replacement for CWTN class plugins
 *
 * Architecture: C++ core with Lua class modules and SQLite data
 * The C++ core exposes eqlib primitives to Lua. All decision logic
 * (heal triage, spell selection, CC priority) lives in Lua scripts.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <sqlite3.h>

#include "core/LuaBindings.h"

#include <filesystem>

PreSetup("MQ2CF");
PLUGIN_VERSION(0.1);

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------
static std::unique_ptr<sol::state> s_lua;
static sqlite3* s_db = nullptr;
static bool s_luaReady = false;

// Path to the Lua scripts directory, resolved at startup
static std::string s_luaDir;

// Path to the SQLite database
static std::string s_dbPath;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string GetPluginDir()
{
	// MQ stores plugin resources relative to the MQ path
	return std::string(gPathResources) + "\\MQ2CF";
}

static bool OpenDatabase()
{
	s_dbPath = GetPluginDir() + "\\MQ2CF.db";

	// Ensure the directory exists
	fs::create_directories(fs::path(s_dbPath).parent_path());

	int rc = sqlite3_open(s_dbPath.c_str(), &s_db);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to open database: %s", sqlite3_errmsg(s_db));
		s_db = nullptr;
		return false;
	}

	WriteChatf("\ag[MQ2CF]\ax Database opened: %s", s_dbPath.c_str());
	return true;
}

static void CloseDatabase()
{
	if (s_db)
	{
		sqlite3_close(s_db);
		s_db = nullptr;
	}
}

static void InitializeLua()
{
	s_lua = std::make_unique<sol::state>();
	s_lua->open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::string,
		sol::lib::table,
		sol::lib::math,
		sol::lib::os,
		sol::lib::io
	);

	// Register our C++ bindings
	CF::RegisterLuaBindings(*s_lua);

	// Set up the Lua package path to include our lua/ directory
	s_luaDir = GetPluginDir() + "\\lua";
	fs::create_directories(s_luaDir);

	sol::table package = (*s_lua)["package"];
	std::string currentPath = package["path"].get_or<std::string>("");
	package["path"] = currentPath + ";" + s_luaDir + "\\?.lua";

	s_luaReady = true;
	WriteChatf("\ag[MQ2CF]\ax Lua state initialized (script dir: %s)", s_luaDir.c_str());
}

static void ShutdownLua()
{
	s_luaReady = false;
	s_lua.reset();
}

static void LoadClassModule()
{
	if (!s_luaReady)
		return;

	std::string modulePath = s_luaDir + "\\cleric.lua";
	if (!fs::exists(modulePath))
	{
		WriteChatf("\ay[MQ2CF]\ax No Lua module found at %s", modulePath.c_str());
		return;
	}

	auto result = s_lua->safe_script_file(modulePath, sol::script_pass_on_error);
	if (!result.valid())
	{
		sol::error err = result;
		WriteChatf("\ar[MQ2CF]\ax Lua load error: %s", err.what());
		return;
	}

	WriteChatf("\ag[MQ2CF]\ax Loaded class module: cleric.lua");
}

static void ReloadLua()
{
	WriteChatf("\ag[MQ2CF]\ax Reloading Lua state...");
	ShutdownLua();
	InitializeLua();
	LoadClassModule();
}

// ---------------------------------------------------------------------------
// /cf command
// ---------------------------------------------------------------------------
static void CmdCF(SPAWNINFO* pSpawn, char* szLine)
{
	char arg[MAX_STRING] = { 0 };
	GetArg(arg, szLine, 1);

	if (arg[0] == '\0' || ci_equals(arg, "status"))
	{
		WriteChatf("\ag[MQ2CF]\ax Status:");
		WriteChatf("  Lua state: %s", s_luaReady ? "\agactive\ax" : "\arinactive\ax");
		WriteChatf("  Database:  %s", s_db ? "\agopen\ax" : "\arclosed\ax");
		WriteChatf("  Script dir: %s", s_luaDir.c_str());
		WriteChatf("  DB path:    %s", s_dbPath.c_str());
		return;
	}

	if (ci_equals(arg, "reload"))
	{
		ReloadLua();
		return;
	}

	WriteChatf("\ar[MQ2CF]\ax Unknown subcommand: %s", arg);
	WriteChatf("  Usage: /cf [status|reload]");
}

// ---------------------------------------------------------------------------
// Plugin callbacks
// ---------------------------------------------------------------------------
PLUGIN_API void InitializePlugin()
{
	WriteChatf("\ag[MQ2CF]\ax Class Framework loading...");

	// Initialize SQLite database
	OpenDatabase();

	// Initialize Lua state and bindings
	InitializeLua();

	// Register /cf command
	AddCommand("/cf", CmdCF);

	// Load class module if we're already in-game
	if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
	{
		LoadClassModule();
	}

	WriteChatf("\ag[MQ2CF]\ax Plugin initialized (v%.1f)", MQ2Version);
}

PLUGIN_API void ShutdownPlugin()
{
	WriteChatf("\ag[MQ2CF]\ax Shutting down...");

	RemoveCommand("/cf");
	ShutdownLua();
	CloseDatabase();
}

PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME)
		return;

	if (!pLocalPlayer || !pLocalPC)
		return;

	if (!s_luaReady)
		return;

	// Call the Lua OnPulse if the Cleric module registered one
	sol::object clericModule = (*s_lua)["Cleric"];
	if (clericModule.valid() && clericModule.get_type() == sol::type::table)
	{
		sol::table cleric = clericModule.as<sol::table>();
		sol::object onPulse = cleric["OnPulse"];
		if (onPulse.valid() && onPulse.get_type() == sol::type::function)
		{
			auto result = cleric["OnPulse"](cleric);
			if (!result.valid())
			{
				sol::error err = result;
				WriteChatf("\ar[MQ2CF]\ax Lua OnPulse error: %s", err.what());
			}
		}
	}
}

PLUGIN_API void OnZoned()
{
	// Notify Lua of zone change
	if (!s_luaReady)
		return;

	sol::object clericModule = (*s_lua)["Cleric"];
	if (clericModule.valid() && clericModule.get_type() == sol::type::table)
	{
		sol::table cleric = clericModule.as<sol::table>();
		sol::object onZoned = cleric["OnZoned"];
		if (onZoned.valid() && onZoned.get_type() == sol::type::function)
		{
			auto result = cleric["OnZoned"](cleric);
			if (!result.valid())
			{
				sol::error err = result;
				WriteChatf("\ar[MQ2CF]\ax Lua OnZoned error: %s", err.what());
			}
		}
	}
}

PLUGIN_API void SetGameState(int GameState)
{
	if (GameState == GAMESTATE_INGAME)
	{
		// Load class module when we enter the game
		LoadClassModule();
	}
}

PLUGIN_API void OnUpdateImGui()
{
	// TODO Phase 6: Dispatch to Lua ImGui render callbacks
}
