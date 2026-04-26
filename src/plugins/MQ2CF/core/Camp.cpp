/*
 * MQ2CF: MacroQuest Class Framework
 * Camp subsystem bindings -- manages bot camp position with evac invalidation
 *
 * Stores a "camp" position (X, Y, Z, zone ID) and provides distance checks,
 * leash validation, and automatic invalidation on zone change or evac.
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "Camp.h"

#include <cmath>

namespace CF {

// ---------------------------------------------------------------------------
// Camp state -- static module-level storage
// ---------------------------------------------------------------------------

static float s_campX = 0.0f;
static float s_campY = 0.0f;
static float s_campZ = 0.0f;
static int   s_campZoneId = 0;
static float s_maxLeashDistance = 10000.0f;
static bool  s_hasCamp = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float Distance3D(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float dz = z2 - z1;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

static int GetCurrentZoneId()
{
	if (pLocalPlayer)
		return pLocalPlayer->GetZoneID() & 0x7FFF;
	return 0;
}

// ---------------------------------------------------------------------------
// Core.Camp bindings
// ---------------------------------------------------------------------------

void RegisterCampBindings(sol::table& core)
{
	sol::table camp = core["Camp"].get_or_create<sol::table>();

	camp.set_function("SetCamp", []() -> bool {
		if (!pLocalPlayer)
			return false;
		s_campX = pLocalPlayer->X;
		s_campY = pLocalPlayer->Y;
		s_campZ = pLocalPlayer->Z;
		s_campZoneId = GetCurrentZoneId();
		s_hasCamp = true;
		return true;
	});

	camp.set_function("ClearCamp", []() {
		s_campX = 0.0f;
		s_campY = 0.0f;
		s_campZ = 0.0f;
		s_campZoneId = 0;
		s_hasCamp = false;
	});

	camp.set_function("HasCamp", []() -> bool {
		return s_hasCamp;
	});

	camp.set_function("GetCampPosition", []() -> std::tuple<float, float, float> {
		if (s_hasCamp)
			return std::make_tuple(s_campX, s_campY, s_campZ);
		return std::make_tuple(0.0f, 0.0f, 0.0f);
	});

	camp.set_function("GetDistanceToCamp", []() -> float {
		if (!s_hasCamp || !pLocalPlayer)
			return -1.0f;
		return Distance3D(
			pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z,
			s_campX, s_campY, s_campZ);
	});

	camp.set_function("IsCampValid", []() -> bool {
		if (!s_hasCamp)
			return false;
		// Zone mismatch -> invalidate
		int currentZone = GetCurrentZoneId();
		if (currentZone != s_campZoneId)
		{
			s_hasCamp = false;
			return false;
		}
		// Evac detection: distance exceeds leash
		if (!pLocalPlayer)
			return false;
		float dist = Distance3D(
			pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z,
			s_campX, s_campY, s_campZ);
		if (dist > s_maxLeashDistance)
		{
			s_hasCamp = false;
			return false;
		}
		return true;
	});

	camp.set_function("GetCampZoneID", []() -> int {
		if (s_hasCamp)
			return s_campZoneId;
		return 0;
	});

	camp.set_function("SetMaxLeashDistance", [](float dist) {
		if (dist > 0.0f)
			s_maxLeashDistance = dist;
	});

	camp.set_function("IsInCampRadius", [](float radius) -> bool {
		if (!s_hasCamp || !pLocalPlayer)
			return false;
		float dist = Distance3D(
			pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z,
			s_campX, s_campY, s_campZ);
		return dist <= radius;
	});

	camp.set_function("IsSpawnInRadius", [](int spawnId, float radius) -> bool {
		if (!s_hasCamp)
			return false;
		PlayerClient* pSpawn = GetSpawnByID(static_cast<DWORD>(spawnId));
		if (!pSpawn)
			return false;
		float dy = pSpawn->Y - s_campY;
		float dx = pSpawn->X - s_campX;
		float dz = pSpawn->Z - s_campZ;
		float dist = std::sqrt(dy * dy + dx * dx + dz * dz);
		return dist <= radius;
	});
}

} // namespace CF
