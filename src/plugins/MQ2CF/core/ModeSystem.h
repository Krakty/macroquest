/*
 * MQ2CF: MacroQuest Class Framework
 * Mode subsystem -- manages operating mode (Manual, Assist, Tank, etc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

// Mode enum matching CWTN standard (0-8)
enum class CFMode : int {
	Manual = 0,       // Manual/NoCamp
	Assist = 1,       // Assist/Camp
	ChaseAssist = 2,  // Assist/Chase
	AssistNoCamp = 3, // Assist/NoCamp
	Tank = 4,         // Tank/Camp
	PullerTank = 5,   // Puller Tank
	PullerAssist = 6, // Puller Assist
	ManualTank = 7,   // Manual Tank
	RoamingTank = 8,  // Roaming Tank
};

int GetCurrentMode();
void SetCurrentMode(int mode);
const char* GetModeName(int mode);
bool IsModeAvailableForClass(int mode, int classId);

// Register Core.Mode.* bindings on the given Lua state.
// Expects Core table to already exist.
void RegisterModeBindings(sol::table& core);

} // namespace CF
