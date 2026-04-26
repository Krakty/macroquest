/*
 * MQ2CF: MacroQuest Class Framework
 * Event subsystem -- matches chat lines against registered patterns and dispatches callbacks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <sqlite3.h>

namespace CF {

// Register Core.Event.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterEventBindings(sol::table& core, sqlite3* db);

// Process an incoming chat line against all registered event patterns.
// Call from OnIncomingChat.
void ProcessChatLine(sol::state* lua, const char* line);

// Clear all registered events. Call before destroying the Lua state.
void ClearEventRegistrations();

} // namespace CF
