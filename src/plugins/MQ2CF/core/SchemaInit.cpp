/*
 * MQ2CF: MacroQuest Class Framework
 * SchemaInit -- creates additional per-character SQLite tables
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "SchemaInit.h"

namespace CF {

void InitGemDB(sqlite3* db)
{
	if (!db) return;
	const char* sql =
		"CREATE TABLE IF NOT EXISTS gem_loadouts ("
		"    class TEXT NOT NULL,"
		"    mode INTEGER NOT NULL,"
		"    slot INTEGER NOT NULL,"
		"    spell_group_id INTEGER NOT NULL,"
		"    PRIMARY KEY (class, mode, slot)"
		");";
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to create gem_loadouts table: %s", errMsg ? errMsg : "unknown");
		sqlite3_free(errMsg);
	}
}

void InitCampEventsDB(sqlite3* db)
{
	if (!db) return;
	const char* sql =
		"CREATE TABLE IF NOT EXISTS camp_events ("
		"    event_name TEXT PRIMARY KEY,"
		"    text_pattern TEXT NOT NULL,"
		"    response_type TEXT NOT NULL,"
		"    enabled INTEGER DEFAULT 1"
		");";
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to create camp_events table: %s", errMsg ? errMsg : "unknown");
		sqlite3_free(errMsg);
		return;
	}

	// Insert default event triggers (ignore if already exist)
	const char* defaults =
		"INSERT OR IGNORE INTO camp_events (event_name, text_pattern, response_type) VALUES"
		"('summon', 'has been summoned', 'run_to_camp'),"
		"('evac', 'begins to cast Evacuate', 'reset_camp'),"
		"('coth', 'Call of the Hero', 'reset_camp'),"
		"('zone_change', '', 'reset_camp'),"
		"('group_teleport', '', 'reset_camp');";
	sqlite3_exec(db, defaults, nullptr, nullptr, nullptr);
}

void InitZoneDefaultsDB(sqlite3* db)
{
	if (!db) return;
	const char* sql =
		"CREATE TABLE IF NOT EXISTS zone_mode_defaults ("
		"    zone_short_name TEXT PRIMARY KEY,"
		"    default_mode INTEGER NOT NULL"
		");";
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		WriteChatf("\ar[MQ2CF]\ax Failed to create zone_mode_defaults table: %s", errMsg ? errMsg : "unknown");
		sqlite3_free(errMsg);
		return;
	}

	// Insert hub zone defaults (mode 0 = Manual/NoCamp)
	const char* defaults =
		"INSERT OR IGNORE INTO zone_mode_defaults (zone_short_name, default_mode) VALUES"
		"('poknowledge', 0),"
		"('guildhall', 0),"
		"('bazaar', 0),"
		"('nexus', 0);";
	sqlite3_exec(db, defaults, nullptr, nullptr, nullptr);
}

} // namespace CF
