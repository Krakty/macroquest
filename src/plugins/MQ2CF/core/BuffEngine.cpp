/*
 * MQ2CF: MacroQuest Class Framework
 * Buff engine bindings -- exposes buff/debuff checking to Lua scripts
 *
 * Local player buff/debuff access via PcProfile.
 * Long buffs (0..NUM_LONG_BUFFS-1) = persistent buffs.
 * Short/temp buffs (NUM_LONG_BUFFS..MAX_TOTAL_BUFFS-1) = short duration / debuffs.
 *
 * For non-local spawns, buff visibility is limited by the EQ client.
 * Those functions return safe defaults with a TODO for future implementation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "BuffEngine.h"

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static PcProfile* GetLocalProfile()
{
	if (!pLocalPC)
		return nullptr;
	return pLocalPC->GetCurrentPcProfile();
}

static bool IsLocalPlayer(int spawnId)
{
	if (!pLocalPlayer)
		return false;
	return static_cast<int>(pLocalPlayer->SpawnID) == spawnId;
}

// ---------------------------------------------------------------------------
// Core.Buff bindings
// ---------------------------------------------------------------------------

void RegisterBuffBindings(sol::table& core)
{
	sol::table buff = core["Buff"].get_or_create<sol::table>();

	// -- HasBuff(spawnId, spellId) -> bool --------------------------------
	// Check if a specific spell ID is active as a buff on the target spawn.
	// Currently only supports local player; returns false for other spawns.

	buff.set_function("HasBuff", [](int spawnId, int spellId) -> bool {
		if (spellId <= 0)
			return false;

		if (!IsLocalPlayer(spawnId))
		{
			// TODO: Implement buff checking for non-local spawns.
			// Options: CBuffWindow inspection, tracked cast history, or
			// group member buff data if available via eqlib.
			return false;
		}

		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return false;

		int maxEffects = pProfile->GetMaxEffects();
		for (int i = 0; i < maxEffects; ++i)
		{
			const EQ_Affect& affect = pProfile->GetEffect(i);
			if (affect.SpellID == spellId)
				return true;
		}
		return false;
	});

	// -- HasBuffByGroup(spawnId, spellGroup) -> bool ----------------------
	// Check if any spell from the given spell group is active on the target.
	// Currently only supports local player; returns false for other spawns.

	buff.set_function("HasBuffByGroup", [](int spawnId, int spellGroup) -> bool {
		if (spellGroup <= 0)
			return false;

		if (!IsLocalPlayer(spawnId))
		{
			// TODO: Implement buff-by-group checking for non-local spawns.
			return false;
		}

		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return false;

		int maxEffects = pProfile->GetMaxEffects();
		for (int i = 0; i < maxEffects; ++i)
		{
			const EQ_Affect& affect = pProfile->GetEffect(i);
			if (affect.SpellID <= 0)
				continue;

			EQ_Spell* pSpell = GetSpellByID(affect.SpellID);
			if (pSpell && pSpell->SpellGroup == spellGroup)
				return true;
		}
		return false;
	});

	// -- GetBuffCount(spawnId) -> int ------------------------------------
	// Count of active buffs (long duration) on the spawn.
	// Currently only supports local player; returns 0 for other spawns.

	buff.set_function("GetBuffCount", [](int spawnId) -> int {
		if (!IsLocalPlayer(spawnId))
		{
			// TODO: Implement buff count for non-local spawns.
			return 0;
		}

		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return 0;

		int count = 0;
		for (int i = 0; i < NUM_LONG_BUFFS; ++i)
		{
			const EQ_Affect& affect = pProfile->GetEffect(i);
			if (affect.SpellID > 0)
				++count;
		}
		return count;
	});

	// -- GetBuffInSlot(spawnId, slot) -> spellId -------------------------
	// Return the spell ID in a specific buff slot (0-based).
	// Returns 0 if slot is empty or out of range.
	// Currently only supports local player; returns 0 for other spawns.

	buff.set_function("GetBuffInSlot", [](int spawnId, int slot) -> int {
		if (slot < 0)
			return 0;

		if (!IsLocalPlayer(spawnId))
		{
			// TODO: Implement slot access for non-local spawns.
			return 0;
		}

		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return 0;

		if (slot >= pProfile->GetMaxEffects())
			return 0;

		const EQ_Affect& affect = pProfile->GetEffect(static_cast<uint32_t>(slot));
		if (affect.SpellID <= 0)
			return 0;
		return affect.SpellID;
	});

	// -- HasDebuff(spellId) -> bool --------------------------------------
	// Check if the local player has a specific debuff (short buff).

	buff.set_function("HasDebuff", [](int spellId) -> bool {
		if (spellId <= 0)
			return false;

		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return false;

		for (int i = 0; i < NUM_SHORT_BUFFS; ++i)
		{
			const EQ_Affect& affect = pProfile->GetTempEffect(i);
			if (affect.SpellID == spellId)
				return true;
		}
		return false;
	});

	// -- GetDebuffCount() -> int -----------------------------------------
	// Count of active debuffs (short/temp buffs) on the local player.

	buff.set_function("GetDebuffCount", []() -> int {
		PcProfile* pProfile = GetLocalProfile();
		if (!pProfile)
			return 0;

		int count = 0;
		for (int i = 0; i < NUM_SHORT_BUFFS; ++i)
		{
			const EQ_Affect& affect = pProfile->GetTempEffect(i);
			if (affect.SpellID > 0)
				++count;
		}
		return count;
	});
}

} // namespace CF
