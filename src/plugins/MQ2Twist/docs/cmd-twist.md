---
tags:
  - command
---

# /twist

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/twist [<gem>]... [option] [setting]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Controls the songs and spells you cast in a repeatable queue
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `(no option)` | Displays help text |
| `<#> [<#>]...` | Twists in the order given. Valid options are 1 through 14 for song gems, and 21 thru 40 for item clicks. These may be mixed in any order, and repeats are allowable. If a song is specified with a duration longer than standard (ie, selos) that song will be twisted based on its duration.  For example, riz+mana+selos would be a 2 song twist with selos pulsed every 2.5 min. |
| `hold` | Pause twisting and sing only the specified song |
| `once <#> [<#>]...` | Twists once in the order given, then reverts to original twist. |
| `start` | Resume the twist after using /twist hold or /twist stop. |
| `reset` | Reset timers for item clicks and long duration songs. |
| `clear` | Stop twist and clear song list. |
| `delay <#>` | Add extra delay to twists. 10ths of a second, minimum of 30, default 33. (standard gem refresh delay) |
| `recast <#>` | Add additional delay to spells with a recast time. 10ths of a second, minimum of 0, default 0. |
| `adjust <#>` | in ticks, how early to recast long duration songs. (long defined as greater than 3 ticks). e.g. If a song is normally 10 ticks, by default recast begins on the 9th tick, and `/twist adjust 1` will instead recast on the 8th tick. |
| `stop|end|off` | stop twisting. Does not clear the twist queue. |
| `reload|load` | Reloads the INI file |
| `slots|list` | List the slots defined in the INI and their #'s. |
| `set <click#> <casttime#> <recasttime#> <name> <slot>` | Sets a slot to be defined in the INI. e.g. `/twist set 26 32 120 "Cassindra's Chorus of Clarity" AA` |
| `quiet` | Toggles songs listing and start/stop messages for one-shot twists |

## Examples

- Sing gem 1 forever  
  `/twist 1`
- Twist gems 1,2, and 3 forever  
  `/twist 1 2 3`
- Twist gems 1,2,3, and clicky 23, forever  
  `/twist 1 2 3 23`
- Sing gem 4 forever, until another singing-related /twist command is given  
  `/twist hold 4`
- Set Click_26 to CastTime=32, ReCastTime=120, Name="Cassindra's Chorus of Clarity", Slot=AA ; and save to INI  
  `/twist set 26 32 120 "Cassindra's Chorus of Clarity" AA`