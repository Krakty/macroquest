---
tags:
  - command
---

# /moveto

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/moveto [option]... 
/moveto toggle <setting> 
/moveto set <setting> {on|off}
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
The moveto command will move you from your current location to a specific location or target. You can combine any number of these parameters together inline to enable multiple options for the moveto.
<!--cmd-desc-end-->

## Options

{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-options-start-->" 
  end="<!--cmd-options-end-->" 
%}

!!! tip

    The following options are more specific to /moveto.

| Option | Description |
|--------|-------------|
| `loc Y X [Z]` | Moves to the specified location.<br>Z is optional.<br>***Must be the first parameter.*** |
| `yloc Y | xloc X` | Beeline to the Y or X supplied.<br>Different from precisey/x in that this only considers a single axis.<br>***Must be the first parameter.*** |
| `id [#]` | Moves to the supplied spawn id, or your current target if no id is given. |
| `off` | Stop moving to the current target/location. |
| `loose` | Moveto using more human-like heading adjustments. |
| `truehead` | Moveto using actual keypress heading adjustments. |
| `(id | loc Y X [Z]) precisey | precisex` | Moves to loc stopping when within x or y arrival dist values instead of both.<br>Works with either id or loc. |
| `uw | underwater` | Look angle up and down at destination. |
| `dist #` | Sets value for how close to actual location is considered arrival.<br>Does not turn moveto on.<br>Permanently changes the value in the .ini. |
| `[id spawnid | loc y x [z]] mdist # [id]` | Sets value for how close to actual location is considered arrival.<br>Allowed inline BEFORE id or AFTER loc y x &#91;z&#93; or id &#91;spawn id&#93; parameter.<br>Permanently changes the value in the .ini. |

## Settings

{% 
  include-markdown "projects/mq2moveutils/cmd-stick.md" 
  start="<!--cmd-settings-start-->" 
  end="<!--cmd-settings-end-->" 
%}

!!! tip

    The following settings are specific to /moveto.

| Setting | Valid Input | Description |
|---------|-------------|-------------|
| **/moveto set backupdist #.##** | 1.0 or higher | Range that moveto will walk backwards instead of turning to face destination, if useback enabled. |
| **/moveto set dist #.##** | 1.0 or higher | Acceptable distance to have "arrived" for standard moveto and camp returns. |
| **/moveto [toggle \| set] alwaysUW [on \| off]** | | If enabled, moveto will always adjust looking angle as if uw parameter was typed inline.<br>***Note: this is not the same as autoUW, which only enables uw when actually underwater.*** |
| **/moveto [toggle \| set] breakonhit [on \| off]** | | Breaks from moveto command if you are attacked by an NPC.<br>***Note: This parses chat for hits and misses. If you use the number only hitsmode then it will only parse for misses as parsing every line for only numbers is too high overhead. Consider switching to Normal or Abbreviated hitsmode.*** |
| **/moveto [toggle \| set] useback [on \| off]** | | If enabled, moveto will walk backwards to position itself when close to a destination instead of turning to face it.<br>***Note: This requires loose or truehead style heading adjustments, and does not work with fast heading.***<br>This includes automatic and user-forced camp returns. |
| **/moveto [toggle \| set] loose \| truehead [on \| off]** | | Change the heading type for currently active moveto to this type of heading.<br>If WinEQ is enabled, truehead will fail to switch.<br>Once current command ends, heading type will return to previous. |


## Examples

| Command | Description |
|---------|-------------|
| **/moveto loc 2816 -6** | Moves your character to loc 2816 -6. |
| **/moveto id ${Target.ID}** | Moves your character to Target. |
