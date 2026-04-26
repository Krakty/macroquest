# C++ Core Lua API Reference

## Overview
The C++ Core provides a comprehensive API for Lua class modules to interact with EverQuest game systems. This document details all exposed functions, their signatures, and the underlying decompiled functions they wrap.

## API Conventions

### Type Mappings
- **C++ to Lua types**:
  - `int` → `number` (integer)
  - `float` → `number` (floating point)
  - `bool` → `boolean`
  - `std::string` → `string`
  - `char*` → `string`
  - `Spawn*` → `userdata` (light wrapper)
  - `EQ_Spell*` → `userdata` (light wrapper)
  - `std::vector<T>` → `table` (array)
  - `std::unordered_map<K,V>` → `table` (key-value)

- **Lua to C++ types**:
  - `nil` → `std::nullopt` or default value
  - `function` → `sol::function` (callbacks)
  - `table` → `sol::table` or specific container

### Error Handling
- All functions return `(result, error_message)` tuples
- `nil` return indicates success with no value
- Non-nil error message indicates failure

## Core Subsystems

### 1. Spawn Primitives (`Core.Spawn`)
Access to PlayerClient and PlayerZoneClient fields.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetPosition` | `(spawn_id: number) → (x: number, y: number, z: number)` | Get spawn's 3D position | `FUN_1800aac20` |
| `GetVelocity` | `(spawn_id: number) → (vx: number, vy: number, vz: number)` | Get spawn's velocity vector | `FUN_18007b750` |
| `GetName` | `(spawn_id: number) → string` | Get spawn name | `FUN_180083e50` |
| `GetSpawnID` | `(spawn_ptr: userdata) → number` | Get spawn ID from pointer | `FUN_180083e50` |
| `GetType` | `(spawn_id: number) → number` | Get spawn type (0x02=dead/feigned) | `FUN_1800a64e0` |
| `GetRace` | `(spawn_id: number) → number` | Get race/body type | `FUN_1800aac20` |
| `GetLevel` | `(spawn_id: number) → number` | Get spawn level (byte) | `FUN_18007b750` |
| `GetClass` | `(spawn_id: number) → number` | Get class ID (EQ class enum) | `FUN_1800a5280` |
| `GetHP` | `(spawn_id: number) → (current: number, max: number)` | Get HP values (non-self) | `FUN_180083e50` |
| `GetHPPercent` | `(spawn_id: number) → number` | Calculate HP percentage | `FUN_180083e50` |
| `GetMana` | `(spawn_id: number) → (current: number, max: number)` | Get mana values | `FUN_18007b750` |
| `GetTarget` | `(spawn_id: number) → spawn_id: number` | Get target spawn ID | `FUN_18007b750` |
| `GetMasterID` | `(spawn_id: number) → number` | Get owner spawn ID (pets/mercs) | `FUN_1800aac20` |
| `GetPetID` | `(spawn_id: number) → number` | Get pet spawn ID | `FUN_1800a5570` |
| `IsCasting` | `(spawn_id: number) → boolean` | Check if spawn is casting | `FUN_180074cf0` |
| `GetCastingSpellID` | `(spawn_id: number) → number` | Get currently casting spell ID | `FUN_180074cf0` |
| `GetStandState` | `(spawn_id: number) → number` | Get animation/stand state | `FUN_1800a64e0` |
| `IsMoving` | `(spawn_id: number) → boolean` | Check if spawn is moving | `FUN_18007b750` |
| `GetDistance` | `(spawn_id1: number, spawn_id2: number) → number` | Calculate 3D distance between spawns | `FUN_1800aac20` |
| `GetSpawnByID` | `(spawn_id: number) → spawn_ptr: userdata` | Get spawn pointer by ID | `FUN_1800a5570` |
| `CanSee` | `(from_spawn_id: number, to_spawn_id: number) → boolean` | Line of sight check | `FUN_1800aac20` |

### 2. Local Player (`Core.LocalPlayer`)
Access to PcClient/CharacterZoneClient data.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetMana` | `() → (current: number, max: number)` | Get local player mana | `FUN_18007b750` |
| `GetSpellGem` | `(slot: number) → spell_id: number` | Get memorized spell ID in gem slot | `FUN_18007d580` |
| `GetSpellGemRecast` | `(slot: number) → timer_ms: number` | Get gem recast timer | `FUN_18007d580` |
| `GetGlobalRecovery` | `() → timer_ms: number` | Get global cast recovery timer | `FUN_18007d580` |
| `IsStunned` | `() → boolean` | Check if player is stunned | `FUN_18007b750` |
| `GetXTarget` | `(index: number) → spawn_id: number` | Get XTarget spawn ID | `FUN_1800a5570` |
| `GetXTargetCount` | `() → number` | Get number of XTargets | `FUN_1800a5570` |
| `GetGroupMember` | `(index: number) → spawn_id: number` | Get group member spawn ID | `FUN_1800a5570` |
| `GetGroupMemberCount` | `() → number` | Get group size | `FUN_1800a5570` |
| `GetRaidAssist` | `(index: number) → name: string` | Get raid assist name | `FUN_18008a0b0` |
| `GetBuffTimestamp` | `() → timestamp: number` | Get buff age reference timestamp | `FUN_1800fb730` |
| `IsGateFlagSet` | `() → boolean` | Check gate flag (skip pulse if nonzero) | OnPulse dispatch |
| `GetCharacterBase` | `() → ptr: userdata` | Get CharacterZoneClient base pointer | `FUN_180083e50` |

### 3. Spell Data (`Core.Spell`)
Access to EQ_Spell structure fields.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetRange` | `(spell_id: number) → range: number` | Get spell range (float) | `FUN_1800aac20` |
| `GetAERange` | `(spell_id: number) → ae_range: number` | Get AE/rain/cone range | `FUN_1800c2be0` |
| `GetManaCost` | `(spell_id: number) → mana: number` | Get mana cost | `FUN_18007b750` |
| `GetTargetType` | `(spell_id: number) → type: number` | Get target type enum | `FUN_1800aac20` |
| `GetSpellType` | `(spell_id: number) → type: number` | 0=beneficial,1=detrimental,2=always | `FUN_1800aac20` |
| `GetName` | `(spell_id: number) → name: string` | Get spell name | `FUN_18007b750` |
| `GetCategory` | `(spell_id: number) → category: number` | Get spell category | `FUN_1800aac20` |
| `GetSubcategory` | `(spell_id: number) → subcategory: number` | Get spell subcategory | `FUN_1800aac20` |
| `GetSpellGroup` | `(spell_id: number) → group_id: number` | Get buff stacking group ID | `FUN_1800fb730` |
| `GetLevelRequired` | `(spell_id: number, class_id: number) → level: number` | Get required level for class | `FUN_18007b750` |
| `GetNumEffects` | `(spell_id: number) → count: number` | Get number of spell effects | `FUN_1800a64e0` |
| `GetReagents` | `(spell_id: number) → table` | Get reagent IDs and counts | `FUN_1800a94b0` |
| `GetEnvironmentFlag` | `(spell_id: number) → flag: number` | Get indoor/outdoor flag | `FUN_18007b750` |
| `GetStackingFlag` | `(spell_id: number) → flag: number` | Get buff stacking flag | `FUN_18007b750` |
| `GetHighestLearnedByGroup` | `(group_id: number) → spell_id: number` | Resolve highest learned rank | SQLite query |

### 4. Cast Engine (`Core.Cast`)
Full validation chain and cast dispatch.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `CanCast` | `(spell_id: number, target_id: number) → (can_cast: boolean, reason: string)` | Full validation chain | `FUN_18007b750` |
| `Cast` | `(spell_id: number, target_id: number) → (success: boolean, error: string)` | Dispatch cast with validation | `FUN_18007bd00` |
| `CastBurstOfLife` | `(target_id: number) → (success: boolean, error: string)` | Special Burst of Life path | `FUN_18007b030` |
| `IsGemReady` | `(slot: number) → boolean` | Check gem recast and global recovery | `FUN_18007d580` |
| `FindGemForSpell` | `(spell_id: number) → slot: number` | Find gem containing spell | `FUN_18007d580` |
| `ValidateLevel` | `(spell_id: number) → boolean` | Check player level requirement | `FUN_18007b750` |
| `ValidateMana` | `(spell_id: number) → boolean` | Check mana cost | `FUN_18007b750` |
| `ValidateRange` | `(spell_id: number, target_id: number) → boolean` | Check range to target | `FUN_1800aac20` |
| `ValidateMotion` | `(target_id: number) → boolean` | Check target motion for healing | `FUN_18007b750` |
| `ValidateTargetType` | `(spell_id: number, target_id: number) → boolean` | Check target type compatibility | `FUN_1800aac20` |
| `ValidateEnvironment` | `(spell_id: number) → boolean` | Check indoor/outdoor flag | `FUN_18007b750` |
| `StopCasting` | `() → success: boolean` | Stop current cast | `FUN_180074cf0` |

### 5. Target Validation (`Core.Target`)
Target-specific checks and utilities.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `ValidateLoS` | `(caster_id: number, target_id: number) → boolean` | Line of sight check | `FUN_1800aac20` |
| `ValidatePetOwner` | `(pet_id: number, owner_id: number) → boolean` | Check pet ownership | `FUN_1800aac20` |
| `ValidateMercOwner` | `(merc_id: number, owner_id: number) → boolean` | Check merc ownership | `FUN_1800aac20` |
| `GetValidTargetsForSpell` | `(spell_id: number) → table<spawn_id>` | Get valid targets for spell type | `FUN_1800aac20` |
| `IsValidHealTarget` | `(target_id: number) → boolean` | Check if target can be healed | `FUN_1800a64e0` |
| `IsValidMezTarget` | `(target_id: number) → boolean` | Check if target can be mezzed | `FUN_180147c80` |
| `IsValidCharmTarget` | `(target_id: number) → boolean` | Check if target can be charmed | `FUN_1801473e0` |
| `ApplyFocusRangeMod` | `(base_range: number) → modified_range: number` | Apply focus range modifier | `FUN_1800c2be0` |

### 6. Group Engine (`Core.Group`)
Group and XTarget iteration and triage.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetHealTarget` | `(threshold_pct: number, include_xtarget: boolean) → spawn_id: number` | Find lowest HP valid target | `FUN_1800a5280` |
| `GetMainAssist` | `() → spawn_id: number` | Get group main assist spawn ID | `FUN_180082410` |
| `GetGroupMembers` | `() → table<spawn_id>` | Get all group member spawn IDs | `FUN_1800a5570` |
| `GetXTargets` | `() → table<spawn_id>` | Get all XTarget spawn IDs | `FUN_1800a5570` |
| `GetRaidAssists` | `() → table<name>` | Get all raid assist names | `FUN_18008a0b0` |
| `IsMainAssist` | `(spawn_id: number) → boolean` | Check if spawn is MA | `FUN_180082410` |
| `GetGroupMemberBySpawnID` | `(spawn_id: number) → member_data: table` | Get CGroupMember data | `FUN_1800a5570` |
| `GetLowestHPPct` | `(include_self: boolean) → (spawn_id: number, pct: number)` | Find lowest HP% in group | `FUN_1800a5280` |
| `GetTriagePriority` | `(spawn_id: number) → priority: number` | Calculate heal priority (MA, merc cap, HP%) | `FUN_1800a5280` |

### 7. Buff Engine (`Core.Buff`)
Buff stacking and management.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `IsBuffOnTarget` | `(target_id: number, spell_id: number) → boolean` | Check if buff is active | `FUN_18007b750` |
| `IsBuffOnTargetByGroup` | `(target_id: number, group_id: number) → boolean` | Check if buff from group is active | `FUN_1800fb730` |
| `GetTargetBuffCount` | `(target_id: number) → count: number` | Get number of buffs on target | `FUN_18007b750` |
| `GetTargetBuffArray` | `(target_id: number) → table<buff_data>` | Get all buffs on target | `FUN_18007b750` |
| `CheckStacking` | `(new_spell_id: number, target_id: number) → (can_stack: boolean, conflict_id: number)` | Check buff stacking rules | `FUN_1800fb730` |
| `LoadCWTNBuffsINI` | `() → success: boolean` | Load CWTNBuffs.ini settings | `FUN_1800fb730` |
| `GetBuffAge` | `(target_id: number, spell_id: number) → age_ms: number` | Get buff age using timestamp | `FUN_1800fb730` |
| `ShouldRefreshBuff` | `(target_id: number, spell_id: number, refresh_pct: number) → boolean` | Check if buff needs refresh | `FUN_1800fb730` |

### 8. Reagent Engine (`Core.Reagent`)
Reagent inventory validation.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `HasReagents` | `(spell_id: number) → boolean` | Check if player has required reagents | `FUN_1800a94b0` |
| `GetMissingReagents` | `(spell_id: number) → table<item_id, count_needed>` | List missing reagents | `FUN_1800a94b0` |
| `ConsumeReagents` | `(spell_id: number) → success: boolean` | Consume reagents for spell | `FUN_1800a94b0` |

### 9. CC Engine (`Core.CC`)
Mez, charm, and crowd control.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `FindMezTarget` | `() → spawn_id: number` | Find next mob to mez | `FUN_180147c80` |
| `FindCharmTarget` | `(charm_type: number) → spawn_id: number` | Find best charm target | `FUN_1801473e0` |
| `IsMezzed` | `(spawn_id: number) → boolean` | Check if mob is mezzed (animation) | `FUN_180147c80` |
| `IsCharmed` | `(spawn_id: number) → boolean` | Check if mob is charmed | `FUN_1801473e0` |
| `GetLastCharmID` | `() → spawn_id: number` | Get last charmed mob ID | `FUN_1801473e0` |
| `GetLastMezID` | `() → spawn_id: number` | Get last mezzed mob ID | `FUN_180147c80` |
| `ClearCharmTarget` | `() → success: boolean` | Clear charm tracking | `FUN_1801473e0` |
| `ClearMezTarget` | `() → success: boolean` | Clear mez tracking | `FUN_180147c80` |
| `AddToImmuneList` | `(spawn_name: string, zone_id: number, list_type: string) → success: boolean` | Add to mez/charm immune list | INI patterns |
| `RemoveFromImmuneList` | `(spawn_name: string, list_type: string) → success: boolean` | Remove from immune list | INI patterns |
| `IsImmune` | `(spawn_name: string, list_type: string) → boolean` | Check if spawn is immune | INI patterns |
| `BroadcastMez` | `(spawn_id: number) → success: boolean` | Broadcast mez to DanNet/EQBC | `FUN_180151670` |
| `ShouldTashFirst` | `(spawn_id: number, mob_count: number) → boolean` | Decide if tash before CC | `FUN_180147c80` |
| `ShouldSlowFirst` | `(spawn_id: number) → boolean` | Decide if slow before CC | `FUN_180147c80` |

### 10. Song Engine (`Core.Song`)
Bard-specific song management.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `CastPrioritySong` | `(song_type: string) → success: boolean` | Cast emergency song (DA) | `FUN_180149e30` |
| `CastInvisSong` | `() → success: boolean` | Cast Shauri's Sonorous Clouding | `FUN_180146dd0` |
| `ManageSelo` | `() → success: boolean` | Maintain Selo's speed song | `FUN_180150bb0` |
| `StopSong` | `() → success: boolean` | Stop current song with cooldown | `FUN_18007bc50` |
| `IsSinging` | `() → boolean` | Check if currently singing | `FUN_18007bc50` |
| `GetSongPriority` | `(song_id: number) → priority: number` | Get song priority from loadout | `FUN_180151230` |
| `LoadSongLoadout` | `(loadout_id: number) → success: boolean` | Load song set for mode | `FUN_180151230` |
| `GetCurrentLoadout` | `() → loadout_id: number` | Get active loadout ID | `FUN_180151230` |

### 11. Aura Management (`Core.Aura`)
Aura slot tracking and refresh.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetAuraSlots` | `() → table<slot_data>` | Get aura slot information | `FUN_18014e960` |
| `GetMaxAuras` | `() → count: number` | Get max auras from AA | `FUN_1800b50e0` |
| `RefreshAura` | `(slot: number, spell_id: number) → success: boolean` | Refresh aura if needed | `FUN_18014e960` |
| `IsSafeForAura` | `() → boolean` | Check if safe to cast aura | `FUN_1800c0640` |
| `GetAuraCooldown` | `() → timer_ms: number` | Get aura refresh cooldown | `FUN_18014e960` |

### 12. Settings (`Core.Settings`)
INI and SQLite settings management.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetBool` | `(key: string, default: boolean) → value: boolean` | Get boolean setting | CWTN INI patterns |
| `GetInt` | `(key: string, default: number) → value: number` | Get integer setting | CWTN INI patterns |
| `GetFloat` | `(key: string, default: number) → value: number` | Get float setting | CWTN INI patterns |
| `GetString` | `(key: string, default: string) → value: string` | Get string setting | CWTN INI patterns |
| `SetBool` | `(key: string, value: boolean) → success: boolean` | Set boolean setting | CWTN INI patterns |
| `SetInt` | `(key: string, value: number) → success: boolean` | Set integer setting | CWTN INI patterns |
| `SetFloat` | `(key: string, value: number) → success: boolean` | Set float setting | CWTN INI patterns |
| `SetString` | `(key: string, value: string) → success: boolean` | Set string setting | CWTN INI patterns |
| `LoadSettings` | `(character_name: string) → success: boolean` | Load settings from INI/SQLite | CWTN INI patterns |
| `SaveSettings` | `() → success: boolean` | Save settings to INI/SQLite | CWTN INI patterns |
| `QuerySQL` | `(query: string, params: table) → result: table` | Execute SQLite query | SQLite interface |
| `GetMode` | `() → mode: number` | Get current mode (0=manual, etc.) | Mode state |
| `SetMode` | `(mode: number) → success: boolean` | Set current mode | Mode state |

### 13. Navigation (`Core.Nav`)
Movement and positioning.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `NavigateTo` | `(x: number, y: number, z: number) → success: boolean` | Path to location | `FUN_180082d80` |
| `NavigateToSpawn` | `(spawn_id: number, distance: number) → success: boolean` | Path to spawn | `FUN_180082d80` |
| `StopNavigation` | `() → success: boolean` | Stop movement | `FUN_180082d80` |
| `IsNavigating` | `() → boolean` | Check if navigating | `FUN_180082d80` |
| `GetCampLocation` | `() → (x: number, y: number, z: number)` | Get camp location | Camp system |
| `SetCampLocation` | `(x: number, y: number, z: number) → success: boolean` | Set camp location | Camp system |
| `IsWithinCampRadius` | `(spawn_id: number) → boolean` | Check if within camp radius | Camp system |

### 14. Utility (`Core.Utility`)
General utilities and game state.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `GetTickCount` | `() → tick: number` | Get current game tick | `FUN_18007d580` |
| `GetZoneType` | `() → zone_type: number` | Get zone type (indoor/outdoor) | `FUN_18007b750` |
| `GetZoneID` | `() → zone_id: number` | Get current zone ID | Zone info |
| `GetZoneName` | `() → zone_name: string` | Get current zone name | Zone info |
| `IsInvis` | `() → boolean` | Check invis/visibility flag | `FUN_1800a64e0` |
| `Broadcast` | `(message: string, channel: string) → success: boolean` | Broadcast to DanNet/EQBC | `FUN_180151670` |
| `Print` | `(message: string) → success: boolean` | Print to MQ2 chat | Debug output |
| `GetClassID` | `() → class_id: number` | Get local player class ID | `FUN_1800b6d30` |
| `IsCombat` | `() → boolean` | Check if in combat | Mob count tracking |
| `GetMobCount` | `() → count: number` | Get nearby mob count | `FUN_1800831b0` |

### 15. ImGui (`Core.ImGui`)
UI rendering and callbacks.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `RegisterWindow` | `(name: string, render_callback: function) → success: boolean` | Register ImGui window | UI scaffolding |
| `UnregisterWindow` | `(name: string) → success: boolean` | Unregister window | UI scaffolding |
| `ShowWindow` | `(name: string) → success: boolean` | Show window | UI scaffolding |
| `HideWindow` | `(name: string) → success: boolean` | Hide window | UI scaffolding |
| `IsWindowVisible` | `(name: string) → boolean` | Check window visibility | UI scaffolding |
| `AddButton` | `(window: string, label: string, callback: function) → id: number` | Add button to window | UI scaffolding |
| `AddSlider` | `(window: string, label: string, min: number, max: number, default: number) → id: number` | Add slider to window | UI scaffolding |
| `AddCheckbox` | `(window: string, label: string, default: boolean) → id: number` | Add checkbox to window | UI scaffolding |
| `AddCombo` | `(window: string, label: string, options: table, default: number) → id: number` | Add combo box to window | UI scaffolding |
| `GetControlValue` | `(window: string, id: number) → value: any` | Get control value | UI scaffolding |
| `SetControlValue` | `(window: string, id: number, value: any) → success: boolean` | Set control value | UI scaffolding |

### 16. Lifecycle (`Core.Lifecycle`)
Plugin lifecycle management.

| Function | Signature | Description | Wraps |
|----------|-----------|-------------|-------|
| `RegisterPulse` | `(callback: function, priority: number) → id: number` | Register pulse callback | OnPulse dispatch |
| `UnregisterPulse` | `(id: number) → success: boolean` | Unregister pulse callback | OnPulse dispatch |
| `RegisterEvent` | `(event_type: string, callback: function) → id: number` | Register event handler | Event system |
| `UnregisterEvent` | `(id: number) → success: boolean` | Unregister event handler | Event system |
| `PausePlugin` | `() → success: boolean` | Pause plugin execution | Pause system |
| `ResumePlugin` | `() → success: boolean` | Resume plugin execution | Pause system |
| `IsPaused` | `() → boolean` | Check if plugin is paused | Pause system |
| `ReloadLua` | `() → success: boolean` | Reload Lua modules | Hot reload |
| `Shutdown` | `() → success: boolean` | Shutdown plugin | Plugin exports |

## Constants Appendix

### Spawn Types
```lua
Core.SPAWN_TYPE_PLAYER = 0x00
Core.SPAWN_TYPE_NPC = 0x01
Core.SPAWN_TYPE_CORPSE = 0x02
Core.SPAWN_TYPE_TRAP = 0x03
Core.SPAWN_TYPE_TIMER = 0x04
Core.SPAWN_TYPE_MERCENARY = 0x05
Core.SPAWN_TYPE_BECOME_NPC = 0x06
Core.SPAWN_TYPE_UNKNOWN = 0x07
```

### Class IDs
```lua
Core.CLASS_WARRIOR = 0x01
Core.CLASS_CLERIC = 0x02
Core.CLASS_PALADIN = 0x03
Core.CLASS_RANGER = 0x04
Core.CLASS_SHADOWKNIGHT = 0x05
Core.CLASS_DRUID = 0x06
Core.CLASS_MONK = 0x07
Core.CLASS_BARD = 0x08
Core.CLASS_ROGUE = 0x09
Core.CLASS_SHAMAN = 0x0A
Core.CLASS_NECROMANCER = 0x0B
Core.CLASS_WIZARD = 0x0C
Core.CLASS_MAGICIAN = 0x0D
Core.CLASS_ENCHANTER = 0x0E
Core.CLASS_BEASTLORD = 0x0F
Core.CLASS_BERSERKER = 0x10
Core.CLASS_MERCENARY = 0x11
```

### Target Types
```lua
Core.TARGET_SELF = 0x01
Core.TARGET_GROUP_VIABLE = 0x05
Core.TARGET_PET_OWNER = 0x08
Core.TARGET_RAID_ASSIST = 0x0D
Core.TARGET_RAID = 0x0E
Core.TARGET_SPECIAL = 0x0F
Core.TARGET_AE_PC_V2 = 0x24
Core.TARGET_UNDEAD = 0x26
Core.TARGET_CORPSE = 0x2D
Core.TARGET_PET = 0x2E
```

### Modes
```lua
Core.MODE_MANUAL = 0
Core.MODE_ASSIST = 1
Core.MODE_CHASE = 2
Core.MODE_TANK = 3
Core.MODE_PULLER = 4
Core.MODE_MA = 5
Core.MODE_CAMP = 6
Core.MODE_AFTERCAMP = 7
Core.MODE_PULL = 8
```

### Spell Types
```lua
Core.SPELL_TYPE_BENEFICIAL = 0
Core.SPELL_TYPE_DETRIMENTAL = 1
Core.SPELL_TYPE_ALWAYS = 2
```

## Cross-Reference Table

| Decompiled Function | API Function(s) | Subsystem |
|-------------------|----------------|-----------|
| `FUN_1800aac20` | `GetPosition`, `GetDistance`, `GetRace`, `ValidateRange`, `ValidateTargetType`, `ValidatePetOwner`, `ValidateMercOwner`, `CanSee`, `GetRange`, `GetTargetType`, `GetSpellType`, `GetCategory`, `GetSubcategory` | Spawn, Target, Spell |
| `FUN_18007b750` | `GetVelocity`, `GetLevel`, `GetMana`, `GetTarget`, `IsMoving`, `ValidateLevel`, `ValidateMana`, `ValidateMotion`, `ValidateEnvironment`, `IsBuffOnTarget`, `GetTargetBuffCount`, `GetTargetBuffArray`, `GetManaCost`, `GetName`, `GetEnvironmentFlag`, `GetStackingFlag` | Spawn, Cast, Buff, Spell |
| `FUN_180083e50` | `GetName`, `GetSpawnID`, `GetHP`, `GetHPPercent`, `GetCharacterBase` | Spawn, LocalPlayer |
| `FUN_1800a64e0` | `GetType`, `GetStandState`, `IsValidHealTarget`, `GetNumEffects`, `IsInvis` | Spawn, Target, Spell, Utility |
| `FUN_180074cf0` | `IsCasting`, `GetCastingSpellID`, `StopCasting` | Spawn, Cast |
| `FUN_1800a5570` | `GetPetID`, `GetSpawnByID`, `GetXTarget`, `GetXTargetCount`, `GetGroupMember`, `GetGroupMembers`, `GetXTargets`, `GetGroupMemberBySpawnID` | Spawn, LocalPlayer, Group |
| `FUN_18007d580` | `GetSpellGem`, `GetSpellGemRecast`, `GetGlobalRecovery`, `IsGemReady`, `FindGemForSpell`, `GetTickCount` | LocalPlayer, Cast, Utility |
| `FUN_1800a5280` | `GetClass`, `GetHealTarget`, `GetLowestHPPct`, `GetTriagePriority` | Spawn, Group |
| `FUN_180082410` | `GetMainAssist`, `IsMainAssist` | Group |
| `FUN_18008a0b0` | `GetRaidAssist`, `GetRaidAssists` | LocalPlayer, Group |
| `FUN_1800fb730` | `GetBuffTimestamp`, `CheckStacking`, `LoadCWTNBuffsINI`, `GetBuffAge`, `ShouldRefreshBuff`, `GetSpellGroup`, `IsBuffOnTargetByGroup` | Buff, Spell |
| `FUN_1800a94b0` | `GetReagents`, `HasReagents`, `GetMissingReagents`, `ConsumeReagents` | Reagent, Spell |
| `FUN_1800c2be0` | `GetAERange`, `ApplyFocusRangeMod` | Spell, Target |
| `FUN_180147c80` | `FindMezTarget`, `IsMezzed`, `GetLastMezID`, `ClearMezTarget`, `ShouldTashFirst`, `ShouldSlowFirst`, `IsValidMezTarget`, `IsValidMezTarget` | CC |
| `FUN_1801473e0` | `FindCharmTarget`, `IsCharmed`, `GetLastCharmID`, `ClearCharmTarget`, `IsValidCharmTarget` | CC |
| `FUN_18007bc50` | `StopSong`, `IsSinging` | Song |
| `FUN_180150bb0` | `ManageSelo` | Song |
| `FUN_180149e30` | `CastPrioritySong` | Song |
| `FUN_180146dd0` | `CastInvisSong` | Song |
| `FUN_18014e960` | `GetAuraSlots`, `RefreshAura`, `GetAuraCooldown` | Aura |
| `FUN_1800b50e0` | `GetMaxAuras` | Aura |
| `FUN_180082d80` | `NavigateTo`, `NavigateToSpawn`, `StopNavigation`, `IsNavigating` | Nav |
| `FUN_180151670` | `Broadcast`, `BroadcastMez` | Utility, CC |
| `FUN_180150a50` | (Plugin initialization, not directly exposed) | Lifecycle |
| `FUN_18007bd00` / `FUN_18007bd50` | `Cast` | Cast |
| `FUN_18007b090` | (Internal cast helper, used by `Cast`) | Cast |
| `FUN_18007b030` | `CastBurstOfLife` | Cast |
| `FUN_1800b0260` | (Internal CC target finder, used by `FindMezTarget`) | CC |
| `FUN_1800c3f90` | (Internal LoS/range validator for CC) | CC, Target |
| `FUN_1800738d0` | (Internal stacking check, used by `CheckStacking`) | Buff |
| `FUN_1800b2430` / `FUN_1800b05f0` | (Internal charm target finder, used by `FindCharmTarget`) | CC |
| `FUN_1800b23c0` | (Internal max mez level check) | CC |
| `FUN_1800c0640` | `IsSafeForAura` | Aura |
| `FUN_1800831b0` | `GetMobCount`, `IsCombat` | Utility |
| `FUN_1800b6d30` | `GetClassID` | Utility |
| `FUN_180085a00` | (Internal spawn resolver, used by various functions) | Spawn |
| `FUN_180082b10` | (Internal pet check, used by charm logic) | CC, Pet |

## Example Usage

### Basic Lua Class Module Template
```lua
-- Example: MQ2Cleric.lua
local Core = require("Core")

-- Register module with Core
Core.Lifecycle.RegisterPulse(function()
    -- Main pulse logic
    if Core.Settings.GetBool("UseAutoHeal", true) then
        local target_id = Core.Group.GetHealTarget(70, true) -- 70% threshold, include XTarget
        if target_id ~= 0 then
            local spell_id = Core.Spell.GetHighestLearnedByGroup(123) -- Example group ID
            local can_cast, reason = Core.Cast.CanCast(spell_id, target_id)
            if can_cast then
                Core.Cast.Cast(spell_id, target_id)
            end
        end
    end
end, 100)

-- Register settings UI
Core.ImGui.RegisterWindow("Cleric Settings", function()
    ImGui.Text("Healing Settings")
    local threshold = Core.Settings.GetInt("HealThreshold", 70)
    if ImGui.SliderInt("Heal Threshold %", threshold, 10, 100) then
        Core.Settings.SetInt("HealThreshold", threshold)
    end
end)

-- Load settings on init
Core.Settings.LoadSettings(Core.Utility.GetZoneName() .. "_" .. Core.LocalPlayer.GetName())
```

### Advanced Example: Enchanter CC Logic
```lua
-- MQ2Enchanter.lua CC subsystem
local function HandleMez()
    if not Core.Settings.GetBool("UseMez", false) then
        return
    end
    
    local target_id = Core.CC.FindMezTarget()
    if target_id == 0 then
        return
    end
    
    -- Tash first check
    if Core.CC.ShouldTashFirst(target_id, Core.Utility.GetMobCount()) then
        local tash_spell = Core.Spell.GetHighestLearnedByGroup(456) -- Tash group ID
        Core.Cast.Cast(tash_spell, target_id)
    end
    
    -- Cast mez
    local mez_spell = Core.Spell.GetHighestLearnedByGroup(789) -- Mez group ID
    Core.Cast.Cast(mez_spell, target_id)
    
    -- Broadcast to group
    Core.CC.BroadcastMez(target_id)
end

Core.Lifecycle.RegisterPulse(HandleMez, 200)
```

## Performance Considerations

1. **Minimize Lua ↔ C++ Calls**: Batch operations when possible (e.g., get all group member data in one call).
2. **Cache Results**: Cache frequently accessed data like spell IDs or settings.
3. **Use Light Userdata**: Spawn and spell pointers are lightweight wrappers; no heavy copying.
4. **Avoid Pulse Overhead**: Register pulse callbacks with appropriate priorities; unregister when not needed.
5. **SQLite Query Optimization**: Use parameterized queries and cache results for static data.

## Error Codes and Messages

Common error returns from API functions:

| Error Message | Meaning | Typical Resolution |
|---------------|---------|-------------------|
| "Invalid spawn ID" | Spawn not found or pointer null | Check if spawn exists with `GetSpawnByID` |
| "Spell not found" | Spell ID doesn't exist in game data | Verify spell ID with `GetHighestLearnedByGroup` |
| "Out of range" | Target outside spell range | Move closer or check `ValidateRange` first |
| "Insufficient mana" | Not enough mana to cast | Check `ValidateMana` before casting |
| "Target invalid for spell" | Target type mismatch | Use `GetValidTargetsForSpell` to filter |
| "Gem not ready" | Spell gem on cooldown | Check `IsGemReady` or wait for timer |
| "Global recovery active" | Global cooldown in progress | Wait for `GetGlobalRecovery` timer |
| "Stunned" | Player is stunned | Wait for stun to wear off |
| "No line of sight" | Cannot see target | Reposition or check `CanSee` |
| "Missing reagents" | Required components missing | Check `HasReagents` or `GetMissingReagents` |
| "Buff would not stack" | Stacking conflict detected | Use `CheckStacking` before casting buffs |
| "Not safe to cast" | In combat or moving for aura | Check `IsSafeForAura` conditions |

## Versioning and Compatibility

The API follows semantic versioning:
- **Major version**: Breaking changes to function signatures or behavior
- **Minor version**: New functions or features, backward compatible
- **Patch version**: Bug fixes and performance improvements

Lua modules should declare required API version:
```lua
Core.Lifecycle.SetAPIVersion("1.0.0")
```

## Security Notes

1. **No Unsafe Operations**: Lua modules cannot directly access memory or call arbitrary functions.
2. **Input Validation**: All parameters are validated in C++ before use.
3. **Resource Limits**: SQLite queries have timeout and row limits.
4. **Sandboxing**: Lua state is isolated with limited standard library access.
5. **No Network Access**: Lua modules cannot make network calls.

## Debugging and Logging

Enable debug output:
```lua
Core.Settings.SetBool("DebugMode", true)
Core.Utility.Print("Debug: Starting heal check...")
```

Check function success:
```lua
local success, err = Core.Cast.Cast(spell_id, target_id)
if not success then
    Core.Utility.Print("Cast failed: " .. err)
end
```

## Migration from CWTN DLLs

For developers familiar with CWTN plugin commands:

| CWTN Command/Setting | Equivalent API Call |
|----------------------|-------------------|
| `/plugin mq2cleric healon` | `Core.Settings.SetBool("UseAutoHeal", true)` |
| `/brd loadout 2` | `Core.Song.LoadSongLoadout(2)` |
| `UseMez=1` in INI | `Core.Settings.SetBool("UseMez", true)` |
| Heal threshold logic | `Core.Group.GetHealTarget(threshold, include_xtarget)` |
| Buff checking | `Core.Buff.IsBuffOnTargetByGroup(target_id, group_id)` |
| Casting with validation | `Core.Cast.CanCast(spell_id, target_id)` then `Core.Cast.Cast()` |

This completes the C++ Core Lua API Reference. The API provides comprehensive coverage of all subsystems identified through reverse engineering while maintaining safety, performance, and flexibility for class-specific Lua modules.