# MQ2CF: MacroQuest Class Framework
# Open-source CWTN replacement with C++/Lua/SQLite hybrid architecture

include("../../Plugin.cmake")

# ---------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------
set(MQ2CF_SOURCES
    "MQ2CF.cpp"
    # Core subsystems (Phase 1+)
    # "core/SpawnPrimitives.cpp"
    # "core/LocalPlayer.cpp"
    # "core/SpellData.cpp"
    # "core/CastEngine.cpp"
    # "core/TargetValidation.cpp"
    # "core/GroupEngine.cpp"
    # "core/BuffEngine.cpp"
    # "core/CCEngine.cpp"
    # "core/SongEngine.cpp"
    # "core/AuraManager.cpp"
    # "core/Settings.cpp"
    # "core/NavBridge.cpp"
    # "core/Utility.cpp"
    # "core/ImGuiScaffold.cpp"
    # "core/Database.cpp"
)

source_groups("Source Files" ${MQ2CF_SOURCES})

# ---------------------------------------------------------------------
# Target definition
# ---------------------------------------------------------------------
add_library(MQ2CF SHARED ${MQ2CF_SOURCES})

set_target_properties(MQ2CF PROPERTIES FOLDER "plugins")

# ---------------------------------------------------------------------
# Apply props file configurations
# ---------------------------------------------------------------------
target_Plugin_props(MQ2CF)

# ---------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------
target_link_libraries(MQ2CF PRIVATE MQ2Main)

# TODO Phase 1: Add sol2 and SQLite3 dependencies
# find_package(Lua REQUIRED)
# target_link_libraries(MQ2CF PRIVATE ${LUA_LIBRARIES} sqlite3)
# target_include_directories(MQ2CF PRIVATE ${LUA_INCLUDE_DIR})
