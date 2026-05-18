---
tags:
  - command
---

# /circle

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/circle [option]... 
/circle toggle <setting> 
/circle set <setting> {on|off}
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Autofaces character to run in a circle with a given radius
<!--cmd-desc-end-->

## Options

{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-options-start-->" 
  end="<!--cmd-options-end-->" 
%}

!!! tip "The following options are more specific to /circle."

| Option | Description |
|--------|-------------|
| `on [#]` | Begin circling using your current location as the center with default radius if optional # parameter supplied, use # as the radius size&gt;<br>*Must be first parameter.* |
| `off` | Stop circling. |
| `loc Y X` | Begin circling using the specified location as the center.<br>*Must be first parameter.* |
| `drunken` | Turn to complete the circle at random intervals. |
| `clockwise | cw ` | Circle in a clockwise direction (default). |
| `cw | counterclockwise | reverse` | Circle in a counter-clockwise direction. |
| `radius #` | Sets the default size of the circle radius.<br>For use with loc y x since on # would have to be first parameter. |
| `backward` | Run backwards instead of forwards. |

## Settings
{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-settings-start-->" 
  end="<!--cmd-settings-end-->" 
%}

!!! tip "The following settings are specific to /circle."

| Setting | Valid Input | Description |
|---------|-------------|-------------|
| **/circle [toggle \| set] loose \| truehead [on \| off]** | | Change the heading type for currently active circle to this type of heading.<br>If WinEQ is enabled, truehead will fail to switch.<br>Once current command ends, heading type will return to previous. |


## Examples

<code>/circle on 100</code>  
Does a circle with 100 radius.