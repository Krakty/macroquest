/*
 * MQMCPServer - Model Context Protocol server for MacroQuest
 *
 * Exposes EverQuest game state as MCP resources and accepts tool calls
 * to control the character. Runs an HTTP server on localhost:8284.
 *
 * MCP endpoint:  POST http://localhost:8284/mcp  (JSON-RPC 2.0)
 * Debug REST:    GET  http://localhost:8284/me|zone|target|spawns
 */

// winsock2.h must be included before windows.h (which mq/Plugin.h pulls in)
#include <winsock2.h>
#include <ws2tcpip.h>

#include <mq/Plugin.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#define CPPHTTPLIB_NO_EXCEPTIONS
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#include <httplib.h>

PreSetup("MQMCPServer");
PLUGIN_VERSION(1.0);

using json = nlohmann::json;

static constexpr int k_defaultPort = 8284;
static int s_port = k_defaultPort;
static std::chrono::steady_clock::time_point s_startTime;

//============================================================================
// Thread-safe command queue — written by HTTP thread, drained on game pulse
//============================================================================

static std::queue<std::string> s_commandQueue;
static std::mutex              s_queueMutex;

static void EnqueueCommand(std::string cmd)
{
	std::lock_guard lock(s_queueMutex);
	s_commandQueue.push(std::move(cmd));
}

//============================================================================
// Chat ring buffer — fed by OnWriteChatColor, read by HTTP thread
//============================================================================

static constexpr size_t k_chatBufferSize = 100;
static std::deque<std::string> s_chatLines;
static std::mutex              s_chatMutex;

static void PushChatLine(const char* line)
{
	// Strip MQ color codes (0x07 + letter) and non-ASCII bytes so the
	// result is valid UTF-8 and safe to embed in JSON.
	std::string clean;
	for (const char* p = line; *p; ++p)
	{
		unsigned char c = static_cast<unsigned char>(*p);
		if (c == 0x07 && *(p + 1)) { ++p; continue; }  // MQ color: skip 2 bytes
		if (c >= 0x80) continue;                        // non-ASCII: skip
		clean += *p;
	}

	std::lock_guard lock(s_chatMutex);
	s_chatLines.push_back(std::move(clean));
	if (s_chatLines.size() > k_chatBufferSize)
		s_chatLines.pop_front();
}

//============================================================================
// Cached game state — written on pulse (main thread), read by HTTP thread
//============================================================================

struct SpawnSnapshot
{
	int         id;
	std::string name;
	std::string type;
	int         level;
	float       x, y, z;
	float       distance;
};

struct GameSnapshot
{
	bool inGame = false;

	// Player
	std::string playerName;
	std::string playerClass;
	int         playerLevel   = 0;
	int         playerClassId = 0;
	float       playerX = 0, playerY = 0, playerZ = 0;
	bool        isMoving      = false;   // true if position changed since last snapshot
	bool        isInvisible   = false;
	int64_t     hp = 0,  hpMax = 0;
	int         mana = 0, manaMax = 0;
	int         endurance = 0, enduranceMax = 0;

	// Zone
	std::string zoneLongName;
	std::string zoneShortName;
	int         zoneId = 0;

	// Target
	bool        hasTarget   = false;
	std::string targetName;
	int         targetId    = 0;
	std::string targetType;
	float       targetDist  = 0.f;
	int64_t     targetHp    = 0;
	int64_t     targetHpMax = 0;
	int         targetLevel = 0;

	// Nearby spawns (within 300 units, capped at 60, sorted by distance)
	std::vector<SpawnSnapshot> spawns;

	// AdvLoot items available to loot
	struct LootEntry {
		std::string name;
		bool        noDrop    = false;
		bool        stackable = false;
		int         stackSize = 1;
		bool        personal  = false;  // true = personal loot, false = shared
	};
	std::vector<LootEntry> lootItems;

	// Memorized spells (gem slots 0-based)
	struct SpellGem {
		int         gem     = 0;
		int         spellId = -1;
		std::string name;
	};
	std::vector<SpellGem> memSpells;

	// Extended targets (XTarget window) — active aggressors/haters
	struct XTargetEntry {
		std::string name;
		int         spawnId  = 0;
		float       distance = 0.f;
		int         level    = 0;
	};
	std::vector<XTargetEntry> xTargets;   // only occupied, in-zone slots
	int xTargetCount = 0;                 // total occupied slots

	// Group members
	struct GroupMember {
		std::string name;
		std::string ownerName;   // mercenary owner
		int         level    = 0;
		int         type     = 0;  // EQP_PC=0, mercenary etc
		bool        offline  = false;
		bool        isLeader = false;
		int         hpPct    = 0;  // 0 if no spawn data
		int         manaPct  = 0;
	};
	std::vector<GroupMember> groupMembers;

	// Active buffs on self (long buffs + songs/disciplines)
	struct BuffEntry {
		int         spellId  = 0;
		std::string name;
		int         duration = 0;   // remaining ticks
		bool        song     = false;
	};
	std::vector<BuffEntry> buffs;

	// Visible buffs/debuffs on current target
	std::vector<BuffEntry> targetBuffs;
};

static GameSnapshot s_snapshot;
static std::mutex   s_snapshotMutex;

static const char* SpawnTypeName(int type)
{
	switch (type)
	{
	case SPAWN_PLAYER: return "PC";
	case SPAWN_NPC:    return "NPC";
	case SPAWN_CORPSE: return "Corpse";
	default:           return "Unknown";
	}
}

static void UpdateSnapshot()
{
	GameSnapshot snap;

	if (GetGameState() != GAMESTATE_INGAME || !pLocalPlayer)
	{
		std::lock_guard lock(s_snapshotMutex);
		s_snapshot = snap;
		return;
	}

	snap.inGame = true;

	// ---- Player ----
	snap.playerName    = pLocalPlayer->Name;
	snap.playerLevel   = pLocalPlayer->Level;
	snap.playerClassId = static_cast<int>(pLocalPlayer->GetClass());
	if (const char* cls = pLocalPlayer->GetClassString())
		snap.playerClass = cls;
	snap.playerX       = pLocalPlayer->X;
	snap.playerY       = pLocalPlayer->Y;
	snap.playerZ       = pLocalPlayer->Z;
	snap.isMoving      = (snap.playerX != s_snapshot.playerX ||
	                      snap.playerY != s_snapshot.playerY ||
	                      snap.playerZ != s_snapshot.playerZ);
	snap.isInvisible   = (pLocalPlayer->HideMode != 0);
	snap.hp            = pLocalPlayer->HPCurrent;
	snap.hpMax         = pLocalPlayer->HPMax;
	snap.mana          = GetCurMana();
	snap.manaMax       = GetMaxMana();
	snap.endurance     = GetCurEndurance();
	snap.enduranceMax  = GetMaxEndurance();

	// ---- Zone ----
	if (pZoneInfo)
	{
		snap.zoneLongName  = pZoneInfo->LongName;
		snap.zoneShortName = pZoneInfo->ShortName;
		snap.zoneId        = pZoneInfo->ZoneID & 0x7FFF;
	}

	// ---- Target ----
	if (pTarget)
	{
		snap.hasTarget   = true;
		snap.targetName  = pTarget->Name;
		snap.targetId    = pTarget->SpawnID;
		snap.targetLevel = pTarget->Level;
		snap.targetHp    = pTarget->HPCurrent;
		snap.targetHpMax = pTarget->HPMax;
		snap.targetType  = SpawnTypeName(pTarget->Type);
		snap.targetDist  = Distance3DToSpawn(pLocalPlayer, pTarget);
	}

	// ---- Nearby spawns ----
	for (SPAWNINFO* pSpawn = (SPAWNINFO*)pSpawnList; pSpawn; pSpawn = pSpawn->pNext)
	{
		if (pSpawn == pLocalPlayer)
			continue;

		float dist = Distance3DToSpawn(pLocalPlayer, pSpawn);
		if (dist > 300.f)
			continue;

		SpawnSnapshot ss;
		ss.id       = pSpawn->SpawnID;
		ss.name     = pSpawn->Name;
		ss.level    = pSpawn->Level;
		ss.type     = SpawnTypeName(pSpawn->Type);
		ss.x        = pSpawn->X;
		ss.y        = pSpawn->Y;
		ss.z        = pSpawn->Z;
		ss.distance = dist;

		snap.spawns.push_back(ss);
	}

	std::sort(snap.spawns.begin(), snap.spawns.end(),
	          [](const SpawnSnapshot& a, const SpawnSnapshot& b) { return a.distance < b.distance; });

	if (snap.spawns.size() > 60)
		snap.spawns.resize(60);

	// ---- AdvLoot items ----
	if (pAdvancedLootWnd)
	{
		auto collectItems = [&](AdvancedLootItemList* list, bool personal) {
			if (!list) return;
			int count = list->Items.GetSize();
			for (int i = 0; i < count; ++i)
			{
				const AdvancedLootItem& li = list->Items[i];
				if (li.Name[0] == '\0') continue;

				GameSnapshot::LootEntry entry;
				entry.name      = li.Name;
				entry.noDrop    = li.NoDrop;
				entry.stackable = li.bStackable;
				entry.stackSize = li.bStackable ? li.MaxStack : 1;
				entry.personal  = personal;
				snap.lootItems.push_back(entry);
			}
		};

		collectItems(pAdvancedLootWnd->pPLootList, true);
		collectItems(pAdvancedLootWnd->pCLootList, false);
	}

	// ---- Memorized spells ----
	if (PcProfile* pProfile = GetPcProfile())
	{
		for (int gem = 0; gem < MAX_MEMORIZED_SPELLS; ++gem)
		{
			int spellId = pProfile->GetMemorizedSpell(gem);
			if (spellId <= 0)
				continue;

			GameSnapshot::SpellGem sg;
			sg.gem     = gem;
			sg.spellId = spellId;

			if (EQ_Spell* pSpell = GetSpellByID(spellId))
				sg.name = pSpell->Name;

			snap.memSpells.push_back(sg);
		}
	}

	// ---- Group members ----
	if (pLocalPC && pLocalPC->Group)
	{
		CGroup* pGrp = pLocalPC->Group;
		for (int i = 0; i < (int)pGrp->GetMaxGroupSize(); ++i)
		{
			CGroupMember* pMem = pGrp->GetGroupMember(i);
			if (!pMem || pMem->Name.empty()) continue;

			GameSnapshot::GroupMember gm;
			gm.name      = pMem->Name.c_str();
			gm.ownerName = pMem->OwnerName.c_str();
			gm.level     = pMem->Level;
			gm.type      = pMem->Type;
			gm.offline   = pMem->bIsOffline;
			gm.isLeader  = (pMem == pGrp->GetGroupLeader());

			if (PlayerClient* pSpawn = pMem->pPlayer)
			{
				int64_t hp  = pSpawn->HPCurrent, hpMax = pSpawn->HPMax;
				gm.hpPct   = hpMax > 0 ? static_cast<int>(hp * 100 / hpMax) : 0;
				int mana = 0, manaMax = 0;
				if (pMem->pPlayer == pLocalPlayer)
				{
					mana    = GetCurMana();
					manaMax = GetMaxMana();
				}
				else
				{
					mana    = pSpawn->GetCurrentMana();
					manaMax = pSpawn->GetMaxMana();
				}
				gm.manaPct = manaMax > 0 ? mana * 100 / manaMax : 0;
			}

			snap.groupMembers.push_back(gm);
		}
	}

	// ---- Self buffs (long + songs/disciplines) ----
	if (pLocalPC)
	{
		auto collectBuffs = [&](auto&& getEffect, int count, bool isSong) {
			for (int i = 0; i < count; ++i)
			{
				const EQ_Affect& affect = getEffect(i);
				if (affect.SpellID <= 0) continue;

				GameSnapshot::BuffEntry entry;
				entry.spellId  = affect.SpellID;
				entry.duration = affect.Duration;
				entry.song     = isSong;

				if (EQ_Spell* pSpell = GetSpellByID(affect.SpellID))
					entry.name = pSpell->Name;

				snap.buffs.push_back(entry);
			}
		};

		collectBuffs([&](int i) -> const EQ_Affect& { return pLocalPC->GetEffect(i); },    NUM_LONG_BUFFS, false);
		collectBuffs([&](int i) -> const EQ_Affect& { return pLocalPC->GetTempEffect(i); }, NUM_TEMP_BUFFS, true);
	}

	// ---- Target buffs (visible effects on target, PC targets only) ----
	if (pTarget && pTarget->Type == SPAWN_PLAYER)
	{
		// Only PC targets have a PcClient buff list accessible this way
		if (PcClient* pTargetPC = pTarget->GetPcClient())
		{
			for (int i = 0; i < NUM_LONG_BUFFS; ++i)
			{
				const EQ_Affect& affect = pTargetPC->GetEffect(i);
				if (affect.SpellID <= 0) continue;

				GameSnapshot::BuffEntry entry;
				entry.spellId  = affect.SpellID;
				entry.duration = affect.Duration;
				entry.song     = false;

				if (EQ_Spell* pSpell = GetSpellByID(affect.SpellID))
					entry.name = pSpell->Name;

				snap.targetBuffs.push_back(entry);
			}
		}
	}

	// ---- Extended targets (XTarget window) ----
	if (pLocalPC && pLocalPC->pXTargetMgr)
	{
		ExtendedTargetList* xtm = pLocalPC->pXTargetMgr;
		int numSlots = xtm->GetNumSlots();
		for (int i = 0; i < numSlots; ++i)
		{
			ExtendedTargetSlot* xts = xtm->GetSlot(i);
			if (!xts || xts->XTargetSlotStatus != eXTSlotCurrentZone || xts->SpawnID == 0)
				continue;

			++snap.xTargetCount;

			GameSnapshot::XTargetEntry entry;
			entry.spawnId = xts->SpawnID;
			entry.name    = xts->Name;

			if (SPAWNINFO* pSpawn = GetSpawnByID(xts->SpawnID))
			{
				entry.level    = pSpawn->Level;
				entry.distance = Distance3DToSpawn(pLocalPlayer, pSpawn);
			}

			snap.xTargets.push_back(entry);
		}
	}

	std::lock_guard lock(s_snapshotMutex);
	s_snapshot = std::move(snap);
}

//============================================================================
// JSON builders
//============================================================================

static json BuildPlayerJson(const GameSnapshot& s)
{
	int hpPct = s.hpMax  > 0 ? static_cast<int>(s.hp  * 100 / s.hpMax)  : 0;
	int mnPct = s.manaMax > 0 ? s.mana * 100 / s.manaMax : 0;

	return {
		{"name",        s.playerName},
		{"level",       s.playerLevel},
		{"classId",     s.playerClassId},
		{"invisible",   s.isInvisible},
		{"position",    {{"x", s.playerX}, {"y", s.playerY}, {"z", s.playerZ}}},
		{"hp",          {{"current", s.hp},        {"max", s.hpMax},       {"pct", hpPct}}},
		{"mana",        {{"current", s.mana},       {"max", s.manaMax},     {"pct", mnPct}}},
		{"endurance",   {{"current", s.endurance},  {"max", s.enduranceMax}}}
	};
}

static json BuildZoneJson(const GameSnapshot& s)
{
	return {
		{"longName",  s.zoneLongName},
		{"shortName", s.zoneShortName},
		{"id",        s.zoneId}
	};
}

static json BuildTargetJson(const GameSnapshot& s)
{
	if (!s.hasTarget)
		return nullptr;

	json tBuffs = json::array();
	for (const GameSnapshot::BuffEntry& b : s.targetBuffs)
		tBuffs.push_back({{"name", b.name}, {"id", b.spellId}, {"duration", b.duration}});

	return {
		{"name",     s.targetName},
		{"id",       s.targetId},
		{"type",     s.targetType},
		{"level",    s.targetLevel},
		{"distance", s.targetDist},
		{"hp",       {{"current", s.targetHp}, {"max", s.targetHpMax}}},
		{"buffs",    tBuffs}
	};
}

// Compact state summary returned alongside every tool response so the LLM
// always has situational awareness after acting.
static json BuildStateJson(const GameSnapshot& s)
{
	if (!s.inGame)
		return {{"inGame", false}};

	int hpPct  = s.hpMax  > 0 ? static_cast<int>(s.hp  * 100 / s.hpMax)  : 0;
	int mnPct  = s.manaMax > 0 ? s.mana * 100 / s.manaMax : 0;
	int endPct = s.enduranceMax > 0 ? s.endurance * 100 / s.enduranceMax : 0;

	// Extended targets (confirmed aggressors in XTarget window)
	json xtargets = json::array();
	for (const GameSnapshot::XTargetEntry& xt : s.xTargets)
		xtargets.push_back({{"name", xt.name}, {"level", xt.level}, {"distance", xt.distance}});

	// Recent chat (all buffered lines, up to k_chatBufferSize)
	json chat = json::array();
	{
		std::lock_guard chatLock(s_chatMutex);
		for (const std::string& line : s_chatLines)
			chat.push_back(line);
	}

	// Memorized spells + recast readiness
	json spells = json::array();
	for (const GameSnapshot::SpellGem& sg : s.memSpells)
	{
		uint32_t msRemaining = GetSpellGemTimer(sg.gem);
		spells.push_back({{"gem", sg.gem}, {"name", sg.name}, {"id", sg.spellId},
		                  {"ready", msRemaining == 0}, {"ms_remaining", msRemaining}});
	}

	// Loot available
	json loot = json::array();
	for (const GameSnapshot::LootEntry& le : s.lootItems)
		loot.push_back({{"name", le.name}, {"noDrop", le.noDrop}, {"personal", le.personal}});

	// Active buffs on self
	json buffs = json::array();
	for (const GameSnapshot::BuffEntry& b : s.buffs)
		buffs.push_back({{"name", b.name}, {"id", b.spellId}, {"duration", b.duration}, {"song", b.song}});

	json state = {
		{"inGame",         true},
		{"name",           s.playerName},
		{"level",          s.playerLevel},
		{"class",          s.playerClass},
		{"hp_pct",         hpPct},
		{"mana_pct",       mnPct},
		{"endurance_pct",  endPct},
		{"moving",         s.isMoving},
		{"invisible",      s.isInvisible},
		{"position",       {{"x", s.playerX}, {"y", s.playerY}, {"z", s.playerZ}}},
		{"zone",           s.zoneShortName},
		{"group_size",     static_cast<int>(s.groupMembers.size())},
		{"aggro_count",    s.xTargetCount},
		{"aggressors",     xtargets},
		{"mem_spells",     spells},
		{"active_buffs",   buffs},
		{"loot_available", loot},
		{"recent_chat",    chat}
	};

	if (s.hasTarget)
	{
		int tHpPct = s.targetHpMax > 0 ? static_cast<int>(s.targetHp * 100 / s.targetHpMax) : 0;
		json tBuffs = json::array();
		for (const GameSnapshot::BuffEntry& b : s.targetBuffs)
			tBuffs.push_back({{"name", b.name}, {"id", b.spellId}});
		state["target"] = {
			{"name",     s.targetName},
			{"type",     s.targetType},
			{"distance", s.targetDist},
			{"hp_pct",   tHpPct},
			{"buffs",    tBuffs}
		};
	}

	return state;
}

static json BuildSpawnsJson(const GameSnapshot& s)
{
	json arr = json::array();
	for (const SpawnSnapshot& sp : s.spawns)
	{
		arr.push_back({
			{"id",       sp.id},
			{"name",     sp.name},
			{"type",     sp.type},
			{"level",    sp.level},
			{"distance", sp.distance},
			{"position", {{"x", sp.x}, {"y", sp.y}, {"z", sp.z}}}
		});
	}
	return arr;
}

//============================================================================
// MCP JSON-RPC 2.0 handler
//============================================================================

static json MakeOk(const json& id, json result)
{
	return {{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

static json MakeError(const json& id, int code, std::string msg)
{
	return {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", std::move(msg)}}}};
}

static json HandleMCP(const json& req)
{
	const std::string method = req.value("method", "");
	const json        id     = req.contains("id") ? req.at("id") : json(nullptr);

	// Notifications have no id and require no response
	if (id.is_null() && method.find("notifications/") == 0)
		return {};

	if (method == "initialize")
	{
		return MakeOk(id, {
			{"protocolVersion", "2024-11-05"},
			{"capabilities",    {{"resources", json::object()}, {"tools", json::object()}}},
			{"serverInfo",      {{"name", "MQMCPServer"}, {"version", "1.0"}}}
		});
	}

	if (method == "resources/list")
	{
		return MakeOk(id, {{"resources", {
			{{"uri","eq://me"},     {"name","Player State"},    {"description","Your character's current state"},          {"mimeType","application/json"}},
			{{"uri","eq://zone"},   {"name","Zone Info"},       {"description","Current zone name and ID"},                {"mimeType","application/json"}},
			{{"uri","eq://target"}, {"name","Current Target"},  {"description","Your currently selected target"},          {"mimeType","application/json"}},
			{{"uri","eq://spawns"}, {"name","Nearby Spawns"},   {"description","All spawns within 300 units"},             {"mimeType","application/json"}},
			{{"uri","eq://spells"}, {"name","Memorized Spells"},{"description","Spells currently memorized in gem slots"},    {"mimeType","application/json"}},
			{{"uri","eq://chat"},   {"name","Recent Chat"},     {"description","Last 100 lines of chat and MQ output"},      {"mimeType","application/json"}},
			{{"uri","eq://loot"},   {"name","Available Loot"},  {"description","Items currently in the AdvLoot window"},          {"mimeType","application/json"}},
			{{"uri","eq://group"},     {"name","Group Members"},   {"description","Current group members with HP, level, and role"},             {"mimeType","application/json"}},
			{{"uri","eq://buffs"},     {"name","Active Buffs"},    {"description","Buffs and songs currently active on your character"},         {"mimeType","application/json"}},
			{{"uri","eq://cooldowns"}, {"name","Spell Cooldowns"}, {"description","Spell gem recast timers — which gems are ready vs on cooldown"}, {"mimeType","application/json"}}
		}}});
	}

	if (method == "resources/read")
	{
		const std::string uri = req.value("/params/uri"_json_pointer, "");

		std::lock_guard lock(s_snapshotMutex);
		if (!s_snapshot.inGame)
			return MakeError(id, -32000, "Not in game");

		json content;
		if      (uri == "eq://me")     content = BuildPlayerJson(s_snapshot);
		else if (uri == "eq://zone")   content = BuildZoneJson(s_snapshot);
		else if (uri == "eq://target") content = BuildTargetJson(s_snapshot);
		else if (uri == "eq://spawns") content = BuildSpawnsJson(s_snapshot);
		else if (uri == "eq://spells")
		{
			json arr = json::array();
			for (const GameSnapshot::SpellGem& sg : s_snapshot.memSpells)
				arr.push_back({{"gem", sg.gem}, {"name", sg.name}, {"id", sg.spellId}});
			content = arr;
		}
		else if (uri == "eq://chat")
		{
			json arr = json::array();
			std::lock_guard chatLock(s_chatMutex);
			for (const std::string& line : s_chatLines)
				arr.push_back(line);
			content = arr;
		}
		else if (uri == "eq://loot")
		{
			json arr = json::array();
			for (const GameSnapshot::LootEntry& le : s_snapshot.lootItems)
				arr.push_back({
					{"name",      le.name},
					{"noDrop",    le.noDrop},
					{"stackable", le.stackable},
					{"stackSize", le.stackSize},
					{"personal",  le.personal}
				});
			content = arr;
		}
		else if (uri == "eq://group")
		{
			json arr = json::array();
			for (const GameSnapshot::GroupMember& gm : s_snapshot.groupMembers)
				arr.push_back({
					{"name",     gm.name},
					{"owner",    gm.ownerName},
					{"level",    gm.level},
					{"type",     gm.type},
					{"offline",  gm.offline},
					{"leader",   gm.isLeader},
					{"hp_pct",   gm.hpPct},
					{"mana_pct", gm.manaPct}
				});
			content = arr;
		}
		else if (uri == "eq://buffs")
		{
			json arr = json::array();
			for (const GameSnapshot::BuffEntry& b : s_snapshot.buffs)
				arr.push_back({
					{"name",     b.name},
					{"id",       b.spellId},
					{"duration", b.duration},
					{"song",     b.song}
				});
			content = arr;
		}
		else if (uri == "eq://cooldowns")
		{
			json arr = json::array();
			for (const GameSnapshot::SpellGem& sg : s_snapshot.memSpells)
			{
				uint32_t msRemaining = GetSpellGemTimer(sg.gem);
				arr.push_back({
					{"gem",          sg.gem},
					{"name",         sg.name},
					{"id",           sg.spellId},
					{"ready",        msRemaining == 0},
					{"ms_remaining", msRemaining}
				});
			}
			content = arr;
		}
		else return MakeError(id, -32602, "Unknown resource URI: " + uri);

		return MakeOk(id, {{"contents", {{
			{"uri",      uri},
			{"mimeType", "application/json"},
			{"text",     content.dump()}
		}}}});
	}

	if (method == "tools/list")
	{
		return MakeOk(id, {{"tools", {
			{
				{"name",        "get_state"},
				{"description", "Get current game state: name, level, class, HP%, mana%, endurance%, movement, zone, target, buffs, spells, loot, and recent chat. Call this after any action to check the result."},
				{"inputSchema", {{"type","object"},{"properties",json::object()}}}
			},
			{
				{"name",        "execute_command"},
				{"description", "Execute any MacroQuest slash command (e.g. /say hello, /cast \"Complete Heal\")"},
				{"inputSchema", {
					{"type",       "object"},
					{"properties", {{"command", {{"type","string"},{"description","The full command including the leading slash"}}}}},
					{"required",   {"command"}}
				}}
			},
			{
				{"name",        "navigate_to"},
				{"description", "Navigate to a world location using MQ2Nav. Requires MQ2Nav to be loaded."},
				{"inputSchema", {
					{"type",       "object"},
					{"properties", {
						{"y", {{"type","number"},{"description","North/South (Y) coordinate"}}},
						{"x", {{"type","number"},{"description","East/West (X) coordinate"}}},
						{"z", {{"type","number"},{"description","Up/Down (Z) coordinate, optional"}}}
					}},
					{"required",   {"y", "x"}}
				}}
			},
			{
				{"name",        "say"},
				{"description", "Say text in local /say chat"},
				{"inputSchema", {
					{"type",       "object"},
					{"properties", {{"text", {{"type","string"},{"description","Text to say"}}}}},
					{"required",   {"text"}}
				}}
			}
		}}});
	}

	if (method == "tools/call")
	{
		const std::string tool = req.value("/params/name"_json_pointer, "");
		const json        args = req.value("/params/arguments"_json_pointer, json::object());

		// Helper: build a response that includes current game state
		auto respondWithState = [&](std::string msg) {
			std::lock_guard lock(s_snapshotMutex);
			json state = BuildStateJson(s_snapshot);
			std::string text = msg + "\n\nCurrent state:\n" + state.dump(2);
			return MakeOk(id, {{"content", {{{"type","text"},{"text", std::move(text)}}}}});
		};

		if (tool == "get_state")
		{
			std::lock_guard lock(s_snapshotMutex);
			json state = BuildStateJson(s_snapshot);
			return MakeOk(id, {{"content", {{{"type","text"},{"text", state.dump(2)}}}}});
		}

		if (tool == "execute_command")
		{
			std::string cmd = args.value("command", "");
			if (cmd.empty())
				return MakeError(id, -32602, "command is required");
			EnqueueCommand(cmd);
			return respondWithState("Queued: " + cmd);
		}

		if (tool == "navigate_to")
		{
			auto toFloat = [](const json& v, float def = 0.f) -> float {
				if (v.is_number()) return v.get<float>();
				if (v.is_string()) { try { return std::stof(v.get<std::string>()); } catch (...) {} }
				return def;
			};
			float y = args.contains("y") ? toFloat(args["y"]) : 0.f;
			float x = args.contains("x") ? toFloat(args["x"]) : 0.f;
			float z = args.contains("z") ? toFloat(args["z"]) : 0.f;
			std::string cmd = fmt::format("/nav loc {:.2f} {:.2f} {:.2f}", y, x, z);
			EnqueueCommand(cmd);
			return respondWithState("Navigating: " + cmd);
		}

		if (tool == "say")
		{
			std::string text = args.value("text", "");
			if (text.empty())
				return MakeError(id, -32602, "text is required");
			EnqueueCommand("/say " + text);
			return respondWithState("Said: " + text);
		}

		return MakeError(id, -32601, "Unknown tool: " + tool);
	}

	return MakeError(id, -32601, "Method not found: " + method);
}

//============================================================================
// HTTP server
//============================================================================

static std::unique_ptr<httplib::Server> s_server;
static std::thread                       s_serverThread;

static void AddCors(httplib::Response& res)
{
	res.set_header("Access-Control-Allow-Origin",  "*");
	res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept");
}

static void SetupRoutes()
{
	// CORS preflight
	s_server->Options(".*", [](const httplib::Request&, httplib::Response& res) {
		AddCors(res);
		res.status = 204;
	});

	// -----------------------------------------------------------------------
	// MCP endpoint — POST for JSON-RPC requests
	// -----------------------------------------------------------------------
	s_server->Post("/mcp", [](const httplib::Request& req, httplib::Response& res) {
		AddCors(res);
		try
		{
			json body     = json::parse(req.body);
			json response = HandleMCP(body);

			if (response.is_null() || response.empty())
			{
				res.status = 204;
				return;
			}
			res.set_content(response.dump(), "application/json");
		}
		catch (const std::exception& e)
		{
			res.status = 400;
			res.set_content(
				json{{"jsonrpc","2.0"},{"id",nullptr},{"error",{{"code",-32700},{"message",std::string("Parse error: ") + e.what()}}}}.dump(),
				"application/json");
		}
	});

	// -----------------------------------------------------------------------
	// MCP endpoint — GET for SSE (server-sent events / streaming transport)
	// We don't push server-initiated messages, so we open the stream and keep
	// it alive with periodic comments. Claude Code requires this to exist.
	// -----------------------------------------------------------------------
	s_server->Get("/mcp", [](const httplib::Request&, httplib::Response& res) {
		AddCors(res);
		res.set_header("Cache-Control",  "no-cache");
		res.set_header("X-Accel-Buffering", "no");

		res.set_chunked_content_provider(
			"text/event-stream",
			[](size_t /*offset*/, httplib::DataSink& sink) {
				// Send a keepalive comment every 15 seconds.
				// The loop exits when the client disconnects (sink.is_writable() == false).
				const std::string keepalive = ": keepalive\n\n";
				for (;;)
				{
					if (!sink.is_writable())
						return false;
					sink.write(keepalive.data(), keepalive.size());
					std::this_thread::sleep_for(std::chrono::seconds(15));
				}
			}
		);
	});

	// -----------------------------------------------------------------------
	// OAuth discovery — return empty metadata so Claude Code skips OAuth
	// -----------------------------------------------------------------------
	s_server->Get("/.well-known/oauth-protected-resource", [](const httplib::Request&, httplib::Response& res) {
		AddCors(res);
		// No auth required — return minimal valid metadata
		res.set_content(json{{"resource", fmt::format("http://127.0.0.1:{}", s_port)}}.dump(), "application/json");
	});

	s_server->Get("/.well-known/oauth-authorization-server", [](const httplib::Request&, httplib::Response& res) {
		AddCors(res);
		res.status = 404;
		res.set_content(R"({"error":"not_found"})", "application/json");
	});

	// -----------------------------------------------------------------------
	// Convenience read-only REST endpoints (useful for debugging)
	// -----------------------------------------------------------------------
	auto gameGet = [](httplib::Response& res, auto buildFn) {
		AddCors(res);
		std::lock_guard lock(s_snapshotMutex);
		if (!s_snapshot.inGame)
		{
			res.status = 503;
			res.set_content(R"({"error":"not in game"})", "application/json");
			return;
		}
		res.set_content(buildFn(s_snapshot).dump(2), "application/json");
	};

	s_server->Get("/me",     [gameGet](const httplib::Request&, httplib::Response& res) { gameGet(res, BuildPlayerJson); });
	s_server->Get("/zone",   [gameGet](const httplib::Request&, httplib::Response& res) { gameGet(res, BuildZoneJson);   });
	s_server->Get("/target", [gameGet](const httplib::Request&, httplib::Response& res) { gameGet(res, BuildTargetJson); });
	s_server->Get("/spawns", [gameGet](const httplib::Request&, httplib::Response& res) { gameGet(res, BuildSpawnsJson); });
}

//============================================================================
// /mcp command — display server status
//============================================================================

static void Cmd_MCPStatus(PlayerClient* /*pChar*/, const char* /*szLine*/)
{
	// Uptime
	auto elapsed = std::chrono::steady_clock::now() - s_startTime;
	int totalSec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
	int hours    = totalSec / 3600;
	int minutes  = (totalSec % 3600) / 60;
	int secs     = totalSec % 60;

	// Queue depth
	int queueDepth = 0;
	{
		std::lock_guard lock(s_queueMutex);
		queueDepth = static_cast<int>(s_commandQueue.size());
	}

	// Chat buffer depth
	int chatDepth = 0;
	{
		std::lock_guard lock(s_chatMutex);
		chatDepth = static_cast<int>(s_chatLines.size());
	}

	// Game state summary
	bool inGame = false;
	std::string charInfo;
	{
		std::lock_guard lock(s_snapshotMutex);
		inGame = s_snapshot.inGame;
		if (inGame)
		{
			int hpPct = s_snapshot.hpMax > 0
				? static_cast<int>(s_snapshot.hp * 100 / s_snapshot.hpMax) : 0;
			int mnPct = s_snapshot.manaMax > 0
				? s_snapshot.mana * 100 / s_snapshot.manaMax : 0;
			charInfo = fmt::format("{} ({}), zone: {}, HP: {}%, mana: {}%",
				s_snapshot.playerName, s_snapshot.playerLevel,
				s_snapshot.zoneShortName, hpPct, mnPct);
		}
	}

	WriteChatf("\ag[MQMCPServer]\ax ─────────────────────────────────");
	WriteChatf("\ag[MQMCPServer]\ax  URL    : \ayhttp://127.0.0.1:%d/mcp\ax", s_port);
	WriteChatf("\ag[MQMCPServer]\ax  Uptime : \aw%02d:%02d:%02d\ax", hours, minutes, secs);
	WriteChatf("\ag[MQMCPServer]\ax  Game   : %s", inGame ? fmt::format("\aw{}\ax", charInfo).c_str() : "\arNot in game\ax");
	WriteChatf("\ag[MQMCPServer]\ax  Queue  : \aw%d\ax pending command(s)", queueDepth);
	WriteChatf("\ag[MQMCPServer]\ax  Chat   : \aw%d\ax / %d lines buffered", chatDepth, (int)k_chatBufferSize);
	WriteChatf("\ag[MQMCPServer]\ax ─────────────────────────────────");
}

//============================================================================
// Plugin callbacks
//============================================================================

// Captures MQ window output (plugin messages, /echo, etc.)
PLUGIN_API DWORD OnWriteChatColor(const char* Line, DWORD Color, DWORD Filter)
{
	if (Line && *Line)
		PushChatLine(Line);
	return 0;
}

// Captures all incoming EQ chat: combat messages, spells, skills, say, tells, etc.
PLUGIN_API BOOL OnIncomingChat(const char* Line, DWORD Color)
{
	if (Line && *Line)
		PushChatLine(Line);
	return FALSE;  // don't suppress — let EQ display it normally
}

PLUGIN_API void InitializePlugin()
{
	s_startTime = std::chrono::steady_clock::now();

	s_server = std::make_unique<httplib::Server>();
	SetupRoutes();

	s_serverThread = std::thread([] {
		s_server->listen("0.0.0.0", s_port);
	});

	AddCommand("/mcp", Cmd_MCPStatus);

	WriteChatf("\ag[MQMCPServer]\ax MCP server listening on \ayhttp://127.0.0.1:%d/mcp\ax — type \aw/mcp\ax for status", s_port);
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/mcp");

	if (s_server)
		s_server->stop();
	if (s_serverThread.joinable())
		s_serverThread.join();
	s_server.reset();

	WriteChatf("\ag[MQMCPServer]\ax Stopped.");
}

PLUGIN_API void OnPulse()
{
	// Refresh game state snapshot
	UpdateSnapshot();

	// Drain and execute queued commands on the main thread
	std::queue<std::string> toRun;
	{
		std::lock_guard lock(s_queueMutex);
		std::swap(toRun, s_commandQueue);
	}

	while (!toRun.empty())
	{
		DoCommand(toRun.front().c_str(), false);
		toRun.pop();
	}
}
