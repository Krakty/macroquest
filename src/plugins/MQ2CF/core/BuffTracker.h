/*
 * MQ2CF: MacroQuest Class Framework
 * BuffTracker -- writes CharName_Buffs.ini for cross-character buff visibility
 *
 * Replaces MQ2CWTNBuffs.dll. Scans local player buff slots, counts debuff SPAs,
 * and writes results to INI so other characters' class plugins can read them.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#pragma once

namespace CF {

// Call from OnPulse -- throttled internally (writes every ~3 seconds)
void BuffTracker_OnPulse();

// Call from OnZoned -- delays writes for 3 seconds after zone
void BuffTracker_OnZoned();

// Call from SetGameState -- clears INI sections on logout
void BuffTracker_OnSetGameState(int gameState);

} // namespace CF
