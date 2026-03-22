/*
 * MQ2CF: MacroQuest Class Framework
 * Spawn primitive bindings -- exposes spawn data access to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its spawn and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "SpawnPrimitives.h"

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

// ---------------------------------------------------------------------------
// Core.Spawn bindings
// ---------------------------------------------------------------------------

void RegisterSpawnBindings(sol::table& core)
{
	sol::table spawn = core["Spawn"].get_or_create<sol::table>();

	// -- Identity ----------------------------------------------------------

	spawn.set_function("GetName", [](int spawnId) -> std::string {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->Name;
		return "";
	});

	spawn.set_function("GetDisplayedName", [](int spawnId) -> std::string {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->DisplayedName;
		return "";
	});

	spawn.set_function("GetSpawnID", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->SpawnID);
		return 0;
	});

	spawn.set_function("GetLocalName", []() -> std::string {
		if (pLocalPlayer)
			return pLocalPlayer->Name;
		return "";
	});

	spawn.set_function("GetLocalId", []() -> int {
		if (pLocalPlayer)
			return static_cast<int>(pLocalPlayer->SpawnID);
		return 0;
	});

	// -- Health ------------------------------------------------------------

	spawn.set_function("GetHP", [](int spawnId) -> float {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return 0.0f;
		int64_t maxHp = pSpawn->HPMax;
		if (maxHp <= 0)
			return 0.0f;
		return static_cast<float>(pSpawn->HPCurrent) / static_cast<float>(maxHp) * 100.0f;
	});

	spawn.set_function("GetHPCurrent", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->HPCurrent);
		return 0;
	});

	spawn.set_function("GetHPMax", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->HPMax);
		return 0;
	});

	// -- Mana / Endurance --------------------------------------------------

	spawn.set_function("GetMana", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->ManaCurrent;
		return 0;
	});

	spawn.set_function("GetManaMax", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->ManaMax;
		return 0;
	});

	spawn.set_function("GetEndurance", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->EnduranceCurrent;
		return 0;
	});

	spawn.set_function("GetEnduranceMax", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->EnduranceMax);
		return 0;
	});

	// -- Class / Level -----------------------------------------------------

	spawn.set_function("GetLevel", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->Level);
		return 0;
	});

	spawn.set_function("GetClass", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->CharClass);
		return 0;
	});

	// -- Position / Movement -----------------------------------------------

	spawn.set_function("GetPosition", [](int spawnId) -> std::tuple<float, float, float> {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return std::make_tuple(pSpawn->Y, pSpawn->X, pSpawn->Z);
		return std::make_tuple(0.0f, 0.0f, 0.0f);
	});

	spawn.set_function("IsMoving", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		return pSpawn->SpeedY != 0.0f
			|| pSpawn->SpeedX != 0.0f
			|| pSpawn->SpeedZ != 0.0f;
	});

	// -- State -------------------------------------------------------------

	spawn.set_function("IsDead", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		// StandState 0 = standing ... dead state is typically indicated by
		// the DEAD stand state value (0x08) or HP <= 0.
		if (pSpawn->HPCurrent <= 0)
			return true;
		if (pSpawn->StandState == STANDSTATE_DEAD)
			return true;
		return false;
	});

	spawn.set_function("GetStandState", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->StandState);
		return 0;
	});

	// -- Pet / Master ------------------------------------------------------

	spawn.set_function("GetPetID", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return pSpawn->PetID;
		return 0;
	});

	spawn.set_function("GetMasterID", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (pSpawn)
			return static_cast<int>(pSpawn->MasterID);
		return 0;
	});

	// -- Casting -----------------------------------------------------------

	spawn.set_function("IsCasting", [](int spawnId) -> bool {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return false;
		return pSpawn->CastingData.IsCasting();
	});

	spawn.set_function("GetCastingSpellID", [](int spawnId) -> int {
		PlayerClient* pSpawn = FindSpawn(spawnId);
		if (!pSpawn)
			return -1;
		return pSpawn->CastingData.SpellID;
	});

	// -- Distance / LOS ----------------------------------------------------

	spawn.set_function("GetDistance", [](int fromId, int toId) -> float {
		PlayerClient* pFrom = FindSpawn(fromId);
		PlayerClient* pTo = FindSpawn(toId);
		if (!pFrom || !pTo)
			return 0.0f;
		return Distance3D(
			pFrom->Y, pFrom->X, pFrom->Z,
			pTo->Y, pTo->X, pTo->Z);
	});

	spawn.set_function("CanSee", [](int observerId, int targetId) -> bool {
		PlayerClient* pObserver = FindSpawn(observerId);
		PlayerClient* pTarget = FindSpawn(targetId);
		if (!pObserver || !pTarget)
			return false;
		return pObserver->CanSee(*static_cast<PlayerBase*>(pTarget));
	});
}

} // namespace CF
