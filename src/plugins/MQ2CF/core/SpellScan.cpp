/*
 * MQ2CF: MacroQuest Class Framework
 * SpellScan subsystem -- scans spellbook, AAs, and discs into SQLite
 *
 * Scans the character's spellbook, alternate abilities, and combat
 * abilities (discs) using eqlib APIs and populates SQLite tables for
 * use by class logic scripts.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "SpellScan.h"

#include <string>

namespace CF {

// ---------------------------------------------------------------------------
// Schema for scanned spell/AA data
// ---------------------------------------------------------------------------
static const char* kCreateScannedSpellsTable = R"SQL(
    CREATE TABLE IF NOT EXISTS scanned_spells (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        class TEXT NOT NULL,
        spell_id INTEGER NOT NULL,
        spell_name TEXT NOT NULL,
        level INTEGER DEFAULT 0,
        category INTEGER DEFAULT 0,
        subcategory INTEGER DEFAULT 0,
        spell_group INTEGER DEFAULT 0,
        mana_cost INTEGER DEFAULT 0,
        cast_time REAL DEFAULT 0,
        recast_time REAL DEFAULT 0,
        range REAL DEFAULT 0,
        ae_range REAL DEFAULT 0,
        target_type INTEGER DEFAULT 0,
        base_value INTEGER DEFAULT 0,
        source TEXT DEFAULT 'spellbook',
        UNIQUE(class, spell_id)
    );
)SQL";

static const char* kCreateScannedAAsTable = R"SQL(
    CREATE TABLE IF NOT EXISTS scanned_aas (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        class TEXT NOT NULL,
        aa_id INTEGER NOT NULL,
        aa_name TEXT NOT NULL,
        rank INTEGER DEFAULT 0,
        max_rank INTEGER DEFAULT 0,
        spell_id INTEGER DEFAULT 0,
        spell_name TEXT,
        reuse_time REAL DEFAULT 0,
        UNIQUE(class, aa_id)
    );
)SQL";

// ---------------------------------------------------------------------------
// DB init
// ---------------------------------------------------------------------------
void InitSpellScanDB(sqlite3* db)
{
    if (!db)
        return;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, kCreateScannedSpellsTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax Failed to create scanned_spells table: %s", errMsg ? errMsg : "unknown");
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, kCreateScannedAAsTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax Failed to create scanned_aas table: %s", errMsg ? errMsg : "unknown");
        sqlite3_free(errMsg);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Get the 3-letter uppercase class short name for the local player.
// Returns "UNK" if unavailable.
static std::string GetClassShortName()
{
    if (!pLocalPlayer)
        return "UNK";

    int classId = pLocalPlayer->GetClass();
    if (classId < 1 || classId > 17)
        return "UNK";

    return ClassInfo[classId].UCShortName;
}

// Track how many spells were scanned in the last run.
static int s_lastSpellCount = 0;

// Pointer-to-pointer so lambdas always read the current DB handle.
// Updated by RegisterSpellScanBindings and UpdateSpellScanDB.
static sqlite3** s_ppDb = nullptr;

// ---------------------------------------------------------------------------
// Scan functions
// ---------------------------------------------------------------------------

static int ScanSpellbook(sqlite3* db)
{
    if (!db || !pLocalPC)
        return 0;

    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return 0;

    std::string className = GetClassShortName();
    int classId = pLocalPlayer->GetClass();
    int count = 0;

    const char* sql =
        "INSERT OR REPLACE INTO scanned_spells "
        "(class, spell_id, spell_name, level, category, subcategory, spell_group, "
        "mana_cost, cast_time, recast_time, range, ae_range, target_type, base_value, source) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'spellbook')";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax SpellScan prepare failed: %s", sqlite3_errmsg(db));
        return 0;
    }

    // Wrap in a transaction for performance
    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    for (int slot = 0; slot < NUM_BOOK_SLOTS; slot++)
    {
        int spellId = pProfile->GetSpellBook(slot);
        if (spellId <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(spellId);
        if (!pSpell)
            continue;

        int level = 255;
        if (classId >= 0 && classId <= MAX_CLASSES)
            level = static_cast<int>(pSpell->ClassLevel[classId]);

        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, spellId);
        sqlite3_bind_text(stmt, 3, pSpell->Name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, level);
        sqlite3_bind_int(stmt, 5, pSpell->Category);
        sqlite3_bind_int(stmt, 6, pSpell->Subcategory);
        sqlite3_bind_int(stmt, 7, pSpell->SpellGroup);
        sqlite3_bind_int(stmt, 8, pSpell->ManaCost);
        sqlite3_bind_double(stmt, 9, pSpell->CastTime / 1000.0);
        sqlite3_bind_double(stmt, 10, pSpell->RecastTime / 1000.0);
        sqlite3_bind_double(stmt, 11, pSpell->Range);
        sqlite3_bind_double(stmt, 12, pSpell->AERange);
        sqlite3_bind_int(stmt, 13, static_cast<int>(pSpell->TargetType));
        sqlite3_bind_int(stmt, 14, pSpell->GetNumEffects() > 0 ? static_cast<int>(pSpell->GetEffectBase(0)) : 0);

        if (sqlite3_step(stmt) == SQLITE_DONE)
            count++;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);

    s_lastSpellCount = count;
    return count;
}

static int ScanAAs(sqlite3* db)
{
    if (!db || !pLocalPC || !pAltAdvManager)
        return 0;

    std::string className = GetClassShortName();
    int count = 0;

    const char* sql =
        "INSERT OR REPLACE INTO scanned_aas "
        "(class, aa_id, aa_name, rank, max_rank, spell_id, spell_name, reuse_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax SpellScan AA prepare failed: %s", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    // Iterate over the character's AA list
    for (int i = 0; i < AA_CHAR_MAX_REAL; i++)
    {
        int aaIndex = pLocalPC->GetAlternateAbilityId(i);
        if (aaIndex <= 0)
            continue;

        CAltAbilityData* pAbility = GetAAById(aaIndex);
        if (!pAbility)
            continue;

        const char* aaName = pAbility->GetNameString();
        if (!aaName || !aaName[0])
            continue;

        // Compute current rank
        int currentRank = pAbility->CurrentRank - 1;
        if (pLocalPC->HasAlternateAbility(pAbility->Index))
            currentRank++;

        // Get associated spell name if any
        std::string spellName;
        int spellId = pAbility->SpellID;
        if (spellId > 0)
        {
            EQ_Spell* pSpell = GetSpellByID(spellId);
            if (pSpell)
                spellName = pSpell->Name;
        }

        // Reuse timer in seconds
        double reuseTime = static_cast<double>(
            pAltAdvManager->GetCalculatedTimer(pLocalPC, pAbility)) / 1000.0;

        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, pAbility->GroupID);
        sqlite3_bind_text(stmt, 3, aaName, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, currentRank);
        sqlite3_bind_int(stmt, 5, pAbility->MaxRank);
        sqlite3_bind_int(stmt, 6, spellId);
        sqlite3_bind_text(stmt, 7, spellName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 8, reuseTime);

        if (sqlite3_step(stmt) == SQLITE_DONE)
            count++;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);

    return count;
}

static int ScanDiscs(sqlite3* db)
{
    if (!db || !pLocalPC)
        return 0;

    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return 0;

    std::string className = GetClassShortName();
    int classId = pLocalPlayer->GetClass();
    int count = 0;

    const char* sql =
        "INSERT OR REPLACE INTO scanned_spells "
        "(class, spell_id, spell_name, level, category, subcategory, spell_group, "
        "mana_cost, cast_time, recast_time, range, ae_range, target_type, base_value, source) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'disc')";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax SpellScan disc prepare failed: %s", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    for (int slot = 0; slot < NUM_COMBAT_ABILITIES; slot++)
    {
        int spellId = pProfile->GetCombatAbility(slot);
        if (spellId <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(spellId);
        if (!pSpell)
            continue;

        int level = 255;
        if (classId >= 0 && classId <= MAX_CLASSES)
            level = static_cast<int>(pSpell->ClassLevel[classId]);

        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, spellId);
        sqlite3_bind_text(stmt, 3, pSpell->Name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, level);
        sqlite3_bind_int(stmt, 5, pSpell->Category);
        sqlite3_bind_int(stmt, 6, pSpell->Subcategory);
        sqlite3_bind_int(stmt, 7, pSpell->SpellGroup);
        sqlite3_bind_int(stmt, 8, pSpell->ManaCost);
        sqlite3_bind_double(stmt, 9, pSpell->CastTime / 1000.0);
        sqlite3_bind_double(stmt, 10, pSpell->RecastTime / 1000.0);
        sqlite3_bind_double(stmt, 11, pSpell->Range);
        sqlite3_bind_double(stmt, 12, pSpell->AERange);
        sqlite3_bind_int(stmt, 13, static_cast<int>(pSpell->TargetType));
        sqlite3_bind_int(stmt, 14, pSpell->GetNumEffects() > 0 ? static_cast<int>(pSpell->GetEffectBase(0)) : 0);

        if (sqlite3_step(stmt) == SQLITE_DONE)
            count++;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);

    s_lastSpellCount += count;
    return count;
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------

struct SpellResult
{
    int spellId = 0;
    std::string name;
};

static SpellResult GetBestSpell(sqlite3* db, int category, int subcategory)
{
    SpellResult result;
    if (!db)
        return result;

    std::string className = GetClassShortName();

    const char* sql =
        "SELECT spell_id, spell_name FROM scanned_spells "
        "WHERE class = ? AND category = ? AND subcategory = ? "
        "ORDER BY level DESC LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        return result;

    sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, category);
    sqlite3_bind_int(stmt, 3, subcategory);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.spellId = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.name = name ? name : "";
    }

    sqlite3_finalize(stmt);
    return result;
}

// ---------------------------------------------------------------------------
// Core.SpellScan bindings
// ---------------------------------------------------------------------------

void UpdateSpellScanDB(sqlite3* db)
{
	// Store the address of the db pointer so lambdas always read the current handle
	// This is a static local that persists -- we just update the value it points to
	static sqlite3* s_dbHolder = nullptr;
	s_dbHolder = db;
	s_ppDb = &s_dbHolder;
}

void RegisterSpellScanBindings(sol::table& core, sqlite3* db)
{
    // Initialize the pointer-to-pointer
    UpdateSpellScanDB(db);

    sol::table spellScan = core["SpellScan"].get_or_create<sol::table>();

    // Core.SpellScan.ScanSpellbook() -> int (count of spells scanned)
    spellScan.set_function("ScanSpellbook", []() -> int {
        sqlite3* db = s_ppDb ? *s_ppDb : nullptr;
        if (!pLocalPC || !pLocalPlayer)
        {
            WriteChatf("\ar[MQ2CF]\ax SpellScan: not in game");
            return 0;
        }
        int count = ScanSpellbook(db);
        WriteChatf("\ag[MQ2CF]\ax SpellScan: scanned %d spellbook entries", count);
        return count;
    });

    // Core.SpellScan.ScanAAs() -> int (count of AAs scanned)
    spellScan.set_function("ScanAAs", []() -> int {
        sqlite3* db = s_ppDb ? *s_ppDb : nullptr;
        if (!pLocalPC || !pLocalPlayer)
        {
            WriteChatf("\ar[MQ2CF]\ax SpellScan: not in game");
            return 0;
        }
        int count = ScanAAs(db);
        WriteChatf("\ag[MQ2CF]\ax SpellScan: scanned %d alternate abilities", count);
        return count;
    });

    // Core.SpellScan.ScanDiscs() -> int (count of discs scanned)
    spellScan.set_function("ScanDiscs", []() -> int {
        sqlite3* db = s_ppDb ? *s_ppDb : nullptr;
        if (!pLocalPC || !pLocalPlayer)
        {
            WriteChatf("\ar[MQ2CF]\ax SpellScan: not in game");
            return 0;
        }
        int count = ScanDiscs(db);
        WriteChatf("\ag[MQ2CF]\ax SpellScan: scanned %d combat abilities", count);
        return count;
    });

    // Core.SpellScan.ScanAll() -> int (total count)
    spellScan.set_function("ScanAll", []() -> int {
        sqlite3* db = s_ppDb ? *s_ppDb : nullptr;
        if (!pLocalPC || !pLocalPlayer)
        {
            WriteChatf("\ar[MQ2CF]\ax SpellScan: not in game");
            return 0;
        }

        s_lastSpellCount = 0;
        int spells = ScanSpellbook(db);
        int discs = ScanDiscs(db);
        int aas = ScanAAs(db);

        int total = spells + discs + aas;
        WriteChatf("\ag[MQ2CF]\ax SpellScan: %d spells, %d discs, %d AAs (%d total)",
            spells, discs, aas, total);

        // Dump scan results to a log file for remote inspection
        std::string logPath = std::string(gPathResources) + "\\MQ2CF\\scan_results.log";
        FILE* logFile = _fsopen(logPath.c_str(), "w", _SH_DENYNO);
        if (logFile)
        {
            fprintf(logFile, "MQ2CF Scan Results\n");
            fprintf(logFile, "Character: %s\n", pLocalPlayer->Name);
            fprintf(logFile, "Class: %d\n", pLocalPlayer->GetClass());
            fprintf(logFile, "Level: %d\n", pLocalPlayer->Level);
            fprintf(logFile, "Spells: %d  Discs: %d  AAs: %d  Total: %d\n\n", spells, discs, aas, total);

            // Dump scanned_spells table
            fprintf(logFile, "=== SCANNED SPELLS ===\n");
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, "SELECT spell_id, spell_name, level, category, subcategory, spell_group, mana_cost, cast_time, base_value, source FROM scanned_spells ORDER BY level DESC", -1, &stmt, nullptr) == SQLITE_OK)
            {
                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    fprintf(logFile, "%d|%s|%d|%s|%s|%d|%d|%.1f|%d|%s\n",
                        sqlite3_column_int(stmt, 0),
                        (const char*)sqlite3_column_text(stmt, 1),
                        sqlite3_column_int(stmt, 2),
                        (const char*)sqlite3_column_text(stmt, 3),
                        (const char*)sqlite3_column_text(stmt, 4),
                        sqlite3_column_int(stmt, 5),
                        sqlite3_column_int(stmt, 6),
                        sqlite3_column_double(stmt, 7),
                        sqlite3_column_int(stmt, 8),
                        (const char*)sqlite3_column_text(stmt, 9));
                }
                sqlite3_finalize(stmt);
            }

            // Dump scanned_aas table
            fprintf(logFile, "\n=== SCANNED AAs ===\n");
            if (sqlite3_prepare_v2(db, "SELECT aa_id, aa_name, rank, max_rank, spell_name, reuse_time FROM scanned_aas ORDER BY aa_name", -1, &stmt, nullptr) == SQLITE_OK)
            {
                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    fprintf(logFile, "%d|%s|%d/%d|%s|%.1f\n",
                        sqlite3_column_int(stmt, 0),
                        (const char*)sqlite3_column_text(stmt, 1),
                        sqlite3_column_int(stmt, 2),
                        sqlite3_column_int(stmt, 3),
                        sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "",
                        sqlite3_column_double(stmt, 5));
                }
                sqlite3_finalize(stmt);
            }

            fclose(logFile);
            WriteChatf("\ag[MQ2CF]\ax Scan log: %s", logPath.c_str());
        }

        return total;
    });

    // Core.SpellScan.GetBestSpell(category, subcategory) -> spellID, name
    spellScan.set_function("GetBestSpell",
        [](int category, int subcategory) -> std::tuple<int, std::string> {
            sqlite3* db = s_ppDb ? *s_ppDb : nullptr;
            SpellResult r = GetBestSpell(db, category, subcategory);
            return std::make_tuple(r.spellId, r.name);
        });

    // Core.SpellScan.GetSpellsByGroup(spellGroup) -> table of {id, name, level}
    spellScan.set_function("GetSpellsByGroup",
        [](sol::this_state ts, int spellGroup) -> sol::table {
            sol::state_view lua(ts);
            sol::table results = lua.create_table();
            sqlite3* db = s_ppDb ? *s_ppDb : nullptr;

            if (!db)
                return results;

            std::string className = GetClassShortName();

            const char* sql =
                "SELECT spell_id, spell_name, level FROM scanned_spells "
                "WHERE class = ? AND spell_group = ? "
                "ORDER BY level DESC";

            sqlite3_stmt* stmt = nullptr;
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK)
                return results;

            sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, spellGroup);

            int idx = 1;
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                sol::table entry = lua.create_table();
                entry["id"] = sqlite3_column_int(stmt, 0);

                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                entry["name"] = name ? name : "";
                entry["level"] = sqlite3_column_int(stmt, 2);

                results[idx++] = entry;
            }

            sqlite3_finalize(stmt);
            return results;
        });

    // Core.SpellScan.GetSpellCount() -> int
    spellScan.set_function("GetSpellCount", []() -> int {
        return s_lastSpellCount;
    });
}

} // namespace CF
