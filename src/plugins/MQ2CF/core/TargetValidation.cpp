/*
 * MQ2CF: MacroQuest Class Framework
 * Target validation bindings -- exposes target checks to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its spawn and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "TargetValidation.h"

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

// ---------------------------------------------------------------------------
// Core.Target bindings
// ---------------------------------------------------------------------------

void RegisterTargetBindings(sol::table& core)
{
	sol::table target = core["Target"].get_or_create<sol::table>();

	// -- Spell Validation --------------------------------------------------

	target.set_function("IsValidForSpell", [](int spellId, int targetSpawnId) -> bool {
		EQ_Spell* pSpell = FindSpell(spellId);
		PlayerClient* pTarget = FindSpawn(targetSpawnId);
		if (!pSpell || !pTarget)
			return false;

		// Check line of sight from local player to target
		if (pLocalPlayer && !pLocalPlayer->CanSee(*static_cast<PlayerBase*>(pTarget)))
			return false;

		// Check target type compatibility
		switch (pSpell->TargetType)
		{
		case 5:  // Single
			return true;

		case 41: // Undead
			// Target must be undead body type
			// TODO: BodyType removed from PlayerClient in current eqlib, needs alternative check
		return false;

		case 6:  // Self
			// Must be local player
			return (pLocalPlayer && pTarget->SpawnID == pLocalPlayer->SpawnID);

		case 14: // AE
			// Target must exist
			return true;

		case 1:  // Line of Sight
			// Already checked CanSee above
			return true;

		case 3:  // Group
			// Check if target is in group
			if (!pLocalPC || !pLocalPC->Group)
				return false;
			for (int i = 0; i < MAX_GROUP_SIZE; ++i)
			{
				CGroupMember* pMember = pLocalPC->Group->GetGroupMember(i);
				if (pMember && pMember->pSpawn
					&& pMember->pSpawn->SpawnID == pTarget->SpawnID)
					return true;
			}
			return false;

		default:
			// Unknown target type - assume valid if target exists
			return true;
		}
	});

	target.set_function("IsInRange", [](int spellId, int targetSpawnId) -> bool {
		EQ_Spell* pSpell = FindSpell(spellId);
		PlayerClient* pTarget = FindSpawn(targetSpawnId);
		if (!pSpell || !pTarget || !pLocalPlayer)
			return false;

		float distance = Distance3D(
			pLocalPlayer->Y, pLocalPlayer->X, pLocalPlayer->Z,
			pTarget->Y, pTarget->X, pTarget->Z);

		// Range of 0 means unlimited
		if (pSpell->Range == 0.0f)
			return true;

		return distance <= pSpell->Range;
	});

	target.set_function("IsInAERange", [](int spellId, int targetSpawnId) -> bool {
		EQ_Spell* pSpell = FindSpell(spellId);
		PlayerClient* pTarget = FindSpawn(targetSpawnId);
		if (!pSpell || !pTarget || !pLocalPlayer)
			return false;

		float distance = Distance3D(
			pLocalPlayer->Y, pLocalPlayer->X, pLocalPlayer->Z,
			pTarget->Y, pTarget->X, pTarget->Z);

		// AERange of 0 means unlimited
		if (pSpell->AERange == 0.0f)
			return true;

		return distance <= pSpell->AERange;
	});

	// -- Spawn Type Checks -------------------------------------------------

	target.set_function("IsPet", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		return pSpawn->MasterID != 0;
	});

	target.set_function("IsMerc", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		// Mercenary class ID is 0x11 (17)
		return (static_cast<int>(pSpawn->CharClass) == 0x11);
	});

	target.set_function("IsUndead", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		// TODO: BodyType removed from PlayerClient in current eqlib, needs alternative check
		return false;
	});
}

} // namespace CF
