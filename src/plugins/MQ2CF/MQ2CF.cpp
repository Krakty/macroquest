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
#include "core/ModeSystem.h"
#include "core/ImGuiWindow.h"
#include "core/SchemaInit.h"
#include "core/EventBus.h"
#include "core/BuffTracker.h"

#include <filesystem>

PreSetup("MQ2CF");
PLUGIN_VERSION(0.1);

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------
static std::unique_ptr<sol::state> s_lua;
static sqlite3* s_db = nullptr;       // global DB (shared settings, buff rules, immune lists)
static sqlite3* s_charDb = nullptr;   // per-character DB (spells, AAs, char-specific settings)
static bool s_luaReady = false;

// Path to the Lua scripts directory, resolved at startup
static std::string s_luaDir;

// Paths to the SQLite databases
static std::string s_dbPath;
static std::string s_charDbPath;

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
	std::string pluginDir = GetPluginDir();
	fs::create_directories(pluginDir);

	// Global DB - shared settings, buff rules, immune lists
	s_dbPath = pluginDir + "\\MQ2CF_global.db";
	int rc = sqlite3_open(s_dbPath.c_str(), &s_db);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to open global database: %s", sqlite3_errmsg(s_db));
		s_db = nullptr;
		return false;
	}
	WriteChatf("\ag[MQ2CF]\ax Global DB: %s", s_dbPath.c_str());

	return true;
}

static bool OpenCharDatabase()
{
	if (!pLocalPlayer)
		return false;

	std::string pluginDir = GetPluginDir();
	std::string serverName = GetServerShortName();
	std::string charName = pLocalPlayer->Name;

	s_charDbPath = pluginDir + "\\MQ2CF_" + serverName + "_" + charName + ".db";
	int rc = sqlite3_open(s_charDbPath.c_str(), &s_charDb);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to open character database: %s", sqlite3_errmsg(s_charDb));
		s_charDb = nullptr;
		return false;
	}
	WriteChatf("\ag[MQ2CF]\ax Character DB: %s", s_charDbPath.c_str());

	return true;
}

static void CloseDatabase()
{
	if (s_charDb)
	{
		sqlite3_close(s_charDb);
		s_charDb = nullptr;
	}
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

	// Register our C++ bindings (pass db handle for subsystems that need it)
	CF::RegisterLuaBindings(*s_lua, s_db, s_charDb);

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
	CF::ClearEventRegistrations();
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
		WriteChatf("  Lua state:  %s", s_luaReady ? "\agactive\ax" : "\arinactive\ax");
		WriteChatf("  Global DB:  %s", s_db ? "\agopen\ax" : "\arclosed\ax");
		WriteChatf("  Char DB:    %s", s_charDb ? "\agopen\ax" : "\arclosed\ax");
		WriteChatf("  Script dir: %s", s_luaDir.c_str());
		WriteChatf("  Global DB:  %s", s_dbPath.c_str());
		WriteChatf("  Char DB:    %s", s_charDbPath.empty() ? "not opened" : s_charDbPath.c_str());
		WriteChatf("  Mode:       \ag%s\ax (%d)", CF::GetModeName(CF::GetCurrentMode()), CF::GetCurrentMode());
		return;
	}

	if (ci_equals(arg, "reload"))
	{
		ReloadLua();
		return;
	}

	if (ci_equals(arg, "scan"))
	{
		if (!s_luaReady)
		{
			WriteChatf("\ar[MQ2CF]\ax Lua not initialized");
			return;
		}
		// Call Core.SpellScan.ScanAll() from Lua
		auto result = s_lua->safe_script("Core.SpellScan.ScanAll()", sol::script_pass_on_error);
		if (!result.valid())
		{
			sol::error err = result;
			WriteChatf("\ar[MQ2CF]\ax Scan error: %s", err.what());
		}
		return;
	}

	if (ci_equals(arg, "test"))
	{
		if (!s_luaReady)
		{
			WriteChatf("\ar[MQ2CF]\ax Lua not ready");
			return;
		}

		// Load test harness if not already loaded
		std::string testPath = s_luaDir + "\\test_harness.lua";
		auto loadResult = s_lua->safe_script_file(testPath, sol::script_pass_on_error);
		if (!loadResult.valid())
		{
			sol::error err = loadResult;
			WriteChatf("\ar[MQ2CF]\ax Failed to load test harness: %s", err.what());
			return;
		}

		// Get optional subsystem argument
		char arg2[MAX_STRING] = { 0 };
		GetArg(arg2, szLine, 2);

		// Run tests
		sol::table testModule = loadResult;
		sol::function runFn = testModule["run"];
		if (runFn.valid())
		{
			auto result = runFn(arg2[0] ? arg2 : "all");
			if (!result.valid())
			{
				sol::error err = result;
				WriteChatf("\ar[MQ2CF]\ax Test error: %s", err.what());
			}
		}
		return;
	}

	if (ci_equals(arg, "mode"))
	{
		char arg2[MAX_STRING] = { 0 };
		GetArg(arg2, szLine, 2);

		// No argument: print current mode
		if (arg2[0] == '\0')
		{
			WriteChatf("\ag[MQ2CF]\ax Current mode: %s (%d)", CF::GetModeName(CF::GetCurrentMode()), CF::GetCurrentMode());
			return;
		}

		int newMode = -1;

		// Try numeric first
		if (arg2[0] >= '0' && arg2[0] <= '9')
		{
			newMode = atoi(arg2);
		}
		else
		{
			// Canonical names + legacy aliases (backward compat for other classes)
			if (ci_equals(arg2, "manual"))             newMode = 0;
			else if (ci_equals(arg2, "assist"))        newMode = 1;
			else if (ci_equals(arg2, "chase"))         newMode = 2;
			else if (ci_equals(arg2, "chaseassist"))   newMode = 2;
			else if (ci_equals(arg2, "nocamp"))        newMode = 3;
			else if (ci_equals(arg2, "vorpal"))        newMode = 3;  // legacy alias
			else if (ci_equals(arg2, "tank"))          newMode = 4;
			else if (ci_equals(arg2, "pullertank"))    newMode = 5;
			else if (ci_equals(arg2, "pullerassist"))  newMode = 6;
			else if (ci_equals(arg2, "manualtank"))    newMode = 7;
			else if (ci_equals(arg2, "sictank"))       newMode = 7;  // legacy alias
			else if (ci_equals(arg2, "sic"))           newMode = 7;  // legacy alias
			else if (ci_equals(arg2, "roaming"))       newMode = 8;
			else if (ci_equals(arg2, "huntertank"))    newMode = 8;  // legacy alias
			else if (ci_equals(arg2, "hunter"))        newMode = 8;  // legacy alias
		}

		if (newMode < 0 || newMode > 8)
		{
			WriteChatf("\ar[MQ2CF]\ax Unknown mode: %s", arg2);
			WriteChatf("  Valid modes: manual(0) assist(1) chase(2) nocamp(3) tank(4) pullertank(5) pullerassist(6) manualtank(7) roaming(8)");
			return;
		}

		CF::SetCurrentMode(newMode);

		// Sync to Lua side
		if (s_luaReady)
		{
			auto result = s_lua->safe_script(
				"if Cleric then Cleric.mode = " + std::to_string(newMode) + " end",
				sol::script_pass_on_error);
		}
		return;
	}

	if (ci_equals(arg, "cast"))
	{
		char spellName[MAX_STRING] = { 0 };
		char arg3[MAX_STRING] = { 0 };
		char targetName[MAX_STRING] = { 0 };
		GetArg(spellName, szLine, 2);
		GetArg(arg3, szLine, 3);
		GetArg(targetName, szLine, 4);

		if (spellName[0] == '\0')
		{
			WriteChatf("\ar[MQ2CF]\ax Usage: /cf cast \"SpellName\" [on TargetName]");
			return;
		}

		if (s_luaReady && s_lua)
		{
			// Sanitize spell name for Lua string (escape single quotes)
			std::string sanitized(spellName);
			size_t pos = 0;
			while ((pos = sanitized.find('\'', pos)) != std::string::npos)
			{
				sanitized.replace(pos, 1, "\\'");
				pos += 2;
			}
			std::string targetSanitized(targetName);
			pos = 0;
			while ((pos = targetSanitized.find('\'', pos)) != std::string::npos)
			{
				targetSanitized.replace(pos, 1, "\\'");
				pos += 2;
			}

			std::string script = "if Cleric and Cleric.OneOffCast then Cleric:OneOffCast('"
				+ sanitized + "', '"
				+ targetSanitized + "') end";
			auto result = s_lua->safe_script(script, sol::script_pass_on_error);
			if (!result.valid())
			{
				sol::error err = result;
				WriteChatf("\ar[MQ2CF]\ax Cast error: %s", err.what());
			}
		}
		else
		{
			WriteChatf("\ar[MQ2CF]\ax Lua not ready");
		}
		return;
	}

	if (ci_equals(arg, "ui"))
	{
		CF::ToggleSettingsWindow();
		return;
	}

	WriteChatf("\ar[MQ2CF]\ax Unknown subcommand: %s", arg);
	WriteChatf("  Usage: /cf [status|reload|test [subsystem]|scan|mode [name|#]|ui|cast \"spell\" [on target]]");
}

// ---------------------------------------------------------------------------
// Plugin callbacks
// ---------------------------------------------------------------------------
PLUGIN_API void InitializePlugin()
{
	WriteChatf("\ag[MQ2CF]\ax Class Framework loading...");

	// Initialize SQLite database
	OpenDatabase();

	// Create runtime settings table if needed
	if (s_db)
		CF::InitSettingsDB(s_db);

	// Register /cf command
	AddCommand("/cf", CmdCF);

	// If already in-game, open char DB before Lua init so bindings get the right pointer
	if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
	{
		if (!s_charDb)
		{
			OpenCharDatabase();
			if (s_charDb)
			{
				CF::InitSettingsDB(s_charDb);
				CF::InitSpellScanDB(s_charDb);
				CF::InitGemDB(s_charDb);
				CF::InitCampEventsDB(s_charDb);
				CF::InitZoneDefaultsDB(s_charDb);
			}
		}
	}

	// Initialize Lua state and bindings (after char DB so SpellScan gets the right pointer)
	InitializeLua();

	// Load class module if already in-game
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
	// Buff tracker runs before Lua gate -- it must run on every character
	CF::BuffTracker_OnPulse();

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
	CF::BuffTracker_OnZoned();

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
	CF::BuffTracker_OnSetGameState(GameState);

	if (GameState == GAMESTATE_INGAME)
	{
		// Open per-character DB on entering game
		if (!s_charDb && pLocalPlayer)
		{
			OpenCharDatabase();
			if (s_charDb)
			{
				CF::InitSettingsDB(s_charDb);
				CF::InitSpellScanDB(s_charDb);
				CF::InitGemDB(s_charDb);
				CF::InitCampEventsDB(s_charDb);
				CF::InitZoneDefaultsDB(s_charDb);
				CF::UpdateSpellScanDB(s_charDb);  // Update the live pointer for Lua bindings
			}
		}

		// Load class module when we enter the game
		LoadClassModule();
	}
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (s_luaReady && s_lua)
		CF::ProcessChatLine(s_lua.get(), Line);
	return false;
}

PLUGIN_API void OnUpdateImGui()
{
	if (CF::IsSettingsWindowOpen())
	{
		CF::DrawSettingsWindow(s_lua.get(), s_luaReady,
			s_dbPath.c_str(), s_charDbPath.empty() ? nullptr : s_charDbPath.c_str());
	}
}
