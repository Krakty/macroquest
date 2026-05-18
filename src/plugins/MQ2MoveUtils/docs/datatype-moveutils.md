---
tags:
  - datatype
---
# `MoveUtils`

<!--dt-desc-start-->
Members of this datatype relate to plugin settings and generic information and can be accessed via the ${MoveUtils} TLO.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='bool', name='Aggro') }}

:   Displays true if you are facing your target and your target is facing you.

### {{ renderMember(type='string', name='Command') }}

:   Displays the currently active command. MAKECAMP returns if a camp is setup but no other command is currently in use.

### {{ renderMember(type='float', name='DistStuck') }}

:   Displays the amount of distance needed to have moved (compared against pulse average) or else considered stuck by stucklogic.

### {{ renderMember(type='bool', name='FullVerbosity') }}

:   Displays true if fullverbosity is enabled.

### {{ renderMember(type='bool', name='GM') }}

:   Displays true if BreakonGM fired.

### {{ renderMember(type='bool', name='MovePause') }}

:   Displays true if mpause (PauseKB) is enabled.

### {{ renderMember(type='int', name='PauseMaxDelay') }}

:   Displays the max delay for mousepause and mpause to resume command in ms.

### {{ renderMember(type='int', name='PauseMinDelay') }}

:   Displays the min delay for mousepause and mpause to resume command in ms.

### {{ renderMember(type='int', name='PulseCheck') }}

:   Displays the number of pulses used to average movement rate for stucklogic.

### {{ renderMember(type='int', name='PulseUnstuck') }}

:   Displays the number of pulses successfully moved forward after being stuck to be considered unstuck.

### {{ renderMember(type='bool', name='Stuck') }}

:   Displays true if plugin stucklogic has determined you are currently stuck.

### {{ renderMember(type='bool', name='StuckLogic') }}

:   Displays true if stucklogic is enabled.

### {{ renderMember(type='bool', name='Summoned') }}

:   Displays true if BreakonSummon is enabled and has fired due to your character being summoned beyond breakonsummon distance.

### {{ renderMember(type='bool', name='TotalSilence') }}

:   Displays true if totalsilence is enabled.

### {{ renderMember(type='bool', name='TryToJump') }}

:   Displays true if stucklogic trytojump is enabled.

### {{ renderMember(type='bool', name='Verbosity') }}

:   Displays true if verbosity is enabled.

### {{ renderMember(type='float', name='Version') }}

:   Displays the version number of the plugin.

### {{ renderMember(type='string', name='To string') }}

:   Same as Command

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
