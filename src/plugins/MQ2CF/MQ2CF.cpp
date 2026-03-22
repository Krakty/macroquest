/*
 * MQ2CF: MacroQuest Class Framework
 * Open-source replacement for CWTN class plugins
 *
 * Architecture: C++ core with Lua class modules and SQLite data
 * The C++ core exposes eqlib primitives to Lua. All decision logic
 * (heal triage, spell selection, CC priority) lives in Lua scripts.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>

PreSetup("MQ2CF");
PLUGIN_VERSION(0.1);

// Forward declarations for core subsystems
namespace CF {
    void InitializeLuaBindings();
    void ShutdownLua();
    void PulseLua();
}

PLUGIN_API void InitializePlugin()
{
    WriteChatf("\ag[MQ2CF]\ax Class Framework loading...");

    // TODO Phase 1: Initialize sol2 Lua state
    // TODO Phase 1: Register Core.* API bindings
    // TODO Phase 1: Open SQLite database
    // TODO Phase 1: Detect player class and load appropriate Lua module

    WriteChatf("\ag[MQ2CF]\ax Plugin initialized (v%.1f)", MQ2Version);
}

PLUGIN_API void ShutdownPlugin()
{
    WriteChatf("\ag[MQ2CF]\ax Shutting down...");

    // TODO: Cleanup Lua state
    // TODO: Close SQLite database
}

PLUGIN_API void OnPulse()
{
    if (GetGameState() != GAMESTATE_INGAME)
        return;

    if (!pLocalPlayer || !pLocalPC)
        return;

    // TODO Phase 1: Dispatch to Lua OnPulse callbacks
}

PLUGIN_API void OnZoned()
{
    // TODO: Notify Lua of zone change
}

PLUGIN_API void SetGameState(int GameState)
{
    if (GameState == GAMESTATE_INGAME)
    {
        // TODO: Load/reload character-specific settings
        // TODO: Load class-appropriate Lua module
    }
}

PLUGIN_API void OnUpdateImGui()
{
    // TODO Phase 6: Dispatch to Lua ImGui render callbacks
}
