/*
 * MQ2CF: MacroQuest Class Framework
 * Lifecycle bindings -- pause/resume control and timing for Lua scripts
 *
 * Exposes:
 *   Core.Lifecycle.IsPaused()     -> bool
 *   Core.Lifecycle.Pause()        -> void
 *   Core.Lifecycle.Resume()       -> void
 *   Core.Lifecycle.GetTickCount() -> int  (game tick count for timing)
 *   Core.Lifecycle.GetTimestamp()  -> int  (milliseconds since epoch)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "Lifecycle.h"

#include <chrono>

namespace CF {

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------
static bool s_paused = false;

// ---------------------------------------------------------------------------
// Public API (C++)
// ---------------------------------------------------------------------------
bool IsPaused()
{
	return s_paused;
}

void SetPaused(bool paused)
{
	s_paused = paused;
}

// ---------------------------------------------------------------------------
// Core.Lifecycle bindings
// ---------------------------------------------------------------------------
void RegisterLifecycleBindings(sol::table& core)
{
	sol::table lifecycle = core["Lifecycle"].get_or_create<sol::table>();

	lifecycle.set_function("IsPaused", []() -> bool {
		return s_paused;
	});

	lifecycle.set_function("Pause", []() {
		s_paused = true;
	});

	lifecycle.set_function("Resume", []() {
		s_paused = false;
	});

	lifecycle.set_function("GetTickCount", []() -> int {
		// EQ's internal tick count, available via MQ's MQGetTickCount64
		return static_cast<int>(MQGetTickCount64());
	});

	lifecycle.set_function("GetTimestamp", []() -> int64_t {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch());
		return ms.count();
	});
}

} // namespace CF
