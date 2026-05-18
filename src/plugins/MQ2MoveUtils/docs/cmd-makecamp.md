---
tags:
  - command
---

# /makecamp

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/makecamp [option]... 
/makecamp toggle <setting> 
/makecamp set <setting> {on|off}
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
The makecamp command will create a camp spot for you to return to after combat, or to establish boundries for your character to prevent them from moving beyond a certain radius.

Using makecamp with no parameters will set up a camp at your current location, using default values.
<!--cmd-desc-end-->

## Options

{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-options-start-->" 
  end="<!--cmd-options-end-->" 
%}

!!! tip

    The following options are more specific to /makecamp.

| Option | Description |
|--------|-------------|
| `[on [#] | off]` | On: Set up a camp at current location with default values.<br>If optional # parameter supplied, use # as camp radius size.<br>**Must be first parameter.**<br><br>Off: disable current makecamp |
| `loc <Y X>` | Set up a camp at the specified location.<br>**Must be first parameter.** |
| `player [name]` | Set up a dynamic camp based on a certain pc name if in zone, or targeted pc if optional name not supplied. |
| `leash [#]` | Toggles leashing to camp radius so character cannot leave boundary. <br><br>If optional number is supplied, sets how far beyond camp radius you can move before leashing (LeashLength). |
| `radius <#>` | Sets the radius of the camp size.<br>Does not turn camp on if supplied on its own. |
| `mindelay # | maxdelay #` | Sets the delay time before auto-returning to camp. |
| `return` | Forces a return to the camp radius immediately. |
| `altreturn` | Forces a return to the camp spot you had before your current one, or a camp that is now turned off. |

## Settings

{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-settings-start-->" 
  end="<!--cmd-settings-end-->" 
%}

## Examples

| Example | Description |
|---------|-------------|
| **/makecamp on 1** | Makes a camp with a 1 return radius at loc you are standing at. Make sure you have no target or it will not return to camp. |
| **/makecamp radius 50** | Your character will move back to camp spot if it gets 50 distance away from camp and has no target. |