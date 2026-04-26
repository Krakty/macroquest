---
tags:
  - datatype
---
# `stick`

<!--dt-desc-start-->
Members of this datatype relate to the '/stick' command and can be accessed via the ${Stick} TLO.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='bool', name='Active') }}

:   Displays true if any form of stick is active.

### {{ renderMember(type='bool', name='Always') }}

:   Returns true if /stick always is active.

### {{ renderMember(type='bool', name='Behind') }}

:   Displays true if currently behind target (regardless of /stick behind), false if outside of stick dist or not behind.

### {{ renderMember(type='bool', name='Broken') }}

:   Returns true if BreakonHit event has halted stick prematurely.

### {{ renderMember(type='float', name='Distance') }}

:   Current distance used by stick.

### {{ renderMember(type='float', name='DistMod') }}

:   Current stickdist modifier.

### {{ renderMember(type='float', name='DistModPercent') }}

:   Current stickdist percent modifier.

### {{ renderMember(type='bool', name='Loose') }}

:   Displays true if loose sticking is enabled.

### {{ renderMember(type='bool', name='MoveBack') }}

:   Displays true if moveback is active.

### {{ renderMember(type='bool', name='MoveBehind') }}

:   Displays true if stick behind is active.

### {{ renderMember(type='bool', name='Paused') }}

:   Displays true if plugin is paused.

### {{ renderMember(type='bool', name='Pin') }}

:   Displays true if stick pin is active.

### {{ renderMember(type='string', name='Status') }}

:   Displays on if any form of stick is active.

### {{ renderMember(type='int', name='StickTarget') }}

:   Returns spawnid of stick target if stick id/hold used, else spawnid of current target, 0 if no target and id/hold not used.

### {{ renderMember(type='string', name='StickTargetName') }}

:   Returns DisplayedName of stick target if stick id/hold used, else current target or None if no target and hold/id not used.

### {{ renderMember(type='bool', name='Stopped') }}

:   Displays true if stick is within stick distance.

### {{ renderMember(type='string', name='To string') }}

:   Same as Status

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
