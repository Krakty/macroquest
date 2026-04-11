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

	FILE* f = nullptr;
	fopen_s(&f, filePath, "wb");
	if (f)
	{
		size_t dumpSize = 0x2200;
		fwrite(pPlayer, 1, dumpSize, f);
		fclose(f);
		WriteChatf("\ag[MemDump]\ax Spawn dumped: %s", filePath);
	}
	else
	{
		WriteChatf("\ar[MemDump]\ax Failed to open %s", filePath);
	}
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

		FILE* f = nullptr;
		fopen_s(&f, filePath, "wb");
		if (f)
		{
			fwrite(modInfo.lpBaseOfDll, 1, modInfo.SizeOfImage, f);
			fclose(f);
			WriteChatf("\ag[MemDump]\ax eqgame.exe dumped: 0x%llX (%zu bytes) -> %s",
				(uintptr_t)modInfo.lpBaseOfDll, modInfo.SizeOfImage, filePath);
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
		WriteChatf("  /memdump <tag>              - Single spawn dump");
		WriteChatf("  /memdump eqgame             - Dump eqgame.exe image");
		WriteChatf("  /memdump record <tag> [sec]  - Record dumps for N seconds (default 10)");
		WriteChatf("  /memdump stop                - Stop recording");
		WriteChatf("  Examples: /memdump falling, /memdump record combat 5");
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
