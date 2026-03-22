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
)

source_groups("Header Files" ${MQ2CF_HEADERS})

# ---------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------
set(MQ2CF_SOURCES
    "MQ2CF.cpp"
    "core/LuaBindings.cpp"
    "core/SpawnPrimitives.cpp"
    # Core subsystems (Phase 2+)
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
