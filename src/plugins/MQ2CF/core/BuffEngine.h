/*
 * MQ2CF: MacroQuest Class Framework
 * Buff engine bindings -- exposes buff/debuff checking to Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// Register Core.Buff.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterBuffBindings(sol::table& core);

} // namespace CF
