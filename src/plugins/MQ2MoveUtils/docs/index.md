---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2moveutils.144/"
support_link: "https://www.redguides.com/community/threads/mq2moveutils.66853/"
repository: "https://github.com/RedGuides/MQ2MoveUtils"
config: "MQ2MoveUtils.ini"
authors: "eqmule, Knightly, outlander, SwiftyMUSE, tonio, Lax"
tagline: "This plugin performs basic monotonous movement tasks."
acknowledgements: "MQ2Twister by CyberTech"
---

# MQ2MoveUtils

<!--desc-start-->
MQ2MoveUtils was created in early 2004 by tonio, giving players a significant improvement over EverQuest's built-in /follow command with options like /stick, as well as a handy way for bards to run "circle" macros.
<!--desc-end-->

Today, **the main features of MQ2MoveUtils are as follows:**

* **Stick**: This is the main use for this plugin and allows you to "stick" a certain distance from your target. It can be set to always stick behind the target, or always in front if you're tanking.
* **MoveTo**: Move to a certain point or target. This has some built-in logic that will allow it to move around obstacles.
* **MakeCamp**: You can set a spot as a "camp", so that you can return to it if you get too far away, or after combat, etc.
* **Circle**: Run in a circle.

## Commands

<a href="cmd-stick/">
{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-stick.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-stick.md') }}

<a href="cmd-circle/">
{% 
  include-markdown "projects/mq2moveutils/cmd-circle.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-circle.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-circle.md') }}

<a href="cmd-makecamp/">
{% 
  include-markdown "projects/mq2moveutils/cmd-makecamp.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-makecamp.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-makecamp.md') }}

<a href="cmd-moveto/">
{% 
  include-markdown "projects/mq2moveutils/cmd-moveto.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-moveto.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-moveto.md') }}

<a href="cmd-rootme/">
{% 
  include-markdown "projects/mq2moveutils/cmd-rootme.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-rootme.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-rootme.md') }}

<a href="cmd-calcangle/">
{% 
  include-markdown "projects/mq2moveutils/cmd-calcangle.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2moveutils/cmd-calcangle.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2moveutils/cmd-calcangle.md') }}

### INI Options

#### General settings

| Setting | Value | What it does |
|---------|-------|--------------|
| AllowMove | 10.0+ | Anti-orbit setting for true/loose heading representing angular distance before moving forward. |
| AutoPause | on off | Pauses command if casting spells (non-bard), stunned, rooted, sitting, FD, or self targeted (non-hold). |
| AutoPauseMsg | on off | Displays output when AutoPause halts movement. |
| AutoSave | on off | Automatically save ini file when using 'toggle' or 'set'. |
| AutoUW | on off | Automatically use 'uw' heading adjustments when character is under water. |
| BreakKeyboard | on off | Break command from keyboard press. |
| BreakMouse | on off | Break command from mouselook usage. |
| BreakonGM | on off | Break current command and prevent command usage if GM enters zone. |
| BreakonSummon | on off | Halt command and ability to use commands if summoned beyond certain distance. |
| DistSummon | 2.0+ | Distance moved in a single pulse to trigger breakonsummon (if on). |
| FeignSupport | on off | FD support waits for you to stand up manually before moving, if feigned. |
| Heading | true loose fast| Type of heading adjustments plugin will use (fast=instant, loose=gradual emulated shift, true=real kb presses). |
| HideHelp | on off | Never automatically display help output unless requested. |
| KeyboardPause | on off | Pause command for a delay if keyboard press. |
| LockPause | on off | Plugin will not automatically unpause under any circumstance unless user unpauses. |
| MousePause | on off | Pause command for a delay if mouselook used. |
| PauseMinDelay | 125+ (in ms) | Minimum delay before resuming from mpause/mousepause. |
| PauseMaxDelay | 125 above min (in ms) | Maximum delay before resuming from mpause/mousepause. |
| SaveByChar | on off | Save [server.Charname] section of ini file for individual character settings. |
| TurnRate | 10.0 to 100.0 | Rate at which loose heading emulates turns. |
| UseWindow | on off | Uses a custom user-placed window for all moveutils output if enabled. |
| Verbosity | on off | ChatWnd output for basic command info. |
| FullVerbosity | on off | ChatWnd output for enhanced plugin info. |
| TotalSilence | on off | Overrides verb/fullverb, silence ChatWnd output except for critical or user-requested messages. |
| VerbosityFlags |  | See verbosity section of this wiki. |
| WinEQ | on off | When enabled, moveutils uses feigned movement simulation due to the bug of WinEQ2 holding down alt keys and mouse buttons in background sessions. |

- **NOTE: If WinEQ is enabled, true heading is NOT possible!***

#### Stick Settings

- This section is for settings related to [/stick](cmd-stick.md).

| Setting | Value | What it does |
|---------|-------|--------------|
| AlwaysUW | on off | If enabled, stick will always use the 'uw' parameter as if it were typed inline. |
| ArcBehind | 5.1 to 259.9 | User can configure angular distance arc that "stick behind" uses. |
| ArcNotFront | 5.1 to 259.9 | User can configure angular distance arc that "stick !front" uses. |
| AwareNotAggro | on off | Detect aggro loss if using stick front. |
| BreakonGate | on off | Break from stick if "target Gates." message occurs. |
| BreakonHit | on off | When enabled, stick will halt if user is being attacked. |
| BreakonWarp | on off | Break from stick if target warps beyond certain distance. |
| PauseonWarp | on off | Pause stick unless target gets back in range if warps beyond certain distance (break or pause, can't have both). |
| DelayStrafe | on off | Delay strafing movement when position adjustment required (good for stopping endless circling if aggro is gained). |
| DistBackup | 1.0+ | If you are within this distance when stick turned on, stick will walk backwards rather than spin in a circle to move to target. |
| DistBreak | 1.0+ | Distance mob moved in a single pulse to trigger breakonwarp (if on). |
| DistMod | 0.0+ | Adjust default/supplied stick distance by this amount. |
| DistMod% | 0.0+ (represents a percentage) | Adjust default/supplied stick distance by this percent. |
| DistSnaproll | 1.0+ | Distance behind target snaproll will move before stopping and turning to face target. |
| RandomArc | on off | Randomize min/max arc for any strafe-based stick (so you do not always stick to the exact same spot of a mob). |
| StrafeMinDelay | 125+ (in ms) | Minimum delay before attempting to strafe, if delaystrafe enabled. |
| StrafeMaxDelay | 125 above min (in ms) | Maximum delay before attempting to strafe, if delaystrafe enabled. |
| UseBackward | on off | When enabled, stick will walk backward rather than turn to face if within DistBackup of target. |
| UseFleeing | on off | When enabled, "stick front" will not attempt to position in front of the mob when target begins to flee. |
| UseWalk | on off | When enabled, stick uses walking when close to the target for precise movements and preventing overshooting. |

#### MakeCamp Settings

- This section is for settings related to [/makecamp](cmd-makecamp.md).

| Setting | Value | What it does |
|---------|-------|--------------|
| Bearing | # | Bearing of scatter. |
| CampRadius | 5.0+ | Default camp radius size. |
| LeashLength= | #.# >= camp radius | Length of leash. |
| MaxDelay | 125 above min (in ms) | Maximum delay before auto-returning to camp. |
| MinDelay | 125+ (in ms) | Minimum delay before auto-returning to camp. |
| RealtimePlayer | on off | When enabled, "makecamp player" gets realtime location updates of player and adjusts returning on the fly. |
| ReturnHaveTarget | on off | If on, Auto-Return to camp even if you have a target (default behavior is return only if no target). |
| ReturnNoAggro | on off | Auto-Return to camp only if not aggro. |
| ReturnNotLooting | on off | Do not Auto-Return to camp if looting a corpse. |
| ScatDist | 1.0+ | Distance from camp center to perform scatter. |
| ScatSize | 1.0+ | Radius size of scatter area. |
| UseLeash | on off | Do not allow character to move beyond LeashLength. |
| UseScatter | on off | Use specific scatter values instead of random return location. |

#### MoveTo Settings

- This section is for settings related to [/moveto](cmd-moveto.md).

| Setting | Value | What it does |
|---------|-------|--------------|
| AlwaysUW | on off | If enabled moveto will always use the 'uw' parameter as if it were typed inline. |
| ArrivalDist | 1.0+ | Distance considered acceptable to have arrived at destination. |
| ArrivalDistX | 1.0+ | Distance considered acceptable to have arrived at destination when using precisex. |
| ArrivalDistY | 1.0+ | Distance considered acceptable to have arrived at destination when using precisey. |
| BreakonAggro | on off | When enabled, moveto will halt if aggro is gained (crossed swords in player window). |
| BreakonHit | on off | When enabled, moveto will halt if user is being attacked. |
| DistBackup | 1.0+ | Moveto will walk backwards to location if within this distance rather than spin to face destination first. |
| MoveToMod | 0.0+ | Modifier applied to moveto arrivaldist. |
| UseBackward | on off | When enabled, moveto will use backward movement if within DistBackup (applies to makecamp returns). |
| UseWalk | on off | Turn on walk when close to moveto location and camp return spot for precise movement. |

#### Circle Settings

- This section is for settings related to [/circle](cmd-circle.md).

| Setting | Value | What it does |
|---------|-------|--------------|
| Backward | on off | Always run backwards instead of forwards when circling. |
| CCW | on off | Always run in a counterclockwise (ccw) circle instead of default clockwise. |
| Drunken | on off | Always use drunken circling. |
| RadiusSize | 5.0+ | Default radius size of circle. |

#### StuckLogic Settings

- This section is for settings related to stucklogic.

| Setting | Value | What it does |
|---------|-------|--------------|
| StuckLogic | on off | If you have enabled stucklogic, detects and attempts to auto-correct getting stuck while moving. |
| DistStuck | 0.01+ | Distance needed to have moved or else stuck (compared against an average). |
| PulseCheck | 1+ | Amount of pulses used to calculate moving average. |
| PulseUnstuck | 1+ | Number of pulses successfully moved forward to be considered unstuck. |
| TryToJump | on off | Attempt to jump to help get unstuck. |
| TurnHalf | on off | If you have have turned halfway and failed to get unstuck, reset heading and try other direction instead. |

#### Window Settings

| Setting | Value | What it does |
|---------|-------|--------------|
| ChatTop | | See EQ XML UI file settings |
| ChatBottom | | See EQ XML UI file settings |
| ChatLeft | | See EQ XML UI file settings |
| ChatRight | | See EQ XML UI file settings |
| Fades | | See EQ XML UI file settings |
| Alpha | | See EQ XML UI file settings |
| FadeToAlpha | | See EQ XML UI file settings |
| Duration | | See EQ XML UI file settings |
| Locked | | See EQ XML UI file settings |
| Delay | | See EQ XML UI file settings |
| BGType | | See EQ XML UI file settings |
| BGTint.red | | See EQ XML UI file settings |
| BGTint.green | | See EQ XML UI file settings |
| BGTint.blue | | See EQ XML UI file settings |
| FontSize | 1 to 10 | default font size for window |
| WindowTitle | | custom user title for the window |

#### [yourserver.yourcharacter]

If savebychar is on, this section will be created for every character. The settings in this section could be desired to vary on a char-by-char basis (including WINDOW settings) with the exception of one value:

| Setting | Value | What it does |
|---------|-------|--------------|
| DisregardMe | true false | If you want custom character values to load for some characters but this specific character to use the default values instead, set this to true and though a lot of entries will be written to this section, they will be ignored for this specific character. |

## Example config file

*Default config file, MQ2MoveUtils.ini*
```ini
[Defaults]
AllowMove=32.0
AutoPause=on
AutoPauseMsg=on
AutoSave=on
AutoUW=off
BreakKeyboard=on
BreakMouse=off
BreakOnGM=on
BreakOnSummon=off
DistSummon=8.00
FeignSupport=off
Heading=true
HideHelp=off
KeyboardPause=off
MousePause=off
LockPause=off
PauseMinDelay=500
PauseMaxDelay=5000
SaveByChar=on
TurnRate=14.00
UseWindow=off
Verbosity=on
FullVerbosity=on
TotalSilence=off
VerbosityFlags=33554431
WinEQ=off

[Stick]
AlwaysUW=off
AwareNotAggro=off
ArcBehind=45.0
ArcNotFront=135.0
BreakOnGate=on
BreakOnHit=off
BreakOnTarget=off
BreakOnWarp=on
PauseOnWarp=off
DelayStrafe=on
DistBackup=10.0
DistBreak=250.0
DistMod=0.0
DistMod%=1.0
DistSnaproll=10.0
RandomArc=off
StrafeMinDelay=1500
StrafeMaxDelay=3000
UseBackward=on
UseFleeing=on
UseWalk=off   

[MakeCamp]
CampRadius=40.00
MinDelay=500
MaxDelay=1500
RealtimePlayer=off
ReturnHaveTarget=off
ReturnNoAggro=off
ReturnNotLooting=off
UseLeash=off
LeashLength=50.00
UseScatter=off
Bearing=0.00
ScatDist=10.00
ScatSize=10.00

[MoveTo]
AlwaysUW=off
ArrivalDist=10.0
ArrivalDistX=10.0
ArrivalDistY=10.0
BreakOnAggro=off
BreakOnHit=off
DistBackup=30.0
MoveToMod=0.0
UseBackward=off
UseWalk=on

[Circle]
Backward=off
CCW=off
Drunken=off
RadiusSize=30.0

[StuckLogic]
StuckLogic=on
DistStuck=0.10
PulseCheck=6
PulseUnstuck=10
TryToJump=off
TurnHalf=on

[Window]
ChatTop=10
ChatBottom=210
ChatLeft=10
ChatRight=410
Fades=0
Alpha=255
FadeToAlpha=255
Duration=500
Locked=0
Delay=2000
BGType=1
BGTint.red=255
BGTint.green=255
BGTint.blue=255
FontSize=2
WindowTitle=MoveUtils

[yourserver.yourcharacter]
DisregardMe=false
AllowMove=32.0
ArcBehind=45.0
ArcNotFront=135.0
AutoSave=on
AutoUW=off
DistBreak=250.0
BreakOnGate=on
BreakOnWarp=on
PauseOnWarp=off
LockPause=off
DistSnaproll=10.0
FeignSupport=off
Heading=true
LeashLength=50.00
UseLeash=off
UseWindow=off
Verbosity=on
FullVerbosity=on
VerbosityFlags=33554431
CampRadius=40.00
RealtimePlayer=off
UseScatter=off
Bearing=0.00
ScatDist=10.00
ScatSize=10.00
```

## Debugging

The verbosity system has been revamped to use bit flags for superior control of what messages will be displayed by the plugin. The older system has not been removed - if this is difficult to understand you may still use verbosity, fullverbosity, and totalsilence as before. For those familiar with bit flags, the flags table is below. If you have never worked with bit flags before, here is a brief summary of how to use the information below. Each subset of messages is assigned a numerical value. By adding the numerical values of the messages you want on together, you are able to customize each message that is shown or not shown. 

Examples:

- If you only wanted the plugin to display 'settings' and 'errors', you would look at the value of settings in the table below (8192) and the value of errors (4194304) and add them together to get (4202496). By setting your verbosity flag to 4202496 (using the set verbflags parameter or by saving the value in the INI file) the plugin would then filter out everything except messages related to changing settings or error messages.
- If you only wanted to display 'stick verbosity' messages and nothing else, you would look up the value in the table below (32) and set your flags to 32 without adding anything to it.
- If you want to display a large number of messages, you continue to add them all together and use the total. To display 'autopause', 'movepause', 'stick verbosity', 'stick fullverbosity', 'settings' and 'errors', you would add all their values from the below table (1 + 2 + 32 + 64 + 8192 + 4194304 = 4202595) and use that number for your flags setting (/stick set verbflags 4202595)

### Flags Table

| Flag | Description |
|------|-------------|
| 0 | total plugin silence |
| 1 | autopause |
| 2 | movepause, mousepause, breakonkb |
| 4 | breakonmouse |
| 8 | feign support |
| 16 | hidehelp |
| 32 | stick verbosity |
| 64 | stick full verbosity |
| 128 | moveto verbosity |
| 256 | moveto full verbosity |
| 512 | makecamp verbosity |
| 1024 | makecamp full verbosity |
| 2048 | circle verbosity |
| 4096 | circle full verbosity |
| 8192 | settings |
| 16384 | file input / output |
| 32768 | breakonwarp |
| 65536 | breakonaggro |
| 131072 | breakonhit |
| 262144 | breakonsummon |
| 524288 | breakongm |
| 1048576 | breakongate |
| 2097152 | stick always |
| 4194304 | error messages |
| 8388608 | arc randomization |
| 16777216 | pause / unpause |
| 2720 | prior 'verbosity' setting |
| 11736390 | prior 'fullverbosity' setting |
| 33554431 | all messages enabled |

### Flag Messages

- The following are the messages that are tied to each flag:

| Flag | Message |
|------|---------|
| 1 | autopause AutoPause halting movement... (when autopaused if autopause is enabled)Movement pausing due to self target... (if self targeted during a stick with autopause off) |
| 2 | movepause, mousepause, breakonkb Current command ended from manual movement. Resuming previous command from movement pause. |
| 4 | breakonmouse Current command ended from mouse movement. |
| 8 | feign support Not standing as you are currently Feign Death. |
| 16 | hidehelp Hidehelp when turned on prevents the help output (seen in /stick help) from being automatically output if you input a command incorrectly |
| 32 | stick verbosity You are now sticking to TargetName. You are no longer sticking to anything. You will now stick to every valid NPC target supplied. |
| 64 | stick full verbosity Dir(ANY) Dist(10.0) Head(true) ID(31337) UW MB HEALER |
| 128 | moveto verbosity Moveto off. Arrived at moveto location |
| 256 | moveto full verbosity Moving to loc #, # Dist(10) Head(true) |
| 512 | makecamp verbosity MakeCamp actived. Y(#) X(#) Radius(#) Leash(#) LeashLen(#) Min(#) Max(#)MakeCamp returning to within camp radius immediatelyMakeCamp returning to altcamp immediately.MakeCamp returning to altcamp immediately. Current camp now off.MakeCamp player ended due to player leaving/deathOutside of leash length, breaking from current command |
| 1024 | makecamp full verbosity Ended '/moveto' or '/makecamp return' because leash is on. |
| 2048 | circle verbosity Circling radius (#), center (#, #) off |
| 4096 | circle full verbosity none at this time |
| 8192 | settings Stick modifier changed to Mod(#) Mod%(# %) (from /stick mod #)Stick mod changed Mod(#) ModPercent (# %) (from stick inline -# or #%) Moveto distance mod changed to #. (from /moveto dist #) Option turned on (from /command set option on, or /command toggle option)Option turned off (from /command set option off, or /command toggle option)Option set to # (from /command set option #) |
| 16384 | file input / output Debug file created. Saved settings to C:\yourpath\MQ2MoveUtils.ini (from /command save)Loaded settings from C:\yourpath\MQ2MoveUtils.ini (from /command load) |
| 32768 | breakonwarp Stick pausing until target back in BreakDist range...Stick ending from target warping out of BreakDist range. |
| 65536 | breakonaggro BreakonAggro's: Aggro gained during /moveto, Halting command... |
| 131072 | breakonhit BreakonHit's: Aggro gained during /moveto, Halting command... |
| 262144 | breakonsummon WARNING Command ended from character summoned # distance in a pulse. WARNING Verify you are not being monitored and type /stick imsafe to allow command usage. |
| 524288 | breakongm WARNING Plugin halted from [GM] Name in zone. [GM] Name has left the zone or turned invisible. Use /stick imsafe to allow command usage. |
| 1048576 | breakongate Mob gating ended previous command. |
| 2097152 | stick always Stick awaiting next valid NPC target... |
| 4194304 | error messages (ERROR) /moveto or /circle command used with no parameter. (ERROR) Plugin was already paused. (ERROR) Plugin was not paused. (ERROR) /stick mod [#] supplied incorrectly. (ERROR) /moveto yloc [Y] was supplied incorrectly. (ERROR) /moveto xloc [X] was supplied incorrectly. (ERROR) SpawnID must be a positive numerical value. (ERROR) You cannot use yourself or your mount. (ERROR) You cannot stick hold to yourself. (ERROR) Incorrectly used /moveto dist [#] (ERROR) /makecamp [radius <dist>] was supplied incorrectly. (ERROR) You do not have an active camp. (ERROR) You cannot use this command with a player-camp active. (ERROR) You cannot use this command until you've established an altcamp location. (ERROR) Invalid player name and do not have a valid player target. (ERROR) You cannot makecamp yourself. (ERROR) Use /circle radius [#] to set radius.ERROR: Invalid 'option set' syntax ( option ) [on off number] ERROR: Not a valid command toggle ( option ). ERROR: Not a valid command set option ( option ). Error - Font must be between 1 and 10. ERROR: Invalid 'command set' parameter ( option )(ERROR) You cannot stick to yourself! You must specify something to stick to! (ERROR) /moveto loc [ `<Y>` `<X>` [z] ] was supplied incorrectly. (ERROR) /makecamp loc [ `<Y>` `<X>` ] was supplied incorrectly. (ERROR) Usage /circle loc [ `<Y>` `<X>` ] [other options] (ERROR) Invalid SpawnID and do not have a valid target. (ERROR) /makecamp [mindelay maxdelay] [#] was supplied incorrectly. |
| 8388608 | arc randomization Arcs Randomized! Max: # Min: # |
| 16777216 | pause / unpause PAUSED (from /command pause)RESUMED (from /command unpause) |

## See also

- [MQ2AdvPath](../mq2advpath/index.md)
- [MQ2Nav](../mq2nav/index.md)

## Top-Level Objects

## [Circle](tlo-circle.md)
{% include-markdown "projects/mq2moveutils/tlo-circle.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/tlo-circle.md') }}

## [MakeCamp](tlo-makecamp.md)
{% include-markdown "projects/mq2moveutils/tlo-makecamp.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/tlo-makecamp.md') }}

## [MoveTo](tlo-moveto.md)
{% include-markdown "projects/mq2moveutils/tlo-moveto.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/tlo-moveto.md') }}

## [MoveUtils](tlo-moveutils.md)
{% include-markdown "projects/mq2moveutils/tlo-moveutils.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/tlo-moveutils.md') }}

## [Stick](tlo-stick.md)
{% include-markdown "projects/mq2moveutils/tlo-stick.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/tlo-stick.md') }}

## DataTypes

## [circle](datatype-circle.md)
{% include-markdown "projects/mq2moveutils/datatype-circle.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/datatype-circle.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2moveutils/datatype-circle.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2moveutils/datatype-circle.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}

## [makecamp](datatype-makecamp.md)
{% include-markdown "projects/mq2moveutils/datatype-makecamp.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/datatype-makecamp.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2moveutils/datatype-makecamp.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2moveutils/datatype-makecamp.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}

## [moveto](datatype-moveto.md)
{% include-markdown "projects/mq2moveutils/datatype-moveto.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/datatype-moveto.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2moveutils/datatype-moveto.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2moveutils/datatype-moveto.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}

## [MoveUtils](datatype-moveutils.md)
{% include-markdown "projects/mq2moveutils/datatype-moveutils.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/datatype-moveutils.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2moveutils/datatype-moveutils.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2moveutils/datatype-moveutils.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}

## [stick](datatype-stick.md)
{% include-markdown "projects/mq2moveutils/datatype-stick.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2moveutils/datatype-stick.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2moveutils/datatype-stick.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2moveutils/datatype-stick.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}

