/*
 * MQ2CF: MacroQuest Class Framework
 * Local player bindings -- exposes PcClient/CharacterZoneClient data to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "LocalPlayer.h"

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static PcProfile* GetLocalProfile()
{
	if (pLocalPC)
		return pLocalPC->GetCurrentPcProfile();
	return nullptr;
}

static CGroup* GetGroup()
{
	if (pLocalPC)
		return pLocalPC->Group;
	return nullptr;
}

// ---------------------------------------------------------------------------
// Core.LocalPlayer bindings
// ---------------------------------------------------------------------------

void RegisterLocalPlayerBindings(sol::table& core)
{
	sol::table localPlayer = core["LocalPlayer"].get_or_create<sol::table>();

	// -- Identity ----------------------------------------------------------

	localPlayer.set_function("GetSpawnID", []() -> int {
		if (pLocalPlayer)
			return static_cast<int>(pLocalPlayer->SpawnID);
		return 0;
	});

	// -- Mana / Endurance --------------------------------------------------

	localPlayer.set_function("GetMana", []() -> int {
		if (pLocalPC)
			return pLocalPC->ManaCurrent;
		return 0;
	});

	localPlayer.set_function("GetManaMax", []() -> int {
		if (pLocalPC)
			return pLocalPC->ManaMax;
		return 0;
	});

	localPlayer.set_function("GetEndurance", []() -> int {
		if (pLocalPC)
			return pLocalPC->EnduranceCurrent;
		return 0;
	});

	localPlayer.set_function("GetEnduranceMax", []() -> int {
		if (pLocalPC)
			return static_cast<int>(pLocalPC->EnduranceMax);
		return 0;
	});

	// -- Spell gems --------------------------------------------------------

	localPlayer.set_function("GetGemSpellID", [](int gemSlot) -> int {
		if (gemSlot < 0 || gemSlot > 12)
			return -1;
		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return -1;
		return pProfile->GetMemorizedSpell(gemSlot);
	});

	localPlayer.set_function("IsGemReady", [](int gemSlot) -> bool {
		if (gemSlot < 0 || gemSlot > 12)
			return false;
		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return false;
		return pProfile->GetSpellGemTimer(gemSlot) == 0;
	});

	// -- Casting -----------------------------------------------------------

	localPlayer.set_function("GetCastRecoveryETA", []() -> int {
		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return 0;
		return static_cast<int>(pProfile->GetCastRecoveryTimer());
	});

	localPlayer.set_function("IsInCastRecovery", []() -> bool {
		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return false;
		return pProfile->GetCastRecoveryTimer() > 0;
	});

	// -- Group assist ------------------------------------------------------

	localPlayer.set_function("GetMASpawnID", []() -> int {
		CGroup* pGroup = GetGroup();
		if (!pGroup)
			return 0;
		CGroupMember* pMA = pGroup->GetGroupMemberByRole(GroupRoleAssist);
		if (!pMA || !pMA->pSpawn)
			return 0;
		return static_cast<int>(pMA->pSpawn->SpawnID);
	});

	// -- XTarget -----------------------------------------------------------

	localPlayer.set_function("GetXTargetCount", []() -> int {
		if (!pLocalPC || !pLocalPC->pExtendedTargetList)
			return 0;
		int count = 0;
		int numSlots = pLocalPC->pExtendedTargetList->GetNumSlots();
		for (int i = 0; i < numSlots; ++i)
		{
			ExtendedTargetSlot* pSlot = pLocalPC->pExtendedTargetList->GetSlot(i);
			if (pSlot && pSlot->SpawnID != 0)
				++count;
		}
		return count;
	});

	localPlayer.set_function("GetXTargetSpawnID", [](int index) -> int {
		if (!pLocalPC || !pLocalPC->pExtendedTargetList)
			return 0;
		if (index < 0 || index >= pLocalPC->pExtendedTargetList->GetNumSlots())
			return 0;
		ExtendedTargetSlot* pSlot = pLocalPC->pExtendedTargetList->GetSlot(index);
		if (!pSlot)
			return 0;
		return static_cast<int>(pSlot->SpawnID);
	});

	// -- State -------------------------------------------------------------

	localPlayer.set_function("IsInvis", []() -> bool {
		if (pLocalPlayer)
			return pLocalPlayer->HideMode != 0;
		return false;
	});
}

} // namespace CF
