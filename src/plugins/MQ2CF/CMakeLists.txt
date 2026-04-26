# MQ2CF: MacroQuest Class Framework
# Open-source CWTN replacement with C++/Lua/SQLite hybrid architecture

include("../../Plugin.cmake")

# ---------------------------------------------------------------------
# vcpkg dependencies
# ---------------------------------------------------------------------
find_package(luajit REQUIRED)
find_package(sol2 REQUIRED)
find_package(unofficial-sqlite3 REQUIRED)

# ---------------------------------------------------------------------
# Header files
# ---------------------------------------------------------------------
set(MQ2CF_HEADERS
    "core/LuaBindings.h"
    "core/SpawnPrimitives.h"
    "core/SpellData.h"
    "core/GroupEngine.h"
    "core/LocalPlayer.h"
    "core/TargetValidation.h"
    "core/CastEngine.h"
    "core/Lifecycle.h"
    "core/Humanize.h"
    "core/Settings.h"
    "core/SpellScan.h"
    "core/SchemaInit.h"
    "core/Camp.h"
    "core/ModeSystem.h"
    "core/BuffEngine.h"
    "core/AAEngine.h"
    "core/GemManager.h"
    "core/EventBus.h"
    "core/ImGuiWindow.h"
    "core/BuffTracker.h"
)

source_groups("Header Files" ${MQ2CF_HEADERS})

# ---------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------
set(MQ2CF_SOURCES
    "MQ2CF.cpp"
    "core/LuaBindings.cpp"
    "core/SpawnPrimitives.cpp"
    "core/SpellData.cpp"
    "core/GroupEngine.cpp"
    "core/LocalPlayer.cpp"
    "core/TargetValidation.cpp"
    "core/CastEngine.cpp"
    "core/Lifecycle.cpp"
    "core/Humanize.cpp"
    # Core subsystems (Phase 2+)
    "core/BuffEngine.cpp"
    # "core/CCEngine.cpp"
    # "core/SongEngine.cpp"
    # "core/AuraManager.cpp"
    "core/Settings.cpp"
    "core/SpellScan.cpp"
    "core/SchemaInit.cpp"
    "core/Camp.cpp"
    "core/ModeSystem.cpp"
    "core/AAEngine.cpp"
    "core/GemManager.cpp"
    "core/EventBus.cpp"
    "core/ImGuiWindow.cpp"
    "core/BuffTracker.cpp"
    # "core/NavBridge.cpp"
    # "core/Utility.cpp"
    # "core/ImGuiScaffold.cpp"  # replaced by ImGuiWindow.cpp
    # "core/Database.cpp"
)

source_groups("Source Files" ${MQ2CF_SOURCES})

# ---------------------------------------------------------------------
# Target definition
# ---------------------------------------------------------------------
add_library(MQ2CF SHARED
    ${MQ2CF_HEADERS}
    ${MQ2CF_SOURCES}
)

set_target_properties(MQ2CF PROPERTIES FOLDER "plugins")

# ---------------------------------------------------------------------
# Apply props file configurations
# ---------------------------------------------------------------------
target_Plugin_props(MQ2CF)

# ---------------------------------------------------------------------
# Link vcpkg dependencies
# ---------------------------------------------------------------------
target_link_libraries(MQ2CF PRIVATE
    luajit::luajit
    sol2
    unofficial::sqlite3::sqlite3
)

# ---------------------------------------------------------------------
# Project dependencies
# ---------------------------------------------------------------------
target_link_libraries(MQ2CF PRIVATE MQ2Main)

# ---------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------
target_compile_definitions(MQ2CF PRIVATE
    "SOL_LUAJIT=1"
)

# ---------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------
target_include_directories(MQ2CF PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/"
)

# ---------------------------------------------------------------------
# Compiler options
# ---------------------------------------------------------------------
target_compile_options(MQ2CF PRIVATE
    "/bigobj"
)
