#include <mq/Plugin.h>
#include <windows.h>
#include <dbghelp.h>
#include <mutex>
#include <vector>
#include <string>
#include <cstdio>

PreSetup("MQ2FieldWatch");
PLUGIN_VERSION(1.0);

// Hardware breakpoint slots (DR0-DR3)
struct WatchSlot
{
    int      slot;        // 0-3
    int      offset;      // PZC offset being watched
    bool     active;
    uint64_t hitCount;
};

static WatchSlot s_slots[4] = {};
static std::mutex s_logMutex;
static FILE* s_logFile = nullptr;
static PVOID s_vehHandle = nullptr;
static bool s_enabled = false;

// Log entry written on each breakpoint hit
struct LogEntry
{
    uint64_t rip;
    int      offset;
    uint32_t oldValue;
    uint32_t newValue;
    uint32_t timestamp;
};

static std::vector<LogEntry> s_log;

// Set a hardware breakpoint on the current thread
static bool SetHWBreakpoint(int slot, void* address, int size)
{
    if (slot < 0 || slot > 3) return false;

    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    HANDLE hThread = GetCurrentThread();
    if (!GetThreadContext(hThread, &ctx)) return false;

    // Set the address in DR0-DR3
    switch (slot)
    {
    case 0: ctx.Dr0 = (DWORD64)address; break;
    case 1: ctx.Dr1 = (DWORD64)address; break;
    case 2: ctx.Dr2 = (DWORD64)address; break;
    case 3: ctx.Dr3 = (DWORD64)address; break;
    }

    // DR7 control bits per slot:
    // Bits 0,2,4,6: local enable for DR0-DR3
    // Bits 16-17, 20-21, 24-25, 28-29: condition (00=exec, 01=write, 11=read/write)
    // Bits 18-19, 22-23, 26-27, 30-31: size (00=1, 01=2, 11=4, 10=8)

    int enableBit = slot * 2;        // local enable
    int condBase  = 16 + slot * 4;   // condition bits
    int sizeBase  = 18 + slot * 4;   // size bits

    // Clear this slot's bits
    ctx.Dr7 &= ~((uint64_t)0x3 << enableBit);
    ctx.Dr7 &= ~((uint64_t)0xF << condBase);

    // Set: local enable, write-only condition (01), 4-byte size (11)
    ctx.Dr7 |= ((uint64_t)1 << enableBit);        // local enable
    ctx.Dr7 |= ((uint64_t)0x1 << condBase);        // condition = write only

    // Size encoding: 00=1byte, 01=2byte, 11=4byte, 10=8byte
    uint64_t sizeCode = 0;
    switch (size)
    {
    case 1: sizeCode = 0; break;
    case 2: sizeCode = 1; break;
    case 4: sizeCode = 3; break;
    case 8: sizeCode = 2; break;
    default: sizeCode = 3; break; // default 4 byte
    }
    ctx.Dr7 |= (sizeCode << sizeBase);

    return SetThreadContext(hThread, &ctx) != 0;
}

static bool ClearHWBreakpoint(int slot)
{
    if (slot < 0 || slot > 3) return false;

    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    HANDLE hThread = GetCurrentThread();
    if (!GetThreadContext(hThread, &ctx)) return false;

    switch (slot)
    {
    case 0: ctx.Dr0 = 0; break;
    case 1: ctx.Dr1 = 0; break;
    case 2: ctx.Dr2 = 0; break;
    case 3: ctx.Dr3 = 0; break;
    }

    int enableBit = slot * 2;
    int condBase = 16 + slot * 4;
    ctx.Dr7 &= ~((uint64_t)0x3 << enableBit);
    ctx.Dr7 &= ~((uint64_t)0xF << condBase);

    return SetThreadContext(hThread, &ctx) != 0;
}

// Vectored Exception Handler -- fires on hardware breakpoint hit
static LONG CALLBACK VEHHandler(PEXCEPTION_POINTERS pExInfo)
{
    if (pExInfo->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
        return EXCEPTION_CONTINUE_SEARCH;

    // Check DR6 to see which breakpoint fired
    CONTEXT* ctx = pExInfo->ContextRecord;
    uint64_t dr6 = ctx->Dr6;

    for (int i = 0; i < 4; i++)
    {
        if ((dr6 & (1ULL << i)) && s_slots[i].active)
        {
            s_slots[i].hitCount++;

            // Read the current value at the watched address
            uint32_t currentVal = 0;
            if (pLocalPlayer)
            {
                void* addr = (char*)pLocalPlayer.get() + s_slots[i].offset;
                currentVal = *(uint32_t*)addr;
            }

            // Get EQ module base for relative address
            HMODULE hEQ = GetModuleHandleA("eqgame.exe");
            uint64_t rip = ctx->Rip;
            uint64_t relRip = hEQ ? (rip - (uint64_t)hEQ) : rip;

            // Log it
            {
                std::lock_guard lock(s_logMutex);
                if (s_logFile)
                {
                    fprintf(s_logFile, "HIT slot=%d offset=0x%03X rip=0x%llX (eqgame+0x%llX) value=0x%08X hits=%llu\n",
                        i, s_slots[i].offset, rip, relRip, currentVal, s_slots[i].hitCount);
                    fflush(s_logFile);
                }
            }

            // Clear DR6 for this slot
            ctx->Dr6 &= ~(1ULL << i);

            // Resume execution (RF flag to avoid re-triggering)
            ctx->EFlags |= 0x10000; // RF = Resume Flag
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// /fieldwatch watch <offset> [size] -- set a watch on PZC+offset
// /fieldwatch clear <slot>          -- clear a slot
// /fieldwatch list                  -- show active watches
// /fieldwatch stop                  -- clear all and disable
static void Cmd_FieldWatch(PlayerClient* pChar, const char* szLine)
{
    char szArg[MAX_STRING] = { 0 };
    char szArg2[MAX_STRING] = { 0 };
    char szArg3[MAX_STRING] = { 0 };
    GetArg(szArg, szLine, 1);
    GetArg(szArg2, szLine, 2);
    GetArg(szArg3, szLine, 3);

    if (!szArg[0] || !_stricmp(szArg, "help"))
    {
        WriteChatf("\ag[FieldWatch]\ax Usage:");
        WriteChatf("  /fieldwatch watch <hex_offset> [size]  - Watch PZC+offset (size: 1,2,4,8; default 4)");
        WriteChatf("  /fieldwatch clear <slot>               - Clear slot 0-3");
        WriteChatf("  /fieldwatch list                       - Show active watches");
        WriteChatf("  /fieldwatch stop                       - Clear all watches");
        WriteChatf("  Log output: MacroQuest/Logs/fieldwatch.log");
        return;
    }

    if (!_stricmp(szArg, "watch"))
    {
        if (!pLocalPlayer)
        {
            WriteChatf("\ar[FieldWatch]\ax Not in game");
            return;
        }

        if (!szArg2[0])
        {
            WriteChatf("\ar[FieldWatch]\ax Need offset: /fieldwatch watch 0x1DC");
            return;
        }

        int offset = (int)strtol(szArg2, nullptr, 0);
        int size = szArg3[0] ? (int)strtol(szArg3, nullptr, 0) : 4;

        // Find a free slot
        int freeSlot = -1;
        for (int i = 0; i < 4; i++)
        {
            if (!s_slots[i].active) { freeSlot = i; break; }
        }

        if (freeSlot < 0)
        {
            WriteChatf("\ar[FieldWatch]\ax All 4 slots in use. Clear one first.");
            return;
        }

        void* addr = (char*)pLocalPlayer.get() + offset;

        if (SetHWBreakpoint(freeSlot, addr, size))
        {
            s_slots[freeSlot].slot = freeSlot;
            s_slots[freeSlot].offset = offset;
            s_slots[freeSlot].active = true;
            s_slots[freeSlot].hitCount = 0;

            WriteChatf("\ag[FieldWatch]\ax Slot %d: watching PZC+0x%03X (%d bytes) at 0x%llX",
                freeSlot, offset, size, (uint64_t)addr);
        }
        else
        {
            WriteChatf("\ar[FieldWatch]\ax Failed to set hardware breakpoint (slot %d)", freeSlot);
        }
        return;
    }

    if (!_stricmp(szArg, "clear"))
    {
        int slot = szArg2[0] ? atoi(szArg2) : -1;
        if (slot < 0 || slot > 3)
        {
            WriteChatf("\ar[FieldWatch]\ax Invalid slot (0-3)");
            return;
        }

        ClearHWBreakpoint(slot);
        s_slots[slot].active = false;
        WriteChatf("\ag[FieldWatch]\ax Slot %d cleared", slot);
        return;
    }

    if (!_stricmp(szArg, "list"))
    {
        WriteChatf("\ag[FieldWatch]\ax Active watches:");
        for (int i = 0; i < 4; i++)
        {
            if (s_slots[i].active)
            {
                WriteChatf("  Slot %d: PZC+0x%03X  hits=%llu", i, s_slots[i].offset, s_slots[i].hitCount);
            }
            else
            {
                WriteChatf("  Slot %d: (empty)", i);
            }
        }
        return;
    }

    if (!_stricmp(szArg, "stop"))
    {
        for (int i = 0; i < 4; i++)
        {
            if (s_slots[i].active)
            {
                ClearHWBreakpoint(i);
                s_slots[i].active = false;
            }
        }
        WriteChatf("\ag[FieldWatch]\ax All watches cleared");
        return;
    }

    WriteChatf("\ar[FieldWatch]\ax Unknown subcommand: %s", szArg);
}

PLUGIN_API void InitializePlugin()
{
    // Open log file
    char logPath[MAX_PATH];
    sprintf_s(logPath, "%s\\Logs\\fieldwatch.log", gPathMQRoot);
    fopen_s(&s_logFile, logPath, "a");

    if (s_logFile)
    {
        fprintf(s_logFile, "\n=== FieldWatch session started ===\n");
        fflush(s_logFile);
    }

    // Install VEH (first handler so we get priority)
    s_vehHandle = AddVectoredExceptionHandler(1, VEHHandler);

    AddCommand("/fieldwatch", Cmd_FieldWatch);

    memset(s_slots, 0, sizeof(s_slots));
    s_enabled = true;

    WriteChatf("\ag[FieldWatch]\ax Loaded. Use /fieldwatch watch <offset> to set hardware breakpoints.");
}

PLUGIN_API void ShutdownPlugin()
{
    // Clear all breakpoints
    for (int i = 0; i < 4; i++)
    {
        if (s_slots[i].active)
            ClearHWBreakpoint(i);
    }

    if (s_vehHandle)
    {
        RemoveVectoredExceptionHandler(s_vehHandle);
        s_vehHandle = nullptr;
    }

    RemoveCommand("/fieldwatch");

    if (s_logFile)
    {
        fprintf(s_logFile, "=== FieldWatch session ended ===\n");
        fclose(s_logFile);
        s_logFile = nullptr;
    }

    s_enabled = false;
}
