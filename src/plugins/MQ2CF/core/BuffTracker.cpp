/*
 * MQ2CF: MacroQuest Class Framework
 * BuffTracker -- writes CharName_Buffs.ini for cross-character buff visibility
 *
 * Mechanical clone of MQ2CWTNBuffs.dll (FUN_180003220).
 * Scans buff slots, counts debuff SPAs, writes INI in identical format.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "BuffTracker.h"

#include <cstring>
#include <cstdio>

namespace CF {

// ---------------------------------------------------------------------------
// State -- matches MQ2CWTNBuffs globals
// ---------------------------------------------------------------------------
static ULONGLONG s_writeDelay       = 0;     // DAT_180120b90 -- zone delay
static ULONGLONG s_lastWriteTime    = 0;     // DAT_18011e380 -- last WriteBuffsTimer
static int       s_pulseCounter     = 0;     // DAT_18011e388 -- tick throttle
static int       s_pulseThrottle    = 5;     // DAT_18011c008 -- ticks between checks

// Cached counter values -- only write INI when changed
static uint32_t  s_lastDisease      = 0xFFFFFFFF;  // DAT_18011e330
static uint32_t  s_lastPoison       = 0xFFFFFFFF;  // DAT_18011e334
static uint32_t  s_lastCurse        = 0xFFFFFFFF;  // DAT_18011e338
static uint32_t  s_lastCorruption   = 0xFFFFFFFF;  // DAT_18011e33c

// Cached buff strings -- only write INI when changed
static char s_lastBuffs[2048]       = "";    // DAT_18011e390
static char s_lastShortBuffs[2048]  = "";    // DAT_18011eb90
static char s_lastBlockedBuffs[2048] = "";   // DAT_18011f390
static char s_lastPetBuffs[2048]    = "";    // DAT_18011fb90
static char s_lastPetBlocked[2048]  = "";    // DAT_180120390

// ---------------------------------------------------------------------------
// GetINIPath -- builds path: <MQConfig>\CWTN\Buffs\<CharName>_Buffs.ini
// Matches FUN_180005290 format string at 0x1800f0ad0
// ---------------------------------------------------------------------------
static bool GetINIPath(char* out, size_t outSize)
{
    if (!pLocalPC)
        return false;

    // Get character name from PcProfile
    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return false;

    // Build path matching CWTN format
    sprintf_s(out, outSize, "%s\\CWTN\\Buffs\\%s_Buffs.ini",
              gPathConfig, pLocalPlayer->Name);
    return true;
}

// ---------------------------------------------------------------------------
// AppendSpellName -- appends "|SpellName" to buffer (pipe-delimited)
// Matches FUN_18003fe90 pattern
// ---------------------------------------------------------------------------
static void AppendSpellName(char* buf, size_t bufSize, const char* spellName)
{
    size_t curLen = strlen(buf);
    if (curLen + 1 + strlen(spellName) + 1 < bufSize)
    {
        strcat_s(buf, bufSize, "|");
        strcat_s(buf, bufSize, spellName);
    }
}

// ---------------------------------------------------------------------------
// CountDebuffSPA_LongBuffs -- matches FUN_180002510
// Iterates long buff slots (0..0x3D), for each buff checks spell effects.
// If any effect has GetSpellAttrib == spaId, sums the counter values.
// ---------------------------------------------------------------------------
static int CountDebuffSPA_LongBuffs(int spaId)
{
    if (!pLocalPC)
        return 0;

    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return 0;

    int total = 0;

    for (int slot = 0; slot < NUM_LONG_BUFFS; slot++)
    {
        const EQ_Affect& affect = pProfile->GetEffect(slot);
        if (affect.SpellID <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(affect.SpellID);
        if (!pSpell)
            continue;

        // Skip detrimental spells (SpellType != 0 means beneficial)
        // Matches: *(char *)(lVar4 + 0x185) == '\0'
        if (pSpell->SpellType != 0)
            continue;

        int numEffects = pSpell->NumEffects;
        for (int eff = 0; eff < numEffects; eff++)
        {
            int attrib = GetSpellAttrib(pSpell, eff);
            if (attrib == spaId)
            {
                // Sum counter values from the EQ_Affect slot data
                // The affect has up to 6 slots (0x60 bytes / 0x10 per slot)
                for (int s = 0; s < 6; s++)
                {
                    if (affect.SlotData[s].Slot == eff)
                    {
                        total += static_cast<int>(affect.SlotData[s].Value);
                    }
                }
            }
        }
    }

    return total;
}

// ---------------------------------------------------------------------------
// CountDebuffSPA_ShortBuffs -- matches FUN_180002b60
// Same as above but for short buff slots (0x3E..0x5C)
// ---------------------------------------------------------------------------
static int CountDebuffSPA_ShortBuffs(int spaId)
{
    if (!pLocalPC)
        return 0;

    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return 0;

    int total = 0;

    for (int slot = NUM_LONG_BUFFS; slot < MAX_TOTAL_BUFFS; slot++)
    {
        const EQ_Affect& affect = pProfile->GetEffect(slot);
        if (affect.SpellID <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(affect.SpellID);
        if (!pSpell)
            continue;

        if (pSpell->SpellType != 0)
            continue;

        int numEffects = pSpell->NumEffects;
        for (int eff = 0; eff < numEffects; eff++)
        {
            int attrib = GetSpellAttrib(pSpell, eff);
            if (attrib == spaId)
            {
                for (int s = 0; s < 6; s++)
                {
                    if (affect.SlotData[s].Slot == eff)
                    {
                        total += static_cast<int>(affect.SlotData[s].Value);
                    }
                }
            }
        }
    }

    return total;
}

// ---------------------------------------------------------------------------
// WriteIntIfChanged -- writes an int to INI only if value changed
// Matches FUN_1800040e0
// ---------------------------------------------------------------------------
static void WriteIntIfChanged(const char* section, const char* key,
                              uint32_t value, uint32_t& cached,
                              const char* iniPath)
{
    if (value != cached)
    {
        char valStr[32];
        sprintf_s(valStr, "%u", value);
        WritePrivateProfileStringA(section, key, valStr, iniPath);
        cached = value;
    }
}

// ---------------------------------------------------------------------------
// WriteStringIfChanged -- writes string to INI only if value changed
// ---------------------------------------------------------------------------
static void WriteStringIfChanged(const char* section, const char* key,
                                 const char* value, char* cached,
                                 size_t cachedSize, const char* iniPath)
{
    DWORD readLen = GetPrivateProfileStringA(section, key, "Failed",
                                             cached, static_cast<DWORD>(cachedSize), iniPath);
    if (readLen == 0 || cached[0] == '\0' || strcmp(cached, value) != 0)
    {
        WritePrivateProfileStringA(section, key, value, iniPath);
        size_t len = strlen(value);
        if (len == 0)
        {
            memcpy(cached, value, len + 1);
        }
    }
}

// ---------------------------------------------------------------------------
// BuffTracker_WritePulse -- main write function, matches FUN_180003220
// ---------------------------------------------------------------------------
static void BuffTracker_WritePulse()
{
    if (!pLocalPC || !pLocalPlayer)
        return;

    PcProfile* pProfile = pLocalPC->GetCurrentPcProfile();
    if (!pProfile)
        return;

    char iniPath[260];
    if (!GetINIPath(iniPath, sizeof(iniPath)))
        return;

    // Create directory if needed
    char dirPath[260];
    sprintf_s(dirPath, "%s\\CWTN\\Buffs", gPathConfig);
    CreateDirectoryA((std::string(gPathConfig) + "\\CWTN").c_str(), nullptr);
    CreateDirectoryA(dirPath, nullptr);

    char buildBuf[2048];

    // ---------------------------------------------------------------
    // 1. Long buffs (0..0x3D = 62 slots) -> [MyBuffs] Buffs
    // ---------------------------------------------------------------
    memset(buildBuf, 0, sizeof(buildBuf));
    for (int slot = 0; slot < NUM_LONG_BUFFS; slot++)
    {
        const EQ_Affect& affect = pProfile->GetEffect(slot);
        if (affect.SpellID <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(affect.SpellID);
        if (!pSpell)
            continue;

        // Skip "Permanent" spells (name ends with ":Permanent")
        const char* name = pSpell->Name;
        size_t nameLen = strlen(name);
        if (nameLen > 10)
        {
            const char* suffix = name + nameLen - 10;
            if (_stricmp(suffix, ":Permanent") == 0)
                continue;
        }

        AppendSpellName(buildBuf, sizeof(buildBuf), name);
    }
    // Trailing pipe (CWTN format: |Name1|Name2|Name3|)
    strcat_s(buildBuf, sizeof(buildBuf), "|");
    WriteStringIfChanged("MyBuffs", "Buffs", buildBuf,
                        s_lastBuffs, sizeof(s_lastBuffs), iniPath);

    // ---------------------------------------------------------------
    // 2. Short buffs (0x3E..0x5C = 31 slots) -> [MyBuffs] ShortBuffs
    // ---------------------------------------------------------------
    memset(buildBuf, 0, sizeof(buildBuf));
    for (int slot = NUM_LONG_BUFFS; slot < MAX_TOTAL_BUFFS; slot++)
    {
        const EQ_Affect& affect = pProfile->GetEffect(slot);
        if (affect.SpellID <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(affect.SpellID);
        if (!pSpell)
            continue;

        // Skip ":Permanent" spells
        const char* name = pSpell->Name;
        size_t nameLen = strlen(name);
        if (nameLen > 10)
        {
            const char* suffix = name + nameLen - 10;
            if (_stricmp(suffix, ":Permanent") == 0)
                continue;
        }

        AppendSpellName(buildBuf, sizeof(buildBuf), name);
    }
    strcat_s(buildBuf, sizeof(buildBuf), "|");
    WriteStringIfChanged("MyBuffs", "ShortBuffs", buildBuf,
                        s_lastShortBuffs, sizeof(s_lastShortBuffs), iniPath);

    // ---------------------------------------------------------------
    // 3. Blocked buffs (60 slots) -> [MyBuffs] BlockedBuffs
    // Reads from pLocalPC blocked buff array at offset 0x2650
    // ---------------------------------------------------------------
    memset(buildBuf, 0, sizeof(buildBuf));
    for (int i = 0; i < pProfile->BlockedSpell.GetSize(); i++)
    {
        int spellId = pProfile->BlockedSpell[i];
        if (spellId <= 0)
            continue;

        EQ_Spell* pSpell = GetSpellByID(spellId);
        if (!pSpell)
            continue;

        AppendSpellName(buildBuf, sizeof(buildBuf), pSpell->Name);
    }
    strcat_s(buildBuf, sizeof(buildBuf), "|");
    WriteStringIfChanged("MyBuffs", "BlockedBuffs", buildBuf,
                        s_lastBlockedBuffs, sizeof(s_lastBlockedBuffs), iniPath);

    // ---------------------------------------------------------------
    // 4. Pet buffs (if pet exists)
    // Original checks pLocalPC+0x2848 (CharacterZoneClient) then PetID at +0x230
    // ---------------------------------------------------------------
    if (pLocalPlayer->PetID > 0)
    {
        // Pet buffs from pPetInfoWnd (0x5D=93 slots in original)
        memset(buildBuf, 0, sizeof(buildBuf));
        if (pPetInfoWnd)
        {
            int maxPetBuffs = pPetInfoWnd->GetMaxBuffs();
            for (int i = 0; i < maxPetBuffs; i++)
            {
                int spellId = pPetInfoWnd->GetBuff(i);
                if (spellId <= 0)
                    continue;

                EQ_Spell* pSpell = GetSpellByID(spellId);
                if (!pSpell)
                    continue;

                AppendSpellName(buildBuf, sizeof(buildBuf), pSpell->Name);
            }
        }
        strcat_s(buildBuf, sizeof(buildBuf), "|");
        WriteStringIfChanged("MyBuffs", "PetBuffs", buildBuf,
                            s_lastPetBuffs, sizeof(s_lastPetBuffs), iniPath);

        // Pet blocked buffs (60 slots at pLocalPC+0x2680)
        memset(buildBuf, 0, sizeof(buildBuf));
        for (int i = 0; i < pProfile->BlockedPetSpell.GetSize(); i++)
        {
            int spellId = pProfile->BlockedPetSpell[i];
            if (spellId <= 0)
                continue;

            EQ_Spell* pSpell = GetSpellByID(spellId);
            if (!pSpell)
                continue;

            AppendSpellName(buildBuf, sizeof(buildBuf), pSpell->Name);
        }
        strcat_s(buildBuf, sizeof(buildBuf), "|");
        WriteStringIfChanged("MyBuffs", "PetBlockedBuffs", buildBuf,
                            s_lastPetBlocked, sizeof(s_lastPetBlocked), iniPath);
    }

    // ---------------------------------------------------------------
    // 5. Debuff counters by SPA
    // SPA 0x23 = 35 = Disease
    // SPA 0x24 = 36 = Poison
    // SPA 0x74 = 116 = Curse
    // SPA 0x171 = 369 = Corruption
    // ---------------------------------------------------------------
    {
        uint32_t disease = CountDebuffSPA_LongBuffs(0x23) + CountDebuffSPA_ShortBuffs(0x23);
        WriteIntIfChanged("MyBuffs", "DiseaseCounters", disease, s_lastDisease, iniPath);
    }
    {
        uint32_t poison = CountDebuffSPA_LongBuffs(0x24) + CountDebuffSPA_ShortBuffs(0x24);
        WriteIntIfChanged("MyBuffs", "PoisonCounters", poison, s_lastPoison, iniPath);
    }
    {
        uint32_t curse = CountDebuffSPA_LongBuffs(0x74) + CountDebuffSPA_ShortBuffs(0x74);
        WriteIntIfChanged("MyBuffs", "CurseCounters", curse, s_lastCurse, iniPath);
    }
    {
        uint32_t corruption = CountDebuffSPA_LongBuffs(0x171) + CountDebuffSPA_ShortBuffs(0x171);
        WriteIntIfChanged("MyBuffs", "CorruptionCounters", corruption, s_lastCorruption, iniPath);
    }

    // ---------------------------------------------------------------
    // 6. WriteBuffsTimer + Num_Long_Buffs + Max_Long_Buffs
    // Only written every 3 seconds (matches original timer)
    // ---------------------------------------------------------------
    ULONGLONG now = GetTickCount64();
    if (now >= s_lastWriteTime + 3000)
    {
        s_lastWriteTime = now;

        // WriteBuffsTimer = GetTickCount64 as string
        char timerStr[32];
        _i64toa_s(static_cast<long long>(now), timerStr, sizeof(timerStr), 10);
        WritePrivateProfileStringA("MyInfo", "WriteBuffsTimer", timerStr, iniPath);

        // Num_Long_Buffs -- count of active long buff slots
        int numLong = 0;
        for (int slot = 0; slot < NUM_LONG_BUFFS; slot++)
        {
            const EQ_Affect& affect = pProfile->GetEffect(slot);
            if (affect.SpellID > 0)
                numLong++;
        }
        char numStr[32];
        sprintf_s(numStr, "%d", numLong);
        WritePrivateProfileStringA("MyInfo", "Num_Long_Buffs", numStr, iniPath);

        // Max_Long_Buffs -- from GetCharMaxBuffSlots
        uint32_t maxSlots = GetCharMaxBuffSlots();
        sprintf_s(numStr, "%d", maxSlots);
        WritePrivateProfileStringA("MyInfo", "Max_Long_Buffs", numStr, iniPath);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void BuffTracker_OnPulse()
{
    if (GetGameState() != GAMESTATE_INGAME)
        return;

    if (!pLocalPC || !pLocalPlayer)
        return;

    s_pulseCounter++;
    if (s_pulseCounter < s_pulseThrottle)
        return;

    s_pulseCounter = 0;

    ULONGLONG now = GetTickCount64();
    if (now < s_writeDelay)
        return;

    BuffTracker_WritePulse();
}

void BuffTracker_OnZoned()
{
    ULONGLONG now = GetTickCount64();
    s_writeDelay = now + 3000;
}

void BuffTracker_OnSetGameState(int gameState)
{
    // On logout (not INGAME), clear INI sections
    if (gameState == GAMESTATE_INGAME)
        return;

    if (!pLocalPC || !pLocalPlayer)
        return;

    char iniPath[260];
    if (!GetINIPath(iniPath, sizeof(iniPath)))
        return;

    WritePrivateProfileStringA("MyBuffs", nullptr, nullptr, iniPath);
    WritePrivateProfileStringA("MyInfo", nullptr, nullptr, iniPath);

    // Reset cached values
    s_lastDisease    = 0xFFFFFFFF;
    s_lastPoison     = 0xFFFFFFFF;
    s_lastCurse      = 0xFFFFFFFF;
    s_lastCorruption = 0xFFFFFFFF;
    s_lastBuffs[0]      = '\0';
    s_lastShortBuffs[0] = '\0';
    s_lastBlockedBuffs[0] = '\0';
    s_lastPetBuffs[0]   = '\0';
    s_lastPetBlocked[0] = '\0';
}

} // namespace CF
