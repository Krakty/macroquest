---
tags:
  - datatype
---
# `moveto`

<!--dt-desc-start-->
Members of this datatype relate to the '/moveto' command and can be accessed via the ${MoveTo} TLO.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='float', name='ArrivalDist') }}

:   Acceptable arrival distance.

### {{ renderMember(type='float', name='ArrivalDistX') }}

:   Acceptable arrival distance for precisex.

### {{ renderMember(type='float', name='ArrivalDistY') }}

:   Acceptable arrival distance for precisey.

### {{ renderMember(type='bool', name='Broken') }}

:   Returns true if BreakonAggro or BreakonHit event have halted moveto prematurely.

### {{ renderMember(type='bool', name='CampStopped') }}

:   Displays true if within moveto distance of makecamp Y X location.

### {{ renderMember(type='bool', name='Moving') }}

:   Displays true if moveto or camp return is active.

### {{ renderMember(type='bool', name='Stopped') }}

:   Displays true if the last moveto command completed successfully.

### {{ renderMember(type='bool', name='UseWalk') }}

:   Returns true if UseWalk is enabled.

### {{ renderMember(type='bool', name='To string') }}

:   Displays on if a moveto command is active.

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
<!--dt-linkrefs-end-->
