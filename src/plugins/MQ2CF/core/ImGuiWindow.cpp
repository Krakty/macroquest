/*
 * MQ2CF: MacroQuest Class Framework
 * ImGui settings window -- in-game UI for configuration
 *
 * Stack-based drill-down navigation instead of flat collapsible layout.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "ImGuiWindow.h"
#include "ModeSystem.h"
#include "Lifecycle.h"

#include <vector>

namespace CF {

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------
enum class NavPanel : int {
	Root = 0,
	Mode,
	Healing,
	HealingMA,
	HealingNonMA,
	HealingToggles,
	RezCures,
	BuffsDowntime,
	SpellGems,
	CampEvents,
	Status,
};

static std::vector<NavPanel> s_navStack = { NavPanel::Root };

static void PushPanel(NavPanel panel) { s_navStack.push_back(panel); }
static void PopPanel() { if (s_navStack.size() > 1) s_navStack.pop_back(); }
static NavPanel CurrentPanel() { return s_navStack.back(); }

// ---------------------------------------------------------------------------
// Window state
// ---------------------------------------------------------------------------
static bool s_showWindow = false;

// ---------------------------------------------------------------------------
// Cached settings (mirrored from Lua Cleric.Settings)
// ---------------------------------------------------------------------------

// Heal thresholds -- MA
static int s_maQuickHeal = 35;
static int s_maIntervention = 50;
static int s_maHealsHeals = 70;
static int s_maPromise = 80;
static int s_maDurationHeal = 90;

// Heal thresholds -- Non-MA
static int s_nonMAQuickHeal = 25;
static int s_nonMAIntervention = 40;
static int s_nonMAHealsHeals = 60;
static int s_nonMAPromise = 70;
static int s_nonMADurationHeal = 80;

// Rez & Cures toggles
static bool s_useRez = true;
static bool s_useDivineRez = false;
static bool s_useEpicRez = false;
static bool s_requestEvacOnTankDeath = false;
static bool s_useCures = true;

// Buffs & Downtime toggles
static bool s_useYaulp = true;
static bool s_useWardAA = true;
static bool s_keepHoTUp = false;

// Healing toggles
static bool s_useSplash = true;
static bool s_useXTargetHealing = true;
static bool s_battleMode = false;

// Status info (read-only, refreshed each frame)
static int s_scannedSpellCount = 0;

// Frame throttle for Lua reads (don't read every frame)
static int s_refreshCounter = 0;
static constexpr int kRefreshInterval = 30; // read from Lua every N frames

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void SyncSettingsFromLua(sol::state* lua)
{
	if (!lua)
		return;

	sol::object clericObj = (*lua)["Cleric"];
	if (!clericObj.valid() || clericObj.get_type() != sol::type::table)
		return;

	sol::table cleric = clericObj.as<sol::table>();
	sol::object settingsObj = cleric["Settings"];
	if (!settingsObj.valid() || settingsObj.get_type() != sol::type::table)
		return;

	sol::table s = settingsObj.as<sol::table>();

	// Helper to read an int safely
	auto readInt = [&](const char* key, int& dst) {
		sol::object val = s[key];
		if (val.valid() && val.is<int>())
			dst = val.as<int>();
	};

	// Helper to read a bool safely
	auto readBool = [&](const char* key, bool& dst) {
		sol::object val = s[key];
		if (val.valid() && val.is<bool>())
			dst = val.as<bool>();
	};

	// MA thresholds
	readInt("MAQuickHeal", s_maQuickHeal);
	readInt("MAIntervention", s_maIntervention);
	readInt("MAHealsHeals", s_maHealsHeals);
	readInt("MAPromise", s_maPromise);
	readInt("MADurationHeal", s_maDurationHeal);

	// Non-MA thresholds
	readInt("NonMAQuickHeal", s_nonMAQuickHeal);
	readInt("NonMAIntervention", s_nonMAIntervention);
	readInt("NonMAHealsHeals", s_nonMAHealsHeals);
	readInt("NonMAPromise", s_nonMAPromise);
	readInt("NonMADurationHeal", s_nonMADurationHeal);

	// Rez & Cures
	readBool("UseRez", s_useRez);
	readBool("UseDivineRez", s_useDivineRez);
	readBool("UseEpicRez", s_useEpicRez);
	readBool("RequestEvacOnTankDeath", s_requestEvacOnTankDeath);
	readBool("UseCures", s_useCures);

	// Buffs & Downtime
	readBool("UseYaulp", s_useYaulp);
	readBool("UseWardAA", s_useWardAA);
	readBool("KeepHoTUp", s_keepHoTUp);

	// Healing toggles
	readBool("UseSplash", s_useSplash);
	readBool("UseXTargetHealing", s_useXTargetHealing);
	readBool("BattleMode", s_battleMode);

	// Spell count
	sol::object countObj = cleric["scannedSpellCount"];
	if (countObj.valid() && countObj.is<int>())
		s_scannedSpellCount = countObj.as<int>();
}

static void WriteIntToLua(sol::state* lua, const char* key, int value)
{
	if (!lua)
		return;
	std::string script = std::string("if Cleric and Cleric.Settings then Cleric.Settings.")
		+ key + " = " + std::to_string(value) + " end";
	lua->safe_script(script, sol::script_pass_on_error);
}

static void WriteBoolToLua(sol::state* lua, const char* key, bool value)
{
	if (!lua)
		return;
	std::string script = std::string("if Cleric and Cleric.Settings then Cleric.Settings.")
		+ key + " = " + (value ? "true" : "false") + " end";
	lua->safe_script(script, sol::script_pass_on_error);
}

// Helper: draw back button + separator. Returns true so callers can chain.
static bool DrawBackButton()
{
	if (ImGui::Button("<- Back"))
		PopPanel();
	ImGui::Separator();
	return true;
}

// Helper: checkbox that writes to Lua on change
static void ToggleCheckbox(const char* label, bool& val, const char* luaKey,
	sol::state* lua, bool luaReady)
{
	if (ImGui::Checkbox(label, &val))
	{
		if (luaReady && lua)
			WriteBoolToLua(lua, luaKey, val);
	}
}

// ---------------------------------------------------------------------------
// Panel draw functions
// ---------------------------------------------------------------------------
static void DrawRootPanel()
{
	if (ImGui::Selectable("Mode"))              PushPanel(NavPanel::Mode);
	if (ImGui::Selectable("Healing"))           PushPanel(NavPanel::Healing);
	if (ImGui::Selectable("Rez & Cures"))       PushPanel(NavPanel::RezCures);
	if (ImGui::Selectable("Buffs & Downtime"))  PushPanel(NavPanel::BuffsDowntime);
	if (ImGui::Selectable("Spell Gems"))        PushPanel(NavPanel::SpellGems);
	if (ImGui::Selectable("Camp & Events"))     PushPanel(NavPanel::CampEvents);
	if (ImGui::Selectable("Status"))            PushPanel(NavPanel::Status);
}

static void DrawModePanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Operating Mode");
	ImGui::Spacing();

	int currentMode = GetCurrentMode();

	// Get player class for availability check
	int playerClass = 0;
	if (pLocalPlayer)
		playerClass = pLocalPlayer->GetClass();

	for (int i = 0; i <= 8; i++)
	{
		bool available = IsModeAvailableForClass(i, playerClass);
		if (!available)
			ImGui::BeginDisabled();

		if (ImGui::RadioButton(GetModeName(i), currentMode == i))
		{
			SetCurrentMode(i);
			if (luaReady && lua)
			{
				std::string script = "if Cleric then Cleric.mode = "
					+ std::to_string(i) + " end";
				lua->safe_script(script, sol::script_pass_on_error);
			}
		}

		if (!available)
			ImGui::EndDisabled();

		if (i == 2 || i == 4 || i == 6)
			ImGui::Separator();
	}
}

static void DrawHealingPanel()
{
	DrawBackButton();

	ImGui::Text("Healing Configuration");
	ImGui::Spacing();

	if (ImGui::Selectable("MA Thresholds"))      PushPanel(NavPanel::HealingMA);
	if (ImGui::Selectable("Non-MA Thresholds"))  PushPanel(NavPanel::HealingNonMA);
	if (ImGui::Selectable("Heal Toggles"))       PushPanel(NavPanel::HealingToggles);
}

static void DrawHealingMAPanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("MA Heal Thresholds");
	ImGui::Spacing();

	bool changed = false;
	changed |= ImGui::SliderInt("Quick Heal##ma", &s_maQuickHeal, 0, 100);
	changed |= ImGui::SliderInt("Intervention##ma", &s_maIntervention, 0, 100);
	changed |= ImGui::SliderInt("Heals##ma", &s_maHealsHeals, 0, 100);
	changed |= ImGui::SliderInt("Promise##ma", &s_maPromise, 0, 100);
	changed |= ImGui::SliderInt("Duration Heal##ma", &s_maDurationHeal, 0, 100);

	if (changed && luaReady && lua)
	{
		WriteIntToLua(lua, "MAQuickHeal", s_maQuickHeal);
		WriteIntToLua(lua, "MAIntervention", s_maIntervention);
		WriteIntToLua(lua, "MAHealsHeals", s_maHealsHeals);
		WriteIntToLua(lua, "MAPromise", s_maPromise);
		WriteIntToLua(lua, "MADurationHeal", s_maDurationHeal);
	}
}

static void DrawHealingNonMAPanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Non-MA Heal Thresholds");
	ImGui::Spacing();

	bool changed = false;
	changed |= ImGui::SliderInt("Quick Heal##nonma", &s_nonMAQuickHeal, 0, 100);
	changed |= ImGui::SliderInt("Intervention##nonma", &s_nonMAIntervention, 0, 100);
	changed |= ImGui::SliderInt("Heals##nonma", &s_nonMAHealsHeals, 0, 100);
	changed |= ImGui::SliderInt("Promise##nonma", &s_nonMAPromise, 0, 100);
	changed |= ImGui::SliderInt("Duration Heal##nonma", &s_nonMADurationHeal, 0, 100);

	if (changed && luaReady && lua)
	{
		WriteIntToLua(lua, "NonMAQuickHeal", s_nonMAQuickHeal);
		WriteIntToLua(lua, "NonMAIntervention", s_nonMAIntervention);
		WriteIntToLua(lua, "NonMAHealsHeals", s_nonMAHealsHeals);
		WriteIntToLua(lua, "NonMAPromise", s_nonMAPromise);
		WriteIntToLua(lua, "NonMADurationHeal", s_nonMADurationHeal);
	}
}

static void DrawHealingTogglesPanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Heal Toggles");
	ImGui::Spacing();

	ToggleCheckbox("Use Splash", s_useSplash, "UseSplash", lua, luaReady);
	ToggleCheckbox("Use XTarget Healing", s_useXTargetHealing, "UseXTargetHealing", lua, luaReady);
	ToggleCheckbox("Battle Mode", s_battleMode, "BattleMode", lua, luaReady);
}

static void DrawRezCuresPanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Rez & Cures");
	ImGui::Spacing();

	ToggleCheckbox("Use Rez", s_useRez, "UseRez", lua, luaReady);
	ToggleCheckbox("Use Divine Rez", s_useDivineRez, "UseDivineRez", lua, luaReady);
	ToggleCheckbox("Use Epic Rez", s_useEpicRez, "UseEpicRez", lua, luaReady);
	ToggleCheckbox("Request Evac on Tank Death", s_requestEvacOnTankDeath, "RequestEvacOnTankDeath", lua, luaReady);
	ImGui::Separator();
	ToggleCheckbox("Use Cures", s_useCures, "UseCures", lua, luaReady);
}

static void DrawBuffsDowntimePanel(sol::state* lua, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Buffs & Downtime");
	ImGui::Spacing();

	ToggleCheckbox("Use Yaulp", s_useYaulp, "UseYaulp", lua, luaReady);
	ToggleCheckbox("Use Ward AA", s_useWardAA, "UseWardAA", lua, luaReady);
	ToggleCheckbox("Keep HoT Up", s_keepHoTUp, "KeepHoTUp", lua, luaReady);
}

static void DrawSpellGemsPanel()
{
	DrawBackButton();

	ImGui::Text("Spell Gems");
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Read-only -- loadout display)");
	ImGui::Spacing();

	// Show current spell loadout from spell gems
	for (int gem = 0; gem < NUM_SPELL_GEMS; gem++)
	{
		if (pLocalPC && pLocalPC->GetMemorizedSpell(gem) != -1)
		{
			EQ_Spell* spell = GetSpellByID(pLocalPC->GetMemorizedSpell(gem));
			if (spell)
				ImGui::Text("Gem %2d: %s", gem + 1, spell->Name);
			else
				ImGui::Text("Gem %2d: (unknown spell)", gem + 1);
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Gem %2d: (empty)", gem + 1);
		}
	}
}

static void DrawCampEventsPanel()
{
	DrawBackButton();

	ImGui::Text("Camp & Events");
	ImGui::Spacing();

	// Camp position display
	if (pLocalPlayer)
	{
		ImGui::Text("Current Pos: %.1f, %.1f, %.1f",
			pLocalPlayer->X, pLocalPlayer->Y, pLocalPlayer->Z);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No character loaded");
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Event toggles coming soon)");
}

static void DrawStatusPanel(const char* dbPath, const char* charDbPath, bool luaReady)
{
	DrawBackButton();

	ImGui::Text("Status");
	ImGui::Spacing();

	ImGui::Text("Global DB: %s", dbPath ? dbPath : "not opened");
	ImGui::Text("Char DB:   %s", charDbPath ? charDbPath : "not opened");
	ImGui::Separator();
	ImGui::Text("Scanned Spells: %d", s_scannedSpellCount);
	ImGui::Text("Mode: %s (%d)", GetModeName(GetCurrentMode()), GetCurrentMode());
	ImGui::Text("Lua State: %s", luaReady ? "active" : "inactive");
	ImGui::Text("Paused: %s", IsPaused() ? "yes" : "no");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
bool IsSettingsWindowOpen()
{
	return s_showWindow;
}

void ToggleSettingsWindow()
{
	s_showWindow = !s_showWindow;
	s_refreshCounter = 0; // force immediate refresh on open
	s_navStack = { NavPanel::Root }; // reset nav on toggle
}

void DrawSettingsWindow(sol::state* lua, bool luaReady,
	const char* dbPath, const char* charDbPath)
{
	if (!s_showWindow)
		return;

	// Periodic sync from Lua
	if (luaReady && lua)
	{
		if (s_refreshCounter <= 0)
		{
			SyncSettingsFromLua(lua);
			s_refreshCounter = kRefreshInterval;
		}
		else
		{
			s_refreshCounter--;
		}
	}

	ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("MQ2CF Settings", &s_showWindow))
	{
		ImGui::End();
		return;
	}

	// -----------------------------------------------------------------
	// Header bar: plugin name, current mode, pause button
	// -----------------------------------------------------------------
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "MQ2CF");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "[%s]",
		GetModeName(GetCurrentMode()));

	ImGui::SameLine(ImGui::GetWindowWidth() - 90);
	bool paused = IsPaused();
	if (ImGui::Button(paused ? "Resume" : "Pause", ImVec2(75, 0)))
	{
		SetPaused(!paused);
	}

	ImGui::Separator();

	// -----------------------------------------------------------------
	// Panel dispatch
	// -----------------------------------------------------------------
	switch (CurrentPanel())
	{
	case NavPanel::Root:            DrawRootPanel(); break;
	case NavPanel::Mode:            DrawModePanel(lua, luaReady); break;
	case NavPanel::Healing:         DrawHealingPanel(); break;
	case NavPanel::HealingMA:       DrawHealingMAPanel(lua, luaReady); break;
	case NavPanel::HealingNonMA:    DrawHealingNonMAPanel(lua, luaReady); break;
	case NavPanel::HealingToggles:  DrawHealingTogglesPanel(lua, luaReady); break;
	case NavPanel::RezCures:        DrawRezCuresPanel(lua, luaReady); break;
	case NavPanel::BuffsDowntime:   DrawBuffsDowntimePanel(lua, luaReady); break;
	case NavPanel::SpellGems:       DrawSpellGemsPanel(); break;
	case NavPanel::CampEvents:      DrawCampEventsPanel(); break;
	case NavPanel::Status:          DrawStatusPanel(dbPath, charDbPath, luaReady); break;
	default:
		s_navStack = { NavPanel::Root };
		break;
	}

	ImGui::End();
}

} // namespace CF
