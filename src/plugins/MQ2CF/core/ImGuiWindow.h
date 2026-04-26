/*
 * MQ2CF: MacroQuest Class Framework
 * ImGui settings window -- in-game UI for configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

#define SOL_LUAJIT 1
#include <sol/sol.hpp>

namespace CF {

void DrawSettingsWindow(sol::state* lua, bool luaReady,
	const char* dbPath, const char* charDbPath);
bool IsSettingsWindowOpen();
void ToggleSettingsWindow();

} // namespace CF
