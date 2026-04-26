/*
 * MQ2CF: MacroQuest Class Framework
 * Event subsystem -- matches chat lines against registered patterns and dispatches callbacks
 *
 * Stores a list of named event patterns. When a chat line arrives, each enabled
 * pattern is tested via substring match. On hit, the associated Lua callback
 * fires with the full line text.
 *
 * On init, loads enabled state from the SQLite camp_events table so that
 * persisted enable/disable decisions survive reloads.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "EventBus.h"

#include <vector>
#include <string>
#include <cstring>

namespace CF {

// ---------------------------------------------------------------------------
// Internal data
// ---------------------------------------------------------------------------

struct EventPattern {
	std::string name;
	std::string pattern;
	sol::function callback;
	bool enabled;
};

static std::vector<EventPattern> s_events;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Load enabled state from SQLite camp_events table.
// Events that exist in the DB get their enabled flag synced.
static void LoadEnabledStateFromDB(sqlite3* db)
{
	if (!db) return;

	const char* sql = "SELECT event_name, enabled FROM camp_events";
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK)
		return;

	// Build a quick lookup -- for each DB row, find matching registered event
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		int enabled = sqlite3_column_int(stmt, 1);
		if (!name) continue;

		for (auto& evt : s_events)
		{
			if (evt.name == name)
			{
				evt.enabled = (enabled != 0);
				break;
			}
		}
	}
	sqlite3_finalize(stmt);
}

// Persist enabled state to SQLite camp_events table.
static void SaveEnabledStateToDB(sqlite3* db, const std::string& name, bool enabled)
{
	if (!db) return;

	const char* sql = "UPDATE camp_events SET enabled = ? WHERE event_name = ?";
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK)
		return;

	sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
	sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void RegisterEventBindings(sol::table& core, sqlite3* db)
{
	sol::table event = core["Event"].get_or_create<sol::table>();

	// Core.Event.Register(name, pattern, callback)
	event.set_function("Register", [](const std::string& name, const std::string& pattern, sol::function callback) {
		// Dedup: replace existing event with same name
		for (auto& evt : s_events)
		{
			if (evt.name == name)
			{
				evt.pattern = pattern;
				evt.callback = callback;
				evt.enabled = true;
				return;
			}
		}

		// New event
		s_events.push_back({ name, pattern, callback, true });
	});

	// Core.Event.Unregister(name)
	event.set_function("Unregister", [](const std::string& name) {
		auto it = std::remove_if(s_events.begin(), s_events.end(),
			[&name](const EventPattern& evt) { return evt.name == name; });
		s_events.erase(it, s_events.end());
	});

	// Core.Event.SetEnabled(name, enabled)
	event.set_function("SetEnabled", [db](const std::string& name, bool enabled) {
		for (auto& evt : s_events)
		{
			if (evt.name == name)
			{
				evt.enabled = enabled;
				SaveEnabledStateToDB(db, name, enabled);
				return;
			}
		}
	});

	// Core.Event.IsEnabled(name) -> bool
	event.set_function("IsEnabled", [](const std::string& name) -> bool {
		for (const auto& evt : s_events)
		{
			if (evt.name == name)
				return evt.enabled;
		}
		return false;
	});

	// Core.Event.Fire(name, data)
	event.set_function("Fire", [](const std::string& name, const std::string& data) {
		for (auto& evt : s_events)
		{
			if (evt.name == name)
			{
				if (evt.callback.valid())
				{
					auto result = evt.callback(data);
					if (!result.valid())
					{
						sol::error err = result;
						WriteChatf("\ar[MQ2CF]\ax Event '%s' error: %s", evt.name.c_str(), err.what());
					}
				}
				return;
			}
		}
	});

	// Load persisted enabled state from DB after bindings are set up
	LoadEnabledStateFromDB(db);
}

void ProcessChatLine(sol::state* lua, const char* line)
{
	if (!line || !lua) return;

	for (auto& evt : s_events)
	{
		if (!evt.enabled) continue;
		if (evt.pattern.empty()) continue;

		if (strstr(line, evt.pattern.c_str()) != nullptr)
		{
			if (evt.callback.valid())
			{
				auto result = evt.callback(std::string(line));
				if (!result.valid())
				{
					sol::error err = result;
					WriteChatf("\ar[MQ2CF]\ax Event '%s' error: %s", evt.name.c_str(), err.what());
				}
			}
		}
	}
}

void ClearEventRegistrations()
{
	s_events.clear();
}

} // namespace CF
