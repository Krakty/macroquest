---
tags:
  - datatype
---
# `circle`

<!--dt-desc-start-->
Members of this datatype relate to the '/circle' command and can be accessed via the ${Circle} TLO
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='string', name='Status') }}

:   Returns off / paused / on. On if circling.

### {{ renderMember(type='float', name='CircleY') }}

:   Location of circle center Y.

### {{ renderMember(type='float', name='CircleX') }}

:   Location of circle center X.

### {{ renderMember(type='bool', name='Drunken') }}

:   Displays true if drunken.

### {{ renderMember(type='string', name='Rotation') }}

:   Displays CCW if reverse circling.

### {{ renderMember(type='string', name='Direction') }}

:   Movement direction of current circle.

### {{ renderMember(type='bool', name='Clockwise') }}

:   Displays false if reverse circling.

### {{ renderMember(type='bool', name='Backwards') }}

:   Displays true if movement direction backwards.

### {{ renderMember(type='float', name='Radius') }}

:   Radius size of circle.

### {{ renderMember(type='string', name='To string') }}

:   Same as Status

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
