# MQ2DumpItem

A MacroQuest plugin that dumps raw memory bytes from EverQuest game structs. Essential for verifying field offsets after EQ patches when MSVC PGO randomizes struct field order.

## Commands

| Command | Description |
|---------|-------------|
| `/dumpitem <name>` | Dump ItemBase from your inventory |
| `/dumpitem merch <name>` | Dump ItemBase from merchant's stock |
| `/dumpitemdef <name>` | Dump ItemDefinition (static item template, 0x688 bytes) |
| `/dumpspawn self [start] [end]` | Dump your PlayerClient struct |
| `/dumpspawn target [start] [end]` | Dump your target's PlayerClient struct |
| `/dumpprofile [start] [end]` | Dump PcProfile (buffs, skills, inventory) |
| `/dumpwnd <name> [start] [end]` | Dump any UI window (CXWnd) |
| `/dumpeq [start] [end]` | Dump CEverQuest instance |

All offset arguments are hex (e.g., `0x100 0x200`). Default range varies by struct.

## Struct Coverage

| Struct | Size | Access Via |
|--------|------|-----------|
| ItemBase | 0x108 | FindItemByName / merchant list |
| ItemDefinition | 0x688 | pItem->GetItemDefinition() |
| PlayerClient | 0x20D8 | pLocalPlayer / pTarget |
| PcProfile | 0x6E98 | GetPcProfile() |
| CXWnd | 0x268+ | FindMQ2Window() |
| CEverQuest | 0x19710 | pEverQuest |

## Example: Finding where StackCount lives

```
/dumpitem Cloudy Potion
```
```
=== Raw ItemBase dump for: Cloudy Potion (ID: 14514) [inventory item] ===
  +0x000: 1585578704  (0x5E8202D0)     <- vtable
  +0x008:         -1  (0xFFFFFFFF)     <- RealEstateID
  +0x00C:         20  (0x00000014)     <- StackCount (we have 20!)
  ...
  +0x060:          1  (0x00000001)     <- Charges (1-charge potion)
  ...
  +0x0FC:          2  (0x00000002)     <- MerchantQuantity
```

## Example: Finding EnduranceMax in PlayerClient

After dying (low endurance), check `/echo ${Me.MaxEndurance}` to get your max, then dump the spawn struct to find which offset holds that value:

```
/dumpspawn self 0x210 0x220
```

Look for your MaxEndurance value in the dump. We found it at 0x214 — the previous layout had it at 0x2CC which was always 0 (wrong).

## Example: Verifying merchant stock

While at a merchant with 2 Fine Steel Morning Stars:

```
/dumpitem merch Fine Steel Morning Star
```
```
  +0x0FC:          2  (0x00000002)     <- MerchantQuantity matches!
```

## Why This Exists

EverQuest's MSVC PGO compiler **randomizes struct field order** between patches. IDA/Ghidra analysis can identify field offsets but cannot reliably name same-type fields (e.g., two `int` fields that both init to 0 or 1 are indistinguishable).

This plugin provides **ground truth**: find a game object with known values, dump the raw bytes, and see which offset holds which value. No guessing.

### Fields identified using this tool (Apr 7 2026 patch):
- **StackCount** at 0x0C (not 0x60) — stack of 20 potions showed 20 at 0x0C
- **Charges** at 0x60 (not 0x4C) — 1-charge potion showed 1 at 0x60
- **MerchantQuantity** at 0x0FC (not 0x60) — merchant with 2 items showed 2 at 0x0FC
- **EnduranceMax** at 0x214 (not 0x2CC) — 0x2CC was always 0, 0x214 held the correct value

## Integration with MQMCPServer

When used with [MQMCPServer](https://github.com/niccellular/macroquest), Claude Code can execute dump commands and read results via MCP — enabling fully automated field verification:

```
execute_command("/dumpitem Cloudy Potion")
→ get_state() → parse recent_chat for raw byte values
→ compare against expected values
```

## Building

Requires MacroQuest source tree. Place in `src/plugins/` and build with MSBuild:

```bash
MSBuild MQ2DumpItem.vcxproj /p:Configuration=Release /p:Platform=x64
```

## License

Same as MacroQuest (GPLv2).
