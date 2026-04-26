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

// ---------------------------------------------------------------------------
// Pending cast state (two-phase target-then-cast)
// ---------------------------------------------------------------------------
static int s_pendingSpellId = 0;
static int s_pendingTargetId = 0;
static int s_pendingGemSlot = -1;
static uint64_t s_pendingTargetTime = 0;
static constexpr uint64_t kTargetSettleMs = 50; // wait 50ms after target switch

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

	cast.set_function("CanCast", [](int spellId, int targetSpawnId) -> std::tuple<bool, std::string> {
		return DoValidateCast(spellId, targetSpawnId);
	});

	// -- Casting ------------------------------------------------------------

	cast.set_function("Cast", [](int spellId, int targetSpawnId) -> std::tuple<bool, std::string> {
		auto [ok, reason] = DoValidateCast(spellId, targetSpawnId);
		if (!ok)
			return std::make_tuple(false, reason);

		int gemSlot = FindGemSlot(spellId);
		if (gemSlot == -1)
			return std::make_tuple(false, std::string("not_memorized"));

		// Check if we need to switch targets (non-self spells only)
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell && pSpell->TargetType != 6 && targetSpawnId > 0)
		{
			bool needSwitch = true;
			if (pTarget && pTarget->SpawnID == static_cast<DWORD>(targetSpawnId))
				needSwitch = false;

			if (needSwitch)
			{
				// Issue target switch, store pending cast for next pulse
				char targetCmd[64];
				sprintf_s(targetCmd, "/target id %d", targetSpawnId);
				EzCommand(targetCmd);
				s_pendingSpellId = spellId;
				s_pendingTargetId = targetSpawnId;
				s_pendingGemSlot = gemSlot;
				s_pendingTargetTime = MQGetTickCount64();
				return std::make_tuple(true, std::string("targeting"));
			}
		}

		// Target already correct (or self-cast), cast immediately
		int gemNumber = gemSlot + 1;
		char command[32];
		sprintf_s(command, "/cast %d", gemNumber);
		EzCommand(command);
		return std::make_tuple(true, std::string(""));
	});

	// Complete a pending cast after target switch has settled
	cast.set_function("CompletePendingCast", []() -> std::tuple<bool, std::string> {
		if (s_pendingSpellId == 0)
			return std::make_tuple(false, std::string("no_pending"));

		// Wait for target settle time
		if (MQGetTickCount64() - s_pendingTargetTime < kTargetSettleMs)
			return std::make_tuple(false, std::string("waiting"));

		// Verify target switched correctly
		if (!pTarget || pTarget->SpawnID != static_cast<DWORD>(s_pendingTargetId))
		{
			// Target switch failed, clear pending
			s_pendingSpellId = 0;
			s_pendingTargetId = 0;
			s_pendingGemSlot = -1;
			return std::make_tuple(false, std::string("target_mismatch"));
		}

		// Cast the spell
		int gemNumber = s_pendingGemSlot + 1;
		char command[32];
		sprintf_s(command, "/cast %d", gemNumber);
		EzCommand(command);

		// Clear pending state
		s_pendingSpellId = 0;
		s_pendingTargetId = 0;
		s_pendingGemSlot = -1;
		return std::make_tuple(true, std::string(""));
	});

	// Check if there's a pending cast waiting
	cast.set_function("HasPendingCast", []() -> bool {
		return s_pendingSpellId != 0;
	});

	// Cancel a pending cast
	cast.set_function("ClearPendingCast", []() {
		s_pendingSpellId = 0;
		s_pendingTargetId = 0;
		s_pendingGemSlot = -1;
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
