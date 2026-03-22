/*
 * MQ2CF: MacroQuest Class Framework
 * Group engine bindings -- exposes group/raid/xtarget data to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "GroupEngine.h"

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static CGroup* GetGroup()
{
	if (pLocalPC)
		return pLocalPC->Group;
	return nullptr;
}

// ---------------------------------------------------------------------------
// Core.Group bindings
// ---------------------------------------------------------------------------

void RegisterGroupBindings(sol::table& core)
{
	sol::table group = core["Group"].get_or_create<sol::table>();

	// -- Group membership --------------------------------------------------

	group.set_function("GetMemberCount", []() -> int {
		CGroup* pGroup = GetGroup();
		if (!pGroup)
			return 0;
		return static_cast<int>(pGroup->GetNumberOfMembers(false));
	});

	group.set_function("GetMemberSpawnID", [](int slot) -> int {
		CGroup* pGroup = GetGroup();
		if (!pGroup)
			return 0;
		if (slot < 0 || slot >= MAX_GROUP_SIZE)
			return 0;
		CGroupMember* pMember = pGroup->GetGroupMember(slot);
		if (!pMember || !pMember->pSpawn)
			return 0;
		return static_cast<int>(pMember->pSpawn->SpawnID);
	});

	group.set_function("GetMemberHP", [](int slot) -> float {
		CGroup* pGroup = GetGroup();
		if (!pGroup)
			return 0.0f;
		if (slot < 0 || slot >= MAX_GROUP_SIZE)
			return 0.0f;
		CGroupMember* pMember = pGroup->GetGroupMember(slot);
		if (!pMember || !pMember->pSpawn)
			return 0.0f;
		int64_t maxHp = pMember->pSpawn->HPMax;
		if (maxHp <= 0)
			return 0.0f;
		return static_cast<float>(pMember->pSpawn->HPCurrent) / static_cast<float>(maxHp) * 100.0f;
	});

	// -- Roles -------------------------------------------------------------

	group.set_function("GetMainAssistSpawnID", []() -> int {
		CGroup* pGroup = GetGroup();
		if (!pGroup)
			return 0;
		CGroupMember* pMA = pGroup->GetGroupMemberByRole(GroupRoleAssist);
		if (!pMA || !pMA->pSpawn)
			return 0;
		return static_cast<int>(pMA->pSpawn->SpawnID);
	});

	group.set_function("IsMainAssist", [](int spawnId) -> bool {
		CGroup* pGroup = GetGroup();
		if (!pGroup || spawnId <= 0)
			return false;
		for (int i = 0; i < MAX_GROUP_SIZE; ++i)
		{
			CGroupMember* pMember = pGroup->GetGroupMember(i);
			if (!pMember || !pMember->pSpawn)
				continue;
			if (static_cast<int>(pMember->pSpawn->SpawnID) == spawnId)
				return pMember->IsMainAssist();
		}
		return false;
	});

	// -- Mob count (hostile on xtarget) ------------------------------------

	group.set_function("GetMobCount", []() -> int {
		if (!pLocalPC || !pLocalPC->pExtendedTargetList)
			return 0;
		int count = 0;
		int numSlots = pLocalPC->pExtendedTargetList->GetNumSlots();
		for (int i = 0; i < numSlots; ++i)
		{
			ExtendedTargetSlot* pSlot = pLocalPC->pExtendedTargetList->GetSlot(i);
			if (!pSlot || pSlot->SpawnID == 0)
				continue;
			if (pSlot->xTargetType == 2) // Auto Hater
			{
				PlayerClient* pSpawn = GetSpawnByID(pSlot->SpawnID);
				if (pSpawn)
					++count;
			}
		}
		return count;
	});

	// -- XTarget -----------------------------------------------------------

	group.set_function("GetXTargetCount", []() -> int {
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

	group.set_function("GetXTargetSpawnID", [](int index) -> int {
		if (!pLocalPC || !pLocalPC->pExtendedTargetList)
			return 0;
		if (index < 0 || index >= pLocalPC->pExtendedTargetList->GetNumSlots())
			return 0;
		ExtendedTargetSlot* pSlot = pLocalPC->pExtendedTargetList->GetSlot(index);
		if (!pSlot)
			return 0;
		return static_cast<int>(pSlot->SpawnID);
	});

	// -- Raid assist -------------------------------------------------------

	group.set_function("GetRaidAssistTargetID", [](int slot) -> int {
		if (!pLocalPlayer)
			return 0;
		if (slot < 0 || slot >= MAX_RAID_ASSISTS)
			return 0;
		return static_cast<int>(pLocalPlayer->RaidAssistNPC[slot]);
	});
}

} // namespace CF
