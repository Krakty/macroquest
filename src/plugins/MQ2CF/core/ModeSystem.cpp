/*
 * MQ2CF: MacroQuest Class Framework
 * Mode subsystem -- manages operating mode (Manual, Assist, Tank, etc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "ModeSystem.h"

#include <string>

namespace CF {

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------
static CFMode s_currentMode = CFMode::Assist;

// ---------------------------------------------------------------------------
// Mode name lookup table
// ---------------------------------------------------------------------------
static const char* s_modeNames[] = {
	"Manual/NoCamp",   // 0
	"Assist/Camp",     // 1
	"Assist/Chase",    // 2
	"Assist/NoCamp",   // 3
	"Tank/Camp",       // 4
	"Puller Tank",     // 5
	"Puller Assist",   // 6
	"Manual Tank",     // 7
	"Roaming Tank",    // 8
};

static constexpr int kModeMin = 0;
static constexpr int kModeMax = 8;

// ---------------------------------------------------------------------------
// Public API (C++)
// ---------------------------------------------------------------------------
int GetCurrentMode()
{
	return static_cast<int>(s_currentMode);
}

bool IsModeAvailableForClass(int mode, int classId)
{
	if (mode < kModeMin || mode > kModeMax)
		return false;
	// Cleric (class 2): modes 0-3 only
	if (classId == 2)
		return mode <= 3;
	// Default: all modes available
	return true;
}

void SetCurrentMode(int mode)
{
	if (mode < kModeMin || mode > kModeMax)
	{
		WriteChatf("\ar[MQ2CF]\ax Invalid mode %d (valid range: %d-%d)", mode, kModeMin, kModeMax);
		return;
	}

	if (pLocalPlayer && !IsModeAvailableForClass(mode, pLocalPlayer->GetClass()))
	{
		WriteChatf("\ar[MQ2CF]\ax Mode %s not available for this class", s_modeNames[mode]);
		return;
	}

	int oldMode = static_cast<int>(s_currentMode);
	s_currentMode = static_cast<CFMode>(mode);
	WriteChatf("\ag[MQ2CF]\ax Mode changed: %s -> %s", s_modeNames[oldMode], s_modeNames[mode]);
}

const char* GetModeName(int mode)
{
	if (mode < kModeMin || mode > kModeMax)
		return "Unknown";

	return s_modeNames[mode];
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static bool IsTankFocused()
{
	switch (s_currentMode)
	{
	case CFMode::Tank:
	case CFMode::PullerTank:
	case CFMode::ManualTank:
	case CFMode::RoamingTank:
		return true;
	default:
		return false;
	}
}

static bool IsMovementMode()
{
	switch (s_currentMode)
	{
	case CFMode::ChaseAssist:
	case CFMode::PullerAssist:
	case CFMode::RoamingTank:
		return true;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Core.Mode bindings
// ---------------------------------------------------------------------------
void RegisterModeBindings(sol::table& core)
{
	sol::table mode = core["Mode"].get_or_create<sol::table>();

	// Core.Mode.GetMode() -> int
	mode.set_function("GetMode", []() -> int {
		return GetCurrentMode();
	});

	// Core.Mode.SetMode(mode) -> nil
	mode.set_function("SetMode", [](int m) {
		SetCurrentMode(m);
	});

	// Core.Mode.GetModeName(mode) -> string
	mode.set_function("GetModeName", [](int m) -> const char* {
		return GetModeName(m);
	});

	// Core.Mode.IsTankFocused() -> bool
	mode.set_function("IsTankFocused", []() -> bool {
		return IsTankFocused();
	});

	// Core.Mode.IsMovementMode() -> bool
	mode.set_function("IsMovementMode", []() -> bool {
		return IsMovementMode();
	});

	// Core.Mode.IsAvailable(mode) -> bool
	mode.set_function("IsAvailable", [](int m) -> bool {
		if (!pLocalPlayer) return false;
		return IsModeAvailableForClass(m, pLocalPlayer->GetClass());
	});
}

} // namespace CF
