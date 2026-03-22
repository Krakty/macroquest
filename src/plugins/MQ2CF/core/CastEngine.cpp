/*
 * MQ2CF: MacroQuest Class Framework
 * Cast engine bindings -- exposes spell casting validation and dispatch to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "CastEngine.h"

#include <cmath>

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static PlayerClient* FindSpawn(int spawnId)
{
	if (spawnId <= 0)
		return nullptr;
	return GetSpawnByID(static_cast<DWORD>(spawnId));
}

static float Distance3D(float y1, float x1, float z1, float y2, float x2, float z2)
{
	float dy = y2 - y1;
	float dx = x2 - x1;
	float dz = z2 - z1;
	return std::sqrt(dy * dy + dx * dx + dz * dz);
}

static EQ_Spell* FindSpell(int spellId)
{
	if (!pSpellMgr || spellId <= 0 || spellId >= pSpellMgr->GetMaxSpellID())
		return nullptr;
	return pSpellMgr->GetSpellByID(spellId);
}

static int FindGemSlot(int spellId)
{
	if (!pLocalPC || spellId <= 0)
		return -1;
	PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
	if (!pProfile)
		return -1;
	for (int i = 0; i <= 12; ++i)
	{
		if (pProfile->GetMemorizedSpell(i) == spellId)
			return i;
	}
	return -1;
}

// Internal validation -- returns (pass, reason) without going through sol
static std::tuple<bool, std::string> DoValidateCast(int spellId, int targetSpawnId)
{
	// a. Game state
	if (GetGameState() != GAMESTATE_INGAME)
		return std::make_tuple(false, "not_ingame");

	// b. Local player null
	if (!pLocalPlayer)
		return std::make_tuple(false, "no_player");

	// c. Player dead
	if (pLocalPlayer->HPCurrent <= 0)
		return std::make_tuple(false, "dead");

	// d. Player stunned
	if (pLocalPC && pLocalPC->Stunned != 0)
		return std::make_tuple(false, "stunned");

	// e. Currently casting
	if (pLocalPlayer->CastingData.IsCasting())
		return std::make_tuple(false, "casting");

	// f. Spell not found
	EQ_Spell* pSpell = FindSpell(spellId);
	if (!pSpell)
		return std::make_tuple(false, "bad_spell");

	// g. Level check
	if (pLocalPC)
	{
		int requiredLevel = pSpell->ClassLevel[pLocalPC->GetClass()];
		if (requiredLevel > pLocalPlayer->Level)
			return std::make_tuple(false, "level");
	}

	// h. Mana check
	if (pLocalPlayer && pSpell->ManaCost > pLocalPlayer->GetCurrentMana())
		return std::make_tuple(false, "mana");

	// i. Find gem slot
	int gemSlot = FindGemSlot(spellId);
	if (gemSlot == -1)
		return std::make_tuple(false, "not_memorized");

	// j. Gem ready
	PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
	if (pProfile)
	{
		if (pProfile->SpellRecastTimer[gemSlot] > 0)
			return std::make_tuple(false, "gem_cooldown");
	}

	// k. Range check
	if (pSpell->Range > 0)
	{
		PlayerClient* pTarget = FindSpawn(targetSpawnId);
		if (!pTarget)
			return std::make_tuple(false, "no_target");

		float distance = Distance3D(
			pLocalPlayer->Y, pLocalPlayer->X, pLocalPlayer->Z,
			pTarget->Y, pTarget->X, pTarget->Z);
		if (distance > pSpell->Range)
			return std::make_tuple(false, "range");
	}

	// l. Target valid
	if (pSpell->TargetType != 6) // Not Self
	{
		PlayerClient* pTarget = FindSpawn(targetSpawnId);
		if (!pTarget)
			return std::make_tuple(false, "no_target");
	}

	return std::make_tuple(true, "");
}

// ---------------------------------------------------------------------------
// Core.Cast bindings
// ---------------------------------------------------------------------------

void RegisterCastBindings(sol::table& core)
{
	sol::table cast = core["Cast"].get_or_create<sol::table>();

	// -- Validation ---------------------------------------------------------

	cast.set_function("ValidateCast", [](int spellId, int targetSpawnId) -> std::tuple<bool, std::string> {
		return DoValidateCast(spellId, targetSpawnId);
	});

	cast.set_function("CanCast", [](int spellId, int targetSpawnId) -> bool {
		auto [result, reason] = DoValidateCast(spellId, targetSpawnId);
		return result;
	});

	// -- Casting ------------------------------------------------------------

	cast.set_function("Cast", [](int spellId, int targetSpawnId) -> bool {
		auto [ok, reason] = DoValidateCast(spellId, targetSpawnId);
		if (!ok)
			return false;

		int gemSlot = FindGemSlot(spellId);
		if (gemSlot == -1)
			return false;

		// Convert to 1-indexed for /cast command
		int gemNumber = gemSlot + 1;
		char command[32];
		sprintf_s(command, "/cast %d", gemNumber);
		EzCommand(command);
		return true;
	});

	cast.set_function("StopCasting", []() {
		EzCommand("/stopcast");
	});

	// -- Recovery -----------------------------------------------------------

	cast.set_function("IsGlobalRecovery", []() -> bool {
		if (!pLocalPlayer)
			return false;
		return pLocalPlayer->CastingData.SpellETA > EQGetTime();
	});

	cast.set_function("GetGemReadyIn", [](int gemSlot) -> int {
		if (!pLocalPC)
			return 0;
		if (gemSlot < 0 || gemSlot > 12)
			return 0;
		PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
		if (!pProfile)
			return 0;
		return static_cast<int>(pProfile->SpellRecastTimer[gemSlot]);
	});
}

} // namespace CF
