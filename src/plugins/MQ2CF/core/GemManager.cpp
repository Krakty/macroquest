/*
 * MQ2CF: MacroQuest Class Framework
 * Gem manager -- spell gem slot querying and memorization management
 *
 * Exposes Core.Gem bindings for Lua scripts to query memorized spells,
 * manage desired loadouts, reserve slots, and trigger memorization.
 *
 * All accessors use eqlib typed fields, not raw offsets.
 * Every function null-checks its pointers and returns a safe default on failure.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "GemManager.h"
#include "ModeSystem.h"

#include <string>

namespace CF {

// ---------------------------------------------------------------------------
// Runtime state
// ---------------------------------------------------------------------------

static int  s_desiredLoadout[NUM_SPELL_GEMS] = {0};   // spell_group_id per slot
static bool s_reserved[NUM_SPELL_GEMS]       = {false}; // reserved flag per slot

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
// SQLite persistence
// ---------------------------------------------------------------------------

static const char* kCreateGemLoadoutsTable = R"SQL(
	CREATE TABLE IF NOT EXISTS gem_loadouts (
		class_id   INTEGER NOT NULL,
		mode       INTEGER NOT NULL,
		slot       INTEGER NOT NULL,
		spell_group_id INTEGER NOT NULL,
		PRIMARY KEY(class_id, mode, slot)
	);
)SQL";

static void EnsureSchema(sqlite3* db)
{
	if (!db)
		return;
	char* errMsg = nullptr;
	sqlite3_exec(db, kCreateGemLoadoutsTable, nullptr, nullptr, &errMsg);
	if (errMsg)
		sqlite3_free(errMsg);
}

static void PersistSlot(sqlite3* db, int classId, int mode, int slot, int spellGroupId)
{
	if (!db)
		return;

	const char* sql = R"SQL(
		INSERT OR REPLACE INTO gem_loadouts (class_id, mode, slot, spell_group_id)
		VALUES (?, ?, ?, ?);
	)SQL";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return;

	sqlite3_bind_int(stmt, 1, classId);
	sqlite3_bind_int(stmt, 2, mode);
	sqlite3_bind_int(stmt, 3, slot);
	sqlite3_bind_int(stmt, 4, spellGroupId);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

// ---------------------------------------------------------------------------
// Core.Gem bindings
// ---------------------------------------------------------------------------

void RegisterGemBindings(sol::table& core, sqlite3* db)
{
	EnsureSchema(db);

	sol::table gem = core["Gem"].get_or_create<sol::table>();

	// -- GetCurrentSpell(slot) -> int spellId (-1 if empty) -----------------

	gem.set_function("GetCurrentSpell", [](int slot) -> int {
		if (!pLocalPC)
			return -1;
		PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
		if (!pProfile)
			return -1;
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return -1;
		return pProfile->GetMemorizedSpell(slot);
	});

	// -- GetDesiredSpell(slot) -> int spellGroupId (0 if none) --------------

	gem.set_function("GetDesiredSpell", [](int slot) -> int {
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return 0;
		return s_desiredLoadout[slot];
	});

	// -- SetDesiredSpell(slot, spellGroupId) --------------------------------

	gem.set_function("SetDesiredSpell", [db](int slot, int spellGroupId) {
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return;

		s_desiredLoadout[slot] = spellGroupId;

		// Persist to SQLite keyed on class + mode
		int classId = 0;
		if (pLocalPlayer)
			classId = pLocalPlayer->GetClass();
		int mode = GetCurrentMode();

		PersistSlot(db, classId, mode, slot, spellGroupId);
	});

	// -- IsReserved(slot) -> bool -------------------------------------------

	gem.set_function("IsReserved", [](int slot) -> bool {
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return false;
		return s_reserved[slot];
	});

	// -- SetReserved(slot, reserved) ----------------------------------------

	gem.set_function("SetReserved", [](int slot, bool reserved) {
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return;
		s_reserved[slot] = reserved;
	});

	// -- GetAvailableSlotCount() -> int -------------------------------------

	gem.set_function("GetAvailableSlotCount", []() -> int {
		if (!pLocalPC)
			return 0;

		int count = 0;
		for (int i = 0; i < NUM_SPELL_GEMS; ++i)
		{
			if (pLocalPC->CanUseMemorizedSpellSlot(i))
				++count;
		}
		return count;
	});

	// -- Memorize(slot, spellId) -> bool, reason ----------------------------

	gem.set_function("Memorize", [](int slot, int spellId) -> std::tuple<bool, std::string> {
		if (slot < 0 || slot >= NUM_SPELL_GEMS)
			return std::make_tuple(false, std::string("invalid_slot"));

		if (!pLocalPC)
			return std::make_tuple(false, std::string("no_player"));

		if (!pLocalPC->CanUseMemorizedSpellSlot(slot))
			return std::make_tuple(false, std::string("slot_locked"));

		EQ_Spell* pSpell = FindSpell(spellId);
		if (!pSpell)
			return std::make_tuple(false, std::string("bad_spell"));

		// Issue /memorize "spellname" slot+1
		char command[256];
		sprintf_s(command, "/memorize \"%s\" %d", pSpell->Name, slot + 1);
		EzCommand(command);
		return std::make_tuple(true, std::string(""));
	});

	// -- NeedsMemorization() -> bool ----------------------------------------

	gem.set_function("NeedsMemorization", []() -> bool {
		if (!pLocalPC)
			return false;
		PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
		if (!pProfile)
			return false;

		for (int i = 0; i < NUM_SPELL_GEMS; ++i)
		{
			// Skip slots with no desired spell or reserved slots
			if (s_desiredLoadout[i] == 0 || s_reserved[i])
				continue;

			int currentSpellId = pProfile->GetMemorizedSpell(i);
			if (currentSpellId <= 0)
				return true; // Slot empty but we want something there

			// Compare spell group of current spell against desired spell group
			EQ_Spell* pSpell = pSpellMgr ? pSpellMgr->GetSpellByID(currentSpellId) : nullptr;
			if (!pSpell)
				return true;

			if (pSpell->SpellGroup != s_desiredLoadout[i])
				return true;
		}
		return false;
	});
}

} // namespace CF
