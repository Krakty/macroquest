/*
 * MQ2CF: MacroQuest Class Framework
 * AA engine bindings -- exposes alternate ability activation and queries to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "AAEngine.h"

#include <string>

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Look up a CAltAbilityData by group ID using the manager's built-in lookup.
// Returns nullptr if not found or if required globals are unavailable.
static CAltAbilityData* FindAAByGroupId(int groupId)
{
	if (!pAltAdvManager || !pLocalPC || groupId <= 0)
		return nullptr;

	return pAltAdvManager->GetOwnedAbilityFromGroupID(pLocalPC, groupId);
}

// Check if the local character has purchased an AA by its index.
static bool IsAAPurchased(CAltAbilityData* pAbility)
{
	if (!pAbility || !pLocalPC)
		return false;

	return pLocalPC->HasAlternateAbility(pAbility->Index);
}

// Get the remaining cooldown for an AA in milliseconds.
// Returns 0 if ready or if data is unavailable.
static uint32_t GetAACooldownMs(CAltAbilityData* pAbility)
{
	if (!pAbility || !pAltAdvManager || !pLocalPC)
		return 0;

	int refresh = 0;
	int timer = 0;
	if (pAltAdvManager->IsAbilityReady(pLocalPC, pAbility, &refresh, &timer))
		return 0;

	// timer holds the remaining time in ms when not ready
	return static_cast<uint32_t>(timer > 0 ? timer : 0);
}

// ---------------------------------------------------------------------------
// Core.AA bindings
// ---------------------------------------------------------------------------

void RegisterAABindings(sol::table& core)
{
	sol::table aa = core["AA"].get_or_create<sol::table>();

	// -- Core.AA.CanActivate(aaGroupId) -> bool, reason -----------------------

	aa.set_function("CanActivate", [](int aaGroupId) -> std::tuple<bool, std::string> {
		// a. Game state
		if (GetGameState() != GAMESTATE_INGAME)
			return std::make_tuple(false, "not_ingame");

		// b. Local player null
		if (!pLocalPlayer)
			return std::make_tuple(false, "no_player");

		// c. Player dead
		if (pLocalPlayer->HPCurrent <= 0)
			return std::make_tuple(false, "dead");

		// d. AA manager available
		if (!pAltAdvManager || !pLocalPC)
			return std::make_tuple(false, "no_aa_manager");

		// e. Find the AA
		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return std::make_tuple(false, "not_found");

		// f. Must be purchased
		if (!IsAAPurchased(pAbility))
			return std::make_tuple(false, "not_purchased");

		// g. Cooldown check
		uint32_t cooldownMs = GetAACooldownMs(pAbility);
		if (cooldownMs > 0)
			return std::make_tuple(false, "cooldown");

		// h. Mana/endurance check via associated spell
		if (pAbility->SpellID > 0 && pLocalPlayer)
		{
			EQ_Spell* pSpell = GetSpellByID(pAbility->SpellID);
			if (pSpell)
			{
				if (pSpell->ManaCost > 0 && pSpell->ManaCost > pLocalPlayer->GetCurrentMana())
					return std::make_tuple(false, "mana");

				if (pSpell->EnduranceCost > 0 && pSpell->EnduranceCost > pLocalPlayer->GetCurrentEndurance())
					return std::make_tuple(false, "endurance");
			}
		}

		return std::make_tuple(true, std::string(""));
	});

	// -- Core.AA.Activate(aaGroupId) -> bool, reason --------------------------

	aa.set_function("Activate", [](int aaGroupId) -> std::tuple<bool, std::string> {
		// a. Game state
		if (GetGameState() != GAMESTATE_INGAME)
			return std::make_tuple(false, "not_ingame");

		// b. Local player null
		if (!pLocalPlayer)
			return std::make_tuple(false, "no_player");

		// c. Player dead
		if (pLocalPlayer->HPCurrent <= 0)
			return std::make_tuple(false, "dead");

		// d. AA manager available
		if (!pAltAdvManager || !pLocalPC)
			return std::make_tuple(false, "no_aa_manager");

		// e. Find the AA
		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return std::make_tuple(false, "not_found");

		// f. Must be purchased
		if (!IsAAPurchased(pAbility))
			return std::make_tuple(false, "not_purchased");

		// g. Cooldown check
		uint32_t cooldownMs = GetAACooldownMs(pAbility);
		if (cooldownMs > 0)
			return std::make_tuple(false, "cooldown");

		// h. Mana/endurance check via associated spell
		if (pAbility->SpellID > 0 && pLocalPlayer)
		{
			EQ_Spell* pSpell = GetSpellByID(pAbility->SpellID);
			if (pSpell)
			{
				if (pSpell->ManaCost > 0 && pSpell->ManaCost > pLocalPlayer->GetCurrentMana())
					return std::make_tuple(false, "mana");

				if (pSpell->EnduranceCost > 0 && pSpell->EnduranceCost > pLocalPlayer->GetCurrentEndurance())
					return std::make_tuple(false, "endurance");
			}
		}

		// i. Issue the command
		char command[64];
		sprintf_s(command, "/alt activate %d", aaGroupId);
		EzCommand(command);
		return std::make_tuple(true, std::string(""));
	});

	// -- Core.AA.IsReady(aaGroupId) -> bool -----------------------------------

	aa.set_function("IsReady", [](int aaGroupId) -> bool {
		if (!pAltAdvManager || !pLocalPC)
			return false;

		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return false;

		if (!IsAAPurchased(pAbility))
			return false;

		return GetAACooldownMs(pAbility) == 0;
	});

	// -- Core.AA.GetReuseTimer(aaGroupId) -> number (seconds) -----------------

	aa.set_function("GetReuseTimer", [](int aaGroupId) -> double {
		if (!pAltAdvManager || !pLocalPC)
			return 0.0;

		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return 0.0;

		uint32_t cooldownMs = GetAACooldownMs(pAbility);
		return static_cast<double>(cooldownMs) / 1000.0;
	});

	// -- Core.AA.GetAAName(aaGroupId) -> string -------------------------------

	aa.set_function("GetAAName", [](int aaGroupId) -> std::string {
		if (!pAltAdvManager || !pLocalPC)
			return "";

		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return "";

		const char* name = pAbility->GetNameString();
		if (!name || !name[0])
			return "";

		return std::string(name);
	});

	// -- Core.AA.GetAASpellId(aaGroupId) -> int -------------------------------

	aa.set_function("GetAASpellId", [](int aaGroupId) -> int {
		if (!pAltAdvManager || !pLocalPC)
			return 0;

		CAltAbilityData* pAbility = FindAAByGroupId(aaGroupId);
		if (!pAbility)
			return 0;

		return pAbility->SpellID > 0 ? pAbility->SpellID : 0;
	});
}

} // namespace CF
