---
tags:
  - datatype
---
# `makecamp`

<!--dt-desc-start-->
Members of this datatype relate to the '/makecamp' command and can be accessed via the ${MakeCamp} TLO.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='string', name='Status') }}

:   Displays status of MakeCamp command. AltCamp returns off.

### {{ renderMember(type='bool', name='Leash') }}

:   Displays true if leash is enabled.

### {{ renderMember(type='float', name='AltAnchorX') }}

:   Location of current altcamp X anchor.

### {{ renderMember(type='float', name='AltAnchorY') }}

:   Location of current altcamp Y anchor.

### {{ renderMember(type='float', name='AltCampDist') }}

:   Distance to altcamp anchor from your current location. Returns 0.00 if altcamp not established.

### {{ renderMember(type='float', name='AltRadius') }}

:   Size of altcamp radius.

### {{ renderMember(type='float', name='AnchorX') }}

:   Location of current camp X anchor.

### {{ renderMember(type='float', name='AnchorY') }}

:   Location of current camp Y anchor.

### {{ renderMember(type='float', name='Bearing') }}

:   Bearing (heading) of camp scattering.

### {{ renderMember(type='float', name='CampDist') }}

:   Distance to camp anchor from your current location. Returns 0.00 if camp is disabled.

### {{ renderMember(type='float', name='CampRadius') }}

:   Size of camp radius.

### {{ renderMember(type='float', name='LeashLength') }}

:   Size of Leash Length. (Greater than or equal to CampRadius)

### {{ renderMember(type='int', name='MaxDelay') }}

:   Displays the max delay for auto-returning to camp in ms. (125 or more greater than MinDelay)

### {{ renderMember(type='int', name='MinDelay') }}

:   Displays the min delay for auto-returning to camp in ms. (125 or greater)

### {{ renderMember(type='bool', name='ReturnHaveTarget') }}

:   Displays true if ReturnHaveTarget is enabled.

### {{ renderMember(type='bool', name='Returning') }}

:   Displays true if /makecamp return issued.

### {{ renderMember(type='bool', name='ReturnNoAggro') }}

:   Displays true if ReturnNoAggro is enabled.

### {{ renderMember(type='bool', name='ReturnNotLooting') }}

:   Displays true if ReturnNotLooting is enabled.

### {{ renderMember(type='float', name='ScatDist') }}

:   Distance from anchor to perform scatter.

### {{ renderMember(type='float', name='ScatSize') }}

:   Size of scattering radius.

### {{ renderMember(type='bool', name='Scatter') }}

:   Displays true if camp scattering enabled.

### {{ renderMember(type='string', name='To string') }}

:   Same as Status.

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
