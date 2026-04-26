/*
 * MQ2CF: MacroQuest Class Framework
 * Target validation bindings -- exposes target checks to Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// Register Core.Target.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterTargetBindings(sol::table& core);

} // namespace CF
