# MQMemDump

Raw memory dumper built into MQ2Main for reverse engineering EverQuest struct layouts. Dumps the local player's spawn struct and the eqgame.exe module image to binary files for offline diffing.

## Commands

### /memdump \<tag\>

Dumps `0x2200` bytes starting at `pLocalPlayer` (the PlayerClient struct) to:

```
Logs/memdump_spawn_<tag>.bin
```

The tag is freeform text -- use it to label what state the character is in when you take the dump.

Examples:
```
/memdump baseline
/memdump sitting
/memdump mounted
/memdump afkon
```

### /memdump eqgame

Dumps the entire eqgame.exe module image (base address through SizeOfImage) to:

```
Logs/memdump_eqgame.bin
```

### /memdump help

Prints usage info.

## Auto Zone-In Dump

On every zone-in, MQMemDump automatically dumps both the spawn struct and the eqgame.exe image after 30 pulse ticks (~0.5 seconds). These are written as:

```
Logs/memdump_spawn_autozonedin.bin
Logs/memdump_eqgame.bin
```

This fires once per session (resets on plugin reload). The auto dump gives you a baseline without having to remember to run the command manually.

## Output Files

All files are written to the MQ `Logs/` directory as raw binary.

| File | Contents | Size |
|------|----------|------|
| `memdump_spawn_<tag>.bin` | PlayerClient struct starting at pLocalPlayer | 0x2200 (8704) bytes |
| `memdump_eqgame.bin` | Full eqgame.exe module image | Varies (~20-30MB) |

## Usage Workflow

The basic workflow for identifying field offsets in the PlayerClient/spawn struct:

1. Zone in (auto dump creates `memdump_spawn_autozonedin.bin` as baseline)
2. Change some character state (sit down, mount up, toggle AFK, etc.)
3. Run `/memdump <tag>` describing the new state
4. Diff the two binary files to find which offsets changed

### Diffing with diff_dumps.py

The companion script `tools/diff_dumps.py` (in the MQ-RE repo) diffs two memdump files and shows byte-level changes:

```bash
python tools/diff_dumps.py Logs/memdump_spawn_autozonedin.bin Logs/memdump_spawn_sitting.bin
```

This outputs offset, old value, and new value for every byte that differs.

### Example: Finding the "sitting" field offset

```
# 1. Zone in -- auto dump fires
# 2. Stand up, verify baseline state
/memdump standing

# 3. Sit down
/sit
/memdump sitting

# 4. On your analysis machine, diff the two:
python tools/diff_dumps.py memdump_spawn_standing.bin memdump_spawn_sitting.bin

# 5. Look for offsets that changed from standing to sitting values.
#    Cross-reference with known struct definitions to identify the field.
```

Repeat with different state changes to isolate individual fields. Fields that change together (e.g., animation state + standstate) will show up as correlated offset groups.
