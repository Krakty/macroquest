/*
 * MQ2CF: MacroQuest Class Framework
 * Settings subsystem -- persists configuration to SQLite, exposes to Lua
 *
 * Runtime settings override the defaults defined in settings_defaults.
 * A runtime table (settings_runtime) stores per-key overrides. Get queries
 * check runtime first, then fall back to defaults. Set always writes to
 * runtime. Profiles load/save the entire runtime table under a named tag.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "Settings.h"

#include <string>

namespace CF {

// ---------------------------------------------------------------------------
// Schema for runtime settings
// ---------------------------------------------------------------------------
static const char* kCreateRuntimeTable = R"SQL(
    CREATE TABLE IF NOT EXISTS settings_runtime (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile TEXT NOT NULL DEFAULT 'default',
        key TEXT NOT NULL,
        value TEXT NOT NULL,
        UNIQUE(profile, key)
    );
)SQL";

// ---------------------------------------------------------------------------
// DB init
// ---------------------------------------------------------------------------
void InitSettingsDB(sqlite3* db)
{
    if (!db)
        return;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, kCreateRuntimeTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        WriteChatf("\ar[MQ2CF]\ax Failed to create settings_runtime table: %s", errMsg ? errMsg : "unknown");
        sqlite3_free(errMsg);
    }
}

// ---------------------------------------------------------------------------
// Internal helpers (not exposed to Lua)
// ---------------------------------------------------------------------------

// Current active profile name. Defaults to "default".
static std::string s_activeProfile = "default";

// Get a setting value. Checks runtime first, then settings_defaults.
// Returns empty string if not found.
static std::string GetSettingValue(sqlite3* db, const std::string& key)
{
    if (!db)
        return "";

    // Check runtime table first
    const char* sqlRuntime = "SELECT value FROM settings_runtime WHERE profile = ? AND key = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sqlRuntime, -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, s_activeProfile.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string result = val ? val : "";
            sqlite3_finalize(stmt);
            return result;
        }
        sqlite3_finalize(stmt);
    }

    // Fall back to defaults table
    const char* sqlDefault = "SELECT value FROM settings_defaults WHERE key = ?";
    rc = sqlite3_prepare_v2(db, sqlDefault, -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string result = val ? val : "";
            sqlite3_finalize(stmt);
            return result;
        }
        sqlite3_finalize(stmt);
    }

    return "";
}

// Set a setting value in the runtime table (upsert).
static bool SetSettingValue(sqlite3* db, const std::string& key, const std::string& value)
{
    if (!db)
        return false;

    const char* sql =
        "INSERT INTO settings_runtime (profile, key, value) VALUES (?, ?, ?) "
        "ON CONFLICT(profile, key) DO UPDATE SET value = excluded.value";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, s_activeProfile.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// Load a profile: set active profile name, effectively switching which
// runtime overrides are in effect. Returns true if the profile has any
// rows (or we just set it).
static bool LoadProfile(sqlite3* db, const std::string& profileName)
{
    if (!db || profileName.empty())
        return false;

    s_activeProfile = profileName;

    // Check if profile has any entries
    const char* sql = "SELECT COUNT(*) FROM settings_runtime WHERE profile = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, profileName.c_str(), -1, SQLITE_TRANSIENT);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    WriteChatf("\ag[MQ2CF]\ax Loaded profile '%s' (%d runtime overrides)", profileName.c_str(), count);
    return true;
}

// Save the current runtime settings to a new profile name.
// Copies all rows from the active profile to the target profile.
static bool SaveProfile(sqlite3* db, const std::string& profileName)
{
    if (!db || profileName.empty())
        return false;

    // If saving to the same profile, nothing to copy
    if (profileName == s_activeProfile)
    {
        WriteChatf("\ag[MQ2CF]\ax Profile '%s' saved (already active)", profileName.c_str());
        return true;
    }

    // Delete any existing rows for target profile, then copy from active
    const char* sqlDelete = "DELETE FROM settings_runtime WHERE profile = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sqlDelete, -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, profileName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char* sqlCopy =
        "INSERT INTO settings_runtime (profile, key, value) "
        "SELECT ?, key, value FROM settings_runtime WHERE profile = ?";

    rc = sqlite3_prepare_v2(db, sqlCopy, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, profileName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, s_activeProfile.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        WriteChatf("\ag[MQ2CF]\ax Profile saved as '%s'", profileName.c_str());
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Core.Settings bindings
// ---------------------------------------------------------------------------

void RegisterSettingsBindings(sol::table& core, sqlite3* db)
{
    sol::table settings = core["Settings"].get_or_create<sol::table>();

    // Core.Settings.Get(key) -> string
    settings.set_function("Get", [db](const std::string& key) -> std::string {
        return GetSettingValue(db, key);
    });

    // Core.Settings.Set(key, value) -> bool
    settings.set_function("Set", [db](const std::string& key, const std::string& value) -> bool {
        return SetSettingValue(db, key, value);
    });

    // Core.Settings.GetInt(key, default) -> int
    settings.set_function("GetInt", [db](const std::string& key, int defaultVal) -> int {
        std::string val = GetSettingValue(db, key);
        if (val.empty())
            return defaultVal;
        try
        {
            return std::stoi(val);
        }
        catch (...)
        {
            return defaultVal;
        }
    });

    // Core.Settings.GetBool(key, default) -> bool
    settings.set_function("GetBool", [db](const std::string& key, bool defaultVal) -> bool {
        std::string val = GetSettingValue(db, key);
        if (val.empty())
            return defaultVal;
        // Accept "true"/"1"/"yes" as true, everything else is false
        if (val == "true" || val == "1" || val == "yes" || val == "True" || val == "TRUE")
            return true;
        if (val == "false" || val == "0" || val == "no" || val == "False" || val == "FALSE")
            return false;
        return defaultVal;
    });

    // Core.Settings.GetFloat(key, default) -> float
    settings.set_function("GetFloat", [db](const std::string& key, float defaultVal) -> float {
        std::string val = GetSettingValue(db, key);
        if (val.empty())
            return defaultVal;
        try
        {
            return std::stof(val);
        }
        catch (...)
        {
            return defaultVal;
        }
    });

    // Core.Settings.LoadProfile(profileName) -> bool
    settings.set_function("LoadProfile", [db](const std::string& profileName) -> bool {
        return LoadProfile(db, profileName);
    });

    // Core.Settings.SaveProfile(profileName) -> bool
    settings.set_function("SaveProfile", [db](const std::string& profileName) -> bool {
        return SaveProfile(db, profileName);
    });
}

} // namespace CF
