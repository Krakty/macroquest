/*
 * MQ2CF: MacroQuest Class Framework
 * SchemaInit -- creates additional per-character SQLite tables
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once
#include <sqlite3.h>

namespace CF {
void InitGemDB(sqlite3* db);
void InitCampEventsDB(sqlite3* db);
void InitZoneDefaultsDB(sqlite3* db);
} // namespace CF
