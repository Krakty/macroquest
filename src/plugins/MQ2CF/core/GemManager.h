/*
 * MQ2CF: MacroQuest Class Framework
 * Gem manager -- spell gem slot querying and memorization management
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>
#include <sqlite3.h>

namespace CF {

void RegisterGemBindings(sol::table& core, sqlite3* db);

} // namespace CF
