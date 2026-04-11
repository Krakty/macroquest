#include "pch.h"

#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

#pragma comment(lib, "Psapi.lib")

static bool s_hasDumped = false;

static void DoMemDump()
{
	char filePath[MAX_PATH];

	// Dump player spawn
	PlayerClient* pPlayer = pLocalPlayer;
	if (pPlayer)
	{
		sprintf_s(filePath, "%s\\memdump_spawn.bin", gPathLogs);

		FILE* f = nullptr;
		fopen_s(&f, filePath, "wb");
		if (f)
		{
			size_t dumpSize = 0x2200;
			fwrite(pPlayer, 1, dumpSize, f);
			fclose(f);
			WriteChatf("\ag[MemDump]\ax Spawn dumped: 0x%llX (%zu bytes) -> %s",
				(uintptr_t)pPlayer, dumpSize, filePath);
		}
	}

	// Dump eqgame.exe
	HMODULE hModule = GetModuleHandleA("eqgame.exe");
	if (hModule)
	{
		MODULEINFO modInfo = {};
		if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		{
			sprintf_s(filePath, "%s\\memdump_eqgame.bin", gPathLogs);

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
}

static void Cmd_MemDump(SPAWNINFO* pChar, char* szLine)
{
	s_hasDumped = false;
	DoMemDump();
	s_hasDumped = true;
}

static void MemDump_Pulse()
{
	if (s_hasDumped)
		return;

	if (GetGameState() != GAMESTATE_INGAME)
		return;

	if (!pLocalPlayer)
		return;

	// Wait a moment after zoning for data to populate
	static int tickCount = 0;
	tickCount++;
	if (tickCount < 30)
		return;

	DoMemDump();
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
