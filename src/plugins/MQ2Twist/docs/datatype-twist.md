---
tags:
  - datatype
---
# `twist`

<!--dt-desc-start-->
Returns information about the current twist queue
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='bool', name='Twisting') }}

:   Returns TRUE if currently twisting, FALSE if not and NULL if plugin not loaded.

### {{ renderMember(type='int', name='Current') }}

:   Returns the current gem being sung, -1 for item or 0 if not twisting

### {{ renderMember(type='int', name='Next') }}

:   Returns the next gem to be sung, -1 for item or 0 if not twisting

### {{ renderMember(type='string', name='List') }}

:   Returns the twist sequence in a format suitable for /twist.

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
