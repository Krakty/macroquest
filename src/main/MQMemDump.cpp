#include "pch.h"
#include "MQ2Main.h"

#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace mq {

static bool s_hasDumped = false;

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
		WriteChatf("\ag[MemDump]\ax Spawn dumped: %s (%zu bytes)", filePath, dumpSize);
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
	GetArg(szArg, szLine, 1);

	if (!szArg[0] || !_stricmp(szArg, "help"))
	{
		WriteChatf("\ag[MemDump]\ax Usage:");
		WriteChatf("  /memdump <tag>     - Dump spawn as memdump_spawn_<tag>.bin");
		WriteChatf("  /memdump eqgame    - Dump eqgame.exe image");
		WriteChatf("  Examples: /memdump afkon, /memdump sitting, /memdump baseline");
		return;
	}

	if (!_stricmp(szArg, "eqgame"))
	{
		DumpEqgame();
		return;
	}

	DumpSpawn(szArg);
}

static void MemDump_Pulse()
{
	if (s_hasDumped)
		return;

	if (GetGameState() != GAMESTATE_INGAME)
		return;

	if (!pLocalPlayer)
		return;

	static int tickCount = 0;
	tickCount++;
	if (tickCount < 30)
		return;

	DumpSpawn("autozonedin");
	DumpEqgame();
	s_hasDumped = true;
}

static void MemDump_Initialize()
{
	AddCommand("/memdump", Cmd_MemDump);
	s_hasDumped = false;
}

static void MemDump_Shutdown()
{
	RemoveCommand("/memdump");
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
