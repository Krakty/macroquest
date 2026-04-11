#include "pch.h"
#include "MQ2Main.h"

#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace mq {

static bool s_hasDumped = false;

// Recording state
static bool s_recording = false;
static int s_recordCount = 0;
static int s_recordMax = 0;
static int s_recordInterval = 0;  // ticks between dumps
static int s_recordTick = 0;
static char s_recordTag[64] = { 0 };

static bool DumpRegion(const void* base, size_t size, const char* filePath)
{
	FILE* f = nullptr;
	fopen_s(&f, filePath, "wb");
	if (f)
	{
		fwrite(base, 1, size, f);
		fclose(f);
		return true;
	}
	return false;
}

static void DumpSpawn(const char* tag)
{
	PlayerClient* pPlayer = pLocalPlayer;
	if (!pPlayer)
	{
		WriteChatf("\ar[MemDump]\ax Not in game");
		return;
	}

	char filePath[MAX_PATH];
	sprintf_s(filePath, "%s\\memdump_spawn_%s.bin", mq::internal_paths::Logs.c_str(), tag);

	size_t dumpSize = 0x2200;
	if (DumpRegion(pPlayer, dumpSize, filePath))
	{
		WriteChatf("\ag[MemDump]\ax Spawn dumped: 0x%llX (%zu bytes) -> %s",
			(uintptr_t)pPlayer, dumpSize, filePath);
	}
	else
	{
		WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
	}
}

static void DumpCZC(const char* tag)
{
	PlayerClient* pPlayer = pLocalPlayer;
	if (!pPlayer)
	{
		WriteChatf("\ar[MemDump]\ax Not in game");
		return;
	}

	// pCharacter is at offset 0x4F8 from pLocalPlayer
	uintptr_t pCharPtr = *(uintptr_t*)((char*)pPlayer + 0x4F8);
	if (!pCharPtr)
	{
		WriteChatf("\ar[MemDump]\ax pCharacter is null (offset 0x4F8 from pLocalPlayer)");
		return;
	}

	char filePath[MAX_PATH];
	sprintf_s(filePath, "%s\\memdump_czc_%s.bin", mq::internal_paths::Logs.c_str(), tag);

	size_t dumpSize = 0x4000;
	if (DumpRegion((void*)pCharPtr, dumpSize, filePath))
	{
		WriteChatf("\ag[MemDump]\ax CZC dumped: 0x%llX (%zu bytes) -> %s",
			pCharPtr, dumpSize, filePath);
	}
	else
	{
		WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
	}
}

static void DumpActor(const char* tag)
{
	PlayerClient* pPlayer = pLocalPlayer;
	if (!pPlayer)
	{
		WriteChatf("\ar[MemDump]\ax Not in game");
		return;
	}

	// ActorClient is at offset 0xFE0 from pLocalPlayer
	void* pActor = (char*)pPlayer + 0xFE0;

	char filePath[MAX_PATH];
	sprintf_s(filePath, "%s\\memdump_actor_%s.bin", mq::internal_paths::Logs.c_str(), tag);

	size_t dumpSize = 0x300;
	if (DumpRegion(pActor, dumpSize, filePath))
	{
		WriteChatf("\ag[MemDump]\ax Actor dumped: 0x%llX (%zu bytes) -> %s",
			(uintptr_t)pActor, dumpSize, filePath);
	}
	else
	{
		WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
	}
}

static void DumpGroup(const char* tag)
{
	if (!pLocalPC)
	{
		WriteChatf("\ar[MemDump]\ax pLocalPC is null");
		return;
	}

	char filePath[MAX_PATH];
	sprintf_s(filePath, "%s\\memdump_group_%s.bin", mq::internal_paths::Logs.c_str(), tag);

	size_t dumpSize = 0x4000;
	if (DumpRegion(pLocalPC, dumpSize, filePath))
	{
		WriteChatf("\ag[MemDump]\ax Group (PcClient) dumped: 0x%llX (%zu bytes) -> %s",
			(uintptr_t)pLocalPC, dumpSize, filePath);
	}
	else
	{
		WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
	}
}

static void DumpAll(const char* tag)
{
	WriteChatf("\ag[MemDump]\ax Dumping all targets with tag '%s'...", tag);
	DumpSpawn(tag);
	DumpCZC(tag);
	DumpActor(tag);
	DumpGroup(tag);
}

static void DumpEqgame()
{
	HMODULE hModule = GetModuleHandleA("eqgame.exe");
	if (!hModule)
	{
		WriteChatf("\ar[MemDump]\ax Could not find eqgame.exe module");
		return;
	}

	MODULEINFO modInfo = {};
	if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
	{
		char filePath[MAX_PATH];
		sprintf_s(filePath, "%s\\memdump_eqgame.bin", mq::internal_paths::Logs.c_str());

		if (DumpRegion(modInfo.lpBaseOfDll, modInfo.SizeOfImage, filePath))
		{
			WriteChatf("\ag[MemDump]\ax eqgame.exe dumped: 0x%llX (%zu bytes) -> %s",
				(uintptr_t)modInfo.lpBaseOfDll, modInfo.SizeOfImage, filePath);
		}
		else
		{
			WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
		}
	}
}

static void Cmd_MemDump(PlayerClient* pChar, const char* szLine)
{
	char szArg[MAX_STRING] = { 0 };
	char szArg2[MAX_STRING] = { 0 };
	GetArg(szArg, szLine, 1);
	GetArg(szArg2, szLine, 2);

	if (!szArg[0] || !_stricmp(szArg, "help"))
	{
		WriteChatf("\ag[MemDump]\ax Usage:");
		WriteChatf("  /memdump <tag>                - Dump spawn (backward compatible)");
		WriteChatf("  /memdump spawn <tag>          - Dump spawn (PlayerZoneClient, 0x2200)");
		WriteChatf("  /memdump czc <tag>            - Dump CharacterZoneClient (0x4000)");
		WriteChatf("  /memdump actor <tag>          - Dump ActorClient (0x300)");
		WriteChatf("  /memdump group <tag>          - Dump PcClient/pLocalPC (0x4000)");
		WriteChatf("  /memdump all <tag>            - Dump all structs");
		WriteChatf("  /memdump eqgame              - Dump eqgame.exe image");
		WriteChatf("  /memdump record <tag> [sec]   - Record spawn dumps for N seconds (default 10)");
		WriteChatf("  /memdump stop                 - Stop recording");
		return;
	}

	if (!_stricmp(szArg, "eqgame"))
	{
		DumpEqgame();
		return;
	}

	if (!_stricmp(szArg, "record"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump record <tag> [seconds]");
			return;
		}

		char szArg3[MAX_STRING] = { 0 };
		GetArg(szArg3, szLine, 3);
		int seconds = szArg3[0] ? GetIntFromString(szArg3, 10) : 10;
		if (seconds < 1) seconds = 1;
		if (seconds > 60) seconds = 60;

		strcpy_s(s_recordTag, szArg2);
		s_recordCount = 0;
		s_recordMax = seconds * 10;  // ~10 dumps per second (every 10 ticks at 100ms/tick)
		s_recordInterval = 10;       // every 10 ticks
		s_recordTick = 0;
		s_recording = true;

		WriteChatf("\ag[MemDump]\ax Recording '%s' for %d seconds (~%d dumps)",
			s_recordTag, seconds, s_recordMax);
		return;
	}

	if (!_stricmp(szArg, "stop"))
	{
		if (s_recording)
		{
			s_recording = false;
			WriteChatf("\ag[MemDump]\ax Recording stopped. %d dumps captured.", s_recordCount);
		}
		else
		{
			WriteChatf("\ag[MemDump]\ax Not recording.");
		}
		return;
	}

	// Named subcommands that require a tag
	if (!_stricmp(szArg, "spawn"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump spawn <tag>");
			return;
		}
		DumpSpawn(szArg2);
		return;
	}

	if (!_stricmp(szArg, "czc"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump czc <tag>");
			return;
		}
		DumpCZC(szArg2);
		return;
	}

	if (!_stricmp(szArg, "actor"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump actor <tag>");
			return;
		}
		DumpActor(szArg2);
		return;
	}

	if (!_stricmp(szArg, "group"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump group <tag>");
			return;
		}
		DumpGroup(szArg2);
		return;
	}

	if (!_stricmp(szArg, "all"))
	{
		if (!szArg2[0])
		{
			WriteChatf("\ar[MemDump]\ax Need a tag: /memdump all <tag>");
			return;
		}
		DumpAll(szArg2);
		return;
	}

	// Default: treat first arg as tag for spawn dump (backward compatible)
	DumpSpawn(szArg);
}

static void MemDump_Pulse()
{
	// Auto zone-in dump
	if (!s_hasDumped)
	{
		if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
		{
			static int tickCount = 0;
			tickCount++;
			if (tickCount >= 30)
			{
				DumpSpawn("autozonedin");
				DumpEqgame();
				s_hasDumped = true;
			}
		}
	}

	// Timed recording
	if (s_recording && pLocalPlayer)
	{
		s_recordTick++;
		if (s_recordTick >= s_recordInterval)
		{
			s_recordTick = 0;
			char tag[128];
			sprintf_s(tag, "%s_%04d", s_recordTag, s_recordCount);
			DumpSpawn(tag);
			s_recordCount++;

			if (s_recordCount >= s_recordMax)
			{
				s_recording = false;
				WriteChatf("\ag[MemDump]\ax Recording complete. %d dumps captured.", s_recordCount);
			}
		}
	}
}

static void MemDump_Initialize()
{
	AddCommand("/memdump", Cmd_MemDump);
	s_hasDumped = false;
}

static void MemDump_Shutdown()
{
	RemoveCommand("/memdump");
	s_recording = false;
}

static MQModule s_MemDumpModule = {
	"MemDump",
	false,
	MemDump_Initialize,
	MemDump_Shutdown,
	MemDump_Pulse,
};

MQModule* GetMemDumpModule() { return &s_MemDumpModule; }

} // namespace mq
