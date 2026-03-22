/*
 * MQ2CF: MacroQuest Class Framework
 * Lua bindings -- exposes eqlib primitives to Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// Register all Core.* bindings on the given Lua state.
// Call once after creating the sol::state.
void RegisterLuaBindings(sol::state& lua);

} // namespace CF
