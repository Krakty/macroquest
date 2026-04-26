/*
 * MQ2CF: MacroQuest Class Framework
 * Camp subsystem bindings -- manages bot camp position with evac invalidation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// Register Core.Camp.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterCampBindings(sol::table& core);

} // namespace CF
