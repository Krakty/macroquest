/*
 * MQ2CF: MacroQuest Class Framework
 * Spell data bindings -- exposes EQ_Spell fields to Lua scripts
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its spell and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "SpellData.h"

namespace CF {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static EQ_Spell* FindSpell(int spellId)
{
	if (!pSpellMgr || spellId <= 0 || spellId >= pSpellMgr->GetMaxSpellID())
		return nullptr;
	return pSpellMgr->GetSpellByID(spellId);
}

// ---------------------------------------------------------------------------
// Core.Spell bindings
// ---------------------------------------------------------------------------

void RegisterSpellBindings(sol::table& core)
{
	sol::table spell = core["Spell"].get_or_create<sol::table>();

	// -- Identity ----------------------------------------------------------

	spell.set_function("GetName", [](int spellId) -> std::string {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->Name;
		return "";
	});

	spell.set_function("GetNameByString", [](const std::string& name) -> int {
		EQ_Spell* pSpell = GetSpellByName(name);
		if (pSpell)
			return pSpell->ID;
		return 0;
	});

	// -- Cost / Range ------------------------------------------------------

	spell.set_function("GetManaCost", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->ManaCost;
		return 0;
	});

	spell.set_function("GetRange", [](int spellId) -> float {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->Range;
		return 0.0f;
	});

	spell.set_function("GetAERange", [](int spellId) -> float {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->AERange;
		return 0.0f;
	});

	// -- Target / Type -----------------------------------------------------

	spell.set_function("GetTargetType", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return static_cast<int>(pSpell->TargetType);
		return 0;
	});

	spell.set_function("GetSpellType", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return static_cast<int>(pSpell->SpellType);
		return 0;
	});

	// -- Category / Group --------------------------------------------------

	spell.set_function("GetCategory", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->Category;
		return 0;
	});

	spell.set_function("GetSubcategory", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->Subcategory;
		return 0;
	});

	spell.set_function("GetSpellGroup", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->SpellGroup;
		return 0;
	});

	// -- Level / Effects ---------------------------------------------------

	spell.set_function("GetLevelRequired", [](int spellId, int classId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (!pSpell)
			return 255;
		if (classId < 0 || classId > MAX_CLASSES)
			return 255;
		return static_cast<int>(pSpell->ClassLevel[classId]);
	});

	spell.set_function("GetNumEffects", [](int spellId) -> int {
		EQ_Spell* pSpell = FindSpell(spellId);
		if (pSpell)
			return pSpell->GetNumEffects();
		return 0;
	});

	// -- Rank resolution ---------------------------------------------------

	spell.set_function("GetHighestLearnedByGroup", [](int groupId) -> int {
		EQ_Spell* pSpell = GetHighestLearnedSpellByGroupID(groupId);
		if (pSpell)
			return pSpell->ID;
		return 0;
	});
}

} // namespace CF
