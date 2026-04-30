#include <mq/Plugin.h>

PreSetup("MQ2DumpItem");

static void DumpRawItem(ItemClient* pItem, const char* label)
{
    WriteChatf("\ag=== Raw ItemBase dump for: %s (ID: %d) [%s] ===", pItem->GetName(), pItem->GetID(), label);

    const uint32_t* dwords = reinterpret_cast<const uint32_t*>(pItem);

    for (int i = 0; i < 67; i++)
    {
        int offset = i * 4;
        WriteChatf("  +0x%03X: %11d  (0x%08X)", offset, static_cast<int>(dwords[i]), dwords[i]);
    }
}

void DumpItemCmd(PlayerClient* pChar, const char* szLine)
{
    if (!szLine[0])
    {
        WriteChatf("Usage: /dumpitem <item name>");
        WriteChatf("       /dumpitem merch <item name>  (dump merchant's copy)");
        return;
    }

    char arg1[256] = { 0 };
    GetArg(arg1, szLine, 1);
    const char* rest = szLine + strlen(arg1);
    while (*rest == ' ') rest++;

    if (_stricmp(arg1, "merch") == 0 && rest[0])
    {
        // Search merchant items
        if (!pActiveMerchant || !pMerchantWnd)
        {
            WriteChatf("Not at a merchant!");
            return;
        }

        // Use FindItem with merchant context - search through MQ's merchant item access
        ItemClient* pFound = nullptr;
        if (pMerchantWnd)
        {
            // Iterate merchant page handler items
            for (int page = 0; page < 2; page++)
            {
                if (auto handler = pMerchantWnd->PageHandlers[page])
                {
                    int count = handler->ItemContainer.GetSize();
                    for (int i = 0; i < count; i++)
                    {
                        auto& entry = handler->ItemContainer[i];
                        if (entry.pItem && _stricmp(entry.pItem->GetName(), rest) == 0)
                        {
                            DumpRawItem(entry.pItem.get(), "merchant item");
                            WriteChatf("  MerchantEntry.Unknown = %d", entry.Unknown);
                            // Also dump the raw MerchantItemEntry bytes
                            const uint32_t* entryDwords = reinterpret_cast<const uint32_t*>(&entry);
                            WriteChatf("  \ayRaw MerchantItemEntry:");
                            for (int j = 0; j < 5; j++)
                                WriteChatf("    entry+0x%02X: %d (0x%08X)", j*4, (int)entryDwords[j], entryDwords[j]);
                            return;
                        }
                    }
                }
            }
        }
        WriteChatf("Merchant item not found: %s", rest);
    }
    else
    {
        // Search inventory
        auto pItem = FindItemByName(szLine);
        if (!pItem)
        {
            WriteChatf("Item not found: %s", szLine);
            return;
        }
        DumpRawItem(pItem, "inventory item");
    }
}

// Dump raw PlayerClient memory at a specific offset range
void DumpSpawnCmd(PlayerClient* pChar, const char* szLine)
{
    PlayerClient* pSpawn = nullptr;
    int startOffset = 0;
    int endOffset = 0x108;

    char arg1[256] = { 0 };
    char arg2[256] = { 0 };
    char arg3[256] = { 0 };
    GetArg(arg1, szLine, 1);
    GetArg(arg2, szLine, 2);
    GetArg(arg3, szLine, 3);

    if (!arg1[0] || _stricmp(arg1, "help") == 0)
    {
        WriteChatf("Usage: /dumpspawn self [startOff] [endOff]   -- dump your PlayerClient");
        WriteChatf("       /dumpspawn target [startOff] [endOff] -- dump target's PlayerClient");
        WriteChatf("  Offsets are hex (e.g., 0x100 0x200). Default: 0x000-0x108");
        WriteChatf("  Example: /dumpspawn self 0x1D0 0x260");
        return;
    }

    if (_stricmp(arg1, "self") == 0 || _stricmp(arg1, "me") == 0)
    {
        pSpawn = pLocalPlayer;
        if (!pSpawn) { WriteChatf("Not in game!"); return; }
    }
    else if (_stricmp(arg1, "target") == 0 || _stricmp(arg1, "tar") == 0)
    {
        pSpawn = pTarget;
        if (!pSpawn) { WriteChatf("No target!"); return; }
    }
    else
    {
        WriteChatf("Unknown spawn selector: %s (use 'self' or 'target')", arg1);
        return;
    }

    if (arg2[0]) startOffset = (int)strtol(arg2, nullptr, 0);
    if (arg3[0]) endOffset = (int)strtol(arg3, nullptr, 0);

    // Clamp to reasonable bounds (PlayerClient is ~0x20D8)
    if (startOffset < 0) startOffset = 0;
    if (endOffset <= startOffset) endOffset = startOffset + 0x108;
    if (endOffset > 0x2100) endOffset = 0x2100;

    // Align to DWORD boundary
    startOffset &= ~3;

    WriteChatf("\ag=== Raw PlayerClient dump for: %s (0x%03X - 0x%03X) ===",
        pSpawn->Name, startOffset, endOffset);

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(pSpawn);
    const uint32_t* base = reinterpret_cast<const uint32_t*>(bytes + startOffset);
    int count = (endOffset - startOffset) / 4;

    for (int i = 0; i < count; i++)
    {
        int offset = startOffset + i * 4;
        uint32_t val = base[i];

        // Annotate known fields to help identify unknowns
        const char* annotation = "";
        if (offset == 0x0B4) annotation = " <- Name";
        else if (offset == 0x048) annotation = " <- Lastname";
        else if (offset == 0x074) annotation = " <- Y";
        else if (offset == 0x078) annotation = " <- X";
        else if (offset == 0x07C) annotation = " <- Z";
        else if (offset == 0x090) annotation = " <- Heading";
        else if (offset == 0x135) annotation = " <- Type (byte)";
        else if (offset == 0x168) annotation = " <- SpawnID";
        else if (offset == 0x2A0) annotation = " <- GuildID";
        else if (offset == 0x3FC) annotation = " <- GM";
        else if (offset == 0x3FD) annotation = " <- Level (byte)";
        else if (offset == 0x450) annotation = " <- AFK";
        else if (offset == 0x4F8) annotation = " <- pCharacter";
        else if (offset == 0x654) annotation = " <- Zone";
        else if (offset == 0xFD8) annotation = " <- Unknown0xFD8";
        else if (offset == 0xFE0) annotation = " <- mActorClient";

        WriteChatf("  +0x%03X: %11d  (0x%08X)%s", offset, static_cast<int>(val), val, annotation);
    }
}

// Generic raw memory dumper — dumps DWORDs from any pointer with offset range
static void DumpRawMemory(const void* ptr, const char* label, int startOffset, int endOffset)
{
    if (startOffset < 0) startOffset = 0;
    if (endOffset <= startOffset) endOffset = startOffset + 0x100;
    startOffset &= ~3;

    WriteChatf("\ag=== Raw dump: %s (0x%03X - 0x%03X) ===", label, startOffset, endOffset);

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(ptr);
    int count = (endOffset - startOffset) / 4;

    for (int i = 0; i < count; i++)
    {
        int offset = startOffset + i * 4;
        uint32_t val = *reinterpret_cast<const uint32_t*>(bytes + offset);
        WriteChatf("  +0x%03X: %11d  (0x%08X)", offset, static_cast<int>(val), val);
    }
}

// /dumpprofile [startOff] [endOff] — dump PcProfile (buffs, skills, inventory offsets)
void DumpProfileCmd(PlayerClient* pChar, const char* szLine)
{
    auto pProfile = GetPcProfile();
    if (!pProfile)
    {
        WriteChatf("No PcProfile available!");
        return;
    }

    char arg1[64] = { 0 };
    char arg2[64] = { 0 };
    GetArg(arg1, szLine, 1);
    GetArg(arg2, szLine, 2);

    int start = arg1[0] ? (int)strtol(arg1, nullptr, 0) : 0;
    int end = arg2[0] ? (int)strtol(arg2, nullptr, 0) : start + 0x200;
    if (end > 0x7000) end = 0x7000; // PcProfile is ~0x6E98

    DumpRawMemory(pProfile, "PcProfile", start, end);
}

// /dumpwnd [startOff] [endOff] — dump the focused/target CXWnd
void DumpWndCmd(PlayerClient* pChar, const char* szLine)
{
    char arg1[256] = { 0 };
    char arg2[64] = { 0 };
    char arg3[64] = { 0 };
    GetArg(arg1, szLine, 1);
    GetArg(arg2, szLine, 2);
    GetArg(arg3, szLine, 3);

    if (!arg1[0])
    {
        WriteChatf("Usage: /dumpwnd <window name> [startOff] [endOff]");
        WriteChatf("  Example: /dumpwnd ChatWindow 0x000 0x270");
        return;
    }

    CXWnd* pWnd = FindMQ2Window(arg1);
    if (!pWnd)
    {
        WriteChatf("Window not found: %s", arg1);
        return;
    }

    int start = arg2[0] ? (int)strtol(arg2, nullptr, 0) : 0;
    int end = arg3[0] ? (int)strtol(arg3, nullptr, 0) : start + 0x270;

    char label[256];
    sprintf_s(label, "CXWnd '%s'", arg1);
    DumpRawMemory(pWnd, label, start, end);
}

// /dumpitemdef <item name> [startOff] [endOff] — dump ItemDefinition (static template)
void DumpItemDefCmd(PlayerClient* pChar, const char* szLine)
{
    char arg1[256] = { 0 };
    GetArg(arg1, szLine, 1);
    const char* rest = szLine;
    // Find item name (everything before optional hex offsets)

    if (!szLine[0])
    {
        WriteChatf("Usage: /dumpitemdef <item name>");
        return;
    }

    auto pItem = FindItemByName(szLine);
    if (!pItem)
    {
        WriteChatf("Item not found: %s", szLine);
        return;
    }

    auto pDef = pItem->GetItemDefinition();
    if (!pDef)
    {
        WriteChatf("No ItemDefinition for: %s", szLine);
        return;
    }

    WriteChatf("\ag=== ItemDefinition dump for: %s (ID: %d) ===", pDef->Name, pDef->ItemNumber);
    // Dump full ItemDefinition (0x688 bytes)
    DumpRawMemory(pDef, "ItemDefinition", 0, 0x688);
}

// /dumpeq [startOff] [endOff] — dump CEverQuest instance
void DumpEQCmd(PlayerClient* pChar, const char* szLine)
{
    if (!pEverQuest)
    {
        WriteChatf("pEverQuest is null!");
        return;
    }

    char arg1[64] = { 0 };
    char arg2[64] = { 0 };
    GetArg(arg1, szLine, 1);
    GetArg(arg2, szLine, 2);

    int start = arg1[0] ? (int)strtol(arg1, nullptr, 0) : 0;
    int end = arg2[0] ? (int)strtol(arg2, nullptr, 0) : start + 0x200;
    if (end > 0x19800) end = 0x19800; // CEverQuest is ~0x19710

    DumpRawMemory(pEverQuest, "CEverQuest", start, end);
}

PLUGIN_API void InitializePlugin()
{
    AddCommand("/dumpitem", DumpItemCmd);
    AddCommand("/dumpspawn", DumpSpawnCmd);
    AddCommand("/dumpprofile", DumpProfileCmd);
    AddCommand("/dumpwnd", DumpWndCmd);
    AddCommand("/dumpitemdef", DumpItemDefCmd);
    AddCommand("/dumpeq", DumpEQCmd);
}

PLUGIN_API void ShutdownPlugin()
{
    RemoveCommand("/dumpitem");
    RemoveCommand("/dumpspawn");
    RemoveCommand("/dumpprofile");
    RemoveCommand("/dumpwnd");
    RemoveCommand("/dumpitemdef");
    RemoveCommand("/dumpeq");
}
