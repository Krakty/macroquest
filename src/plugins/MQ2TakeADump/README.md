MQ2TakeADump.dll: MacroQuest2's extension DLL for EverQuest
Copyright (C) 2018 Maudigan

This program is free software; you can redistribute it and/or modify it under the terms of
the GNU General Public License, version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.


DESCRIPTION:
-----------------------------------------------------------------------------------------
This plugin will allow you to dump EQ information out to CSV files, such as doors,
groundspawns, objects, NPCs, current zone and zonepoints. To take a dump use the
/takeadump command. You can optionally send a parameter to limit the dump to just
the specific data you are after, e.g. "/takeadump door". For the list of parameters
try "/takeadump help".

When run it will drop all the varios data dumps as CSV files in the macroquest
directory with the zone name, type, and timestamp in the filename.

The first row of the CSV is a description of the field (taken from the MQ objects).
The second row is a description of the datatype. Numeric and float data shows up as
plain numbers, boolean show up as true or false literals, and strings show up
as text surrounded by double quotes (the quotes may not display in excel but they will
in notepad).

"/takeadump target" is a special case. If you use this you have to have a target. It
will dump that targets coordinates until it dies, you zone or you lose the target.
One row will dump out every time your targets heading changes.


COMMANDS:
-----------------------------------------------------------------------------------------

### Data Dumps

| Command | Description |
|---------|-------------|
| `/takeadump` | Dump all data types (doors, ground items, objects, NPCs, zone, zonepoints) |
| `/takeadump all` | Same as no parameter |
| `/takeadump door` | Dump door data only |
| `/takeadump ground` | Dump ground items only |
| `/takeadump object` | Dump placed objects only |
| `/takeadump npc` | Dump NPC/spawn data only |
| `/takeadump myzone` | Dump current zone info only |
| `/takeadump zonepoint` | Dump zone connection points only |
| `/takeadump target` | Start recording target's coordinates (see below) |
| `/takeadump path` | Alias for target |
| `/takeadump merchant` | Dump item IDs from the open merchant window |
| `/takeadump help` | Show usage info |

Note: "target" is excluded from "all" because it starts a continuous recording session.

### Timer Commands

A built-in timer for calculating pause durations when recording pathing data.

| Command | Description |
|---------|-------------|
| `/takeadump tstart` | Start the timer (resets any existing pause) |
| `/takeadump tpause` | Pause the timer, shows elapsed ms |
| `/takeadump treset` | Reset the timer to zero |
| `/takeadump tcontinue` | Unpause a paused timer |

### TLO: ${TAD}

Timer values are accessible via the TAD TLO for use in macros:

| Member | Type | Description |
|--------|------|-------------|
| `${TAD.Seconds}` | int | Elapsed seconds since timer start |
| `${TAD.SecondsReset}` | int | Elapsed seconds, resets timer on read |
| `${TAD.Milliseconds}` | int | Elapsed milliseconds since timer start |
| `${TAD.MillisecondsReset}` | int | Elapsed milliseconds, resets timer on read |


OUTPUT:
-----------------------------------------------------------------------------------------
CSV files are written to a "Dumps" subdirectory inside the MacroQuest directory. Filenames
include the zone short name, dump type, and timestamp.

Each CSV has:
- Row 1: Field names (from MQ struct definitions)
- Row 2: Data type descriptions
- Row 3+: Data rows


ALIGNMENT VALIDATION:
-----------------------------------------------------------------------------------------
After each patch, the plugin validates struct alignment by checking an element near the
bottom of each data structure. If alignment is off, you will see a red warning:

    [MQ2TakeADump] ALIGNMENT ISSUE: There may be an issue with <struct>

If you see this, spot-check your dumps against known values. The struct definitions in
eqlib may need updating for the current patch.


FORK INTEGRATION NOTES:
-----------------------------------------------------------------------------------------
This is Maudigan's original plugin integrated into our Krakty/macroquest fork. It builds
as part of the standard plugin set and requires no special configuration.

The plugin registers on load and removes its command on unload. No INI files or
persistent state beyond the current session.


REVISION HISTORY
Date        Author          Description
-----------------------------------------------------------------------------------------
20180922    Maudigan        Initial revision
20180929    Maudigan        Added path recording
20181006    Maudigan        Cleaned up output file name
                            Put output files into a "Dumps" folder
                            Stopped target output for "/takeadump all"; must request it now
                            Added some missing elements namely FindBits and Level to NPC
20181008    Maudigan        Split the groundspawn and objects into separate commands/files
20181013    Maudigan        Spawn structure updated for patch
20181104    Maudigan        Fixed some changed data types for the new client
20190209    Maudigan        Added "/takeadump merchant" to dump the item IDs in MerchantWnd
20190309    Maudigan        Misalignment fix in spawninfo struct, added alignment validation
20190511    Maudigan        Repairs after patch
20190803    Maudigan        Updated for 20190731
20190810    Maudigan        Added timer (tstart/tpause/treset/tcontinue) and TAD TLO
20190824    Maudigan        Added markup to merchant window
20200809    Maudigan        Update for 20200722
