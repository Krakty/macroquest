/*
 * MQ2CF: MacroQuest Class Framework
 * Lifecycle bindings -- pause/resume control and timing for Lua scripts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// C++ accessors for pause state (used by ImGui window)
bool IsPaused();
void SetPaused(bool paused);

// Register Core.Lifecycle.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterLifecycleBindings(sol::table& core);

} // namespace CF
