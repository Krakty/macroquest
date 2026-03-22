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

#include "SpawnPrimitives.h"
#include "SpellData.h"
#include "GroupEngine.h"
#include "LocalPlayer.h"
#include "TargetValidation.h"
#include "CastEngine.h"

namespace CF {

// Register all Core.* bindings on the given Lua state.
// Call once after creating the sol::state.
void RegisterLuaBindings(sol::state& lua);

// Individual subsystem registrations -- called by RegisterLuaBindings.
// Each expects the Core table to already exist.
void RegisterSpawnBindings(sol::table& core);
void RegisterSpellBindings(sol::table& core);
void RegisterGroupBindings(sol::table& core);
void RegisterLocalPlayerBindings(sol::table& core);
void RegisterTargetBindings(sol::table& core);
void RegisterCastBindings(sol::table& core);

} // namespace CF
