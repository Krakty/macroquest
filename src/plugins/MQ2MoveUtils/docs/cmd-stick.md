---
tags:
  - command
---

# /stick

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/stick [option]...
/stick toggle <setting>
/stick set <setting> {on|off}
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
/follow-like command, works for any pc/npc. Default distance is melee range. `/stick` with no parameters will stick you to your current target, using max melee range.
<!--cmd-desc-end-->

## Options

<!--cmd-options-start-->
| Option | Description |
|--------|-------------|
| `help [settings]` | Displays generic help information, and help for the command used. The settings optional parameter displays help information for all plugin 'set' and 'toggle' commands. **NOTE:** this command won't show anything if verbosity is off. `/stick set verbosity on` to see help settings. |
| `debug` | Dumps the current values of all plugin variables to a debug INI file. |
| `status [all]` | ChatWnd output for the status of the issuing command. <br>The all optional parameter displays status output for all aspects of the plugin. |
| `pause [lock]` | Pauses all aspects of the plugin.<br>The lock optional parameter prevents plugin from automatically unpausing under any circumstance other than user issuing unpause.<br>***Note: This does not toggle.*** |
| `unpause` | Resumes all aspects of the plugin.<br>**Note: This does not toggle.** |
| `save / load` | Saves or loads your current configuration settings using MQ2MoveUtils.ini. |
| `imsafe` | BreakOnSummon and BreakOnGM have built-in protection disabling the ability to re-issue commands when triggered.<br>This prevents macros from continuing to issue commands in a possibly unsafe situation.<br>The imsafe parameter allows command usage to resume. |
| `min` | Minimizes custom user window similar to /mqmin. |
| `clear` | Clears custom user window buffer similar to /mqclear. |
| `verbflags` | Outputs current verbosity flags setting (this one displays even if totalsilence is enabled). |
<!--cmd-options-end-->

!!! tip

    The following options are more specific to /stick.

| Option | Description |
|--------|-------------|
| `on / off` | Deprecated. Turns stick on and off with default values. On is a nearly-useless parameter and only included to support older macros or stickcmd=on in MQ2Melee to prevent MQ2Melee from doing anything undesired. If you use /stick on in your macro, stop it. |
| `id [#]` | Sticks to the given spawn id.<br>Uses id of your current target if no spawn id is given.<br>This allows you to continue sticking when your target changes, e.g. casting a heal on someone. |
| `# | #%` | Stick at the specified distance or percentage. |
| `-#` | Reduce current stick distance modifier by #. |
| `moveback` | Stick will back up to the &lt;dist&gt; value if the target gets closer, e.g. many targets in the rear pushing target too close to the tank. |
| `loose` | Stick using turn increments instead of instant heading adjustment. |
| `truehead` | Stick using actual keypress heading adjustments.<br>Does not work if wineq option is enabled! |
| `healer` | Healer sticking does not perform face adjustments to look at the target while in stick range.<br>This is good for keeping a healer close &amp; sticking to another group member without having it turn to face the other character constantly as it moves.<br>Does not work with any strafe-style sticks (pin front !front behind behindonce snaproll). |
| `uw | underwater` | Face angle will look up/down at the stick target. |
| `hold` | Stick to the current target even if your target changes. |
| `behind` | Stick to the rear of the target unless you are on HoTT.<br>Will spin in circles if you do not have HoTT and gain aggro (to prevent, use DelayStrafe!). |
| `behindonce` | Stick behind the target when first moving into position, only using &lt;dist&gt; enforcement after. |
| `!front` | Stick to target anywhere but the frontal arc, same considerations as behind apply (use DelayStrafe!). |
| `pin` | Stick to the side of the target, same consideration as behind apply (use DelayStrafe!). |
| `front` | Stick to the front arc of the target.<br>If you have HoTT and lose aggro you will not spin.<br>This will not work by default without HoTT. |
| `(ANY STICK VALUES) always` | When current target is lost, will wait and then resume sticking using supplied values upon next NPC targeted.<br>Does not work with stick hold or stick id. |
| `snaproll left | right | face | rear` | Runs in a straight line behind your target then turns to face.<br>Left/Right/Front of target if optional parameter direction supplied.<br>Rear is default. |
| `mod # | -#` | Modify stick distance by the supplied amount (does not turn stick on). |

## Settings
<!--cmd-settings-start-->
<!-- these are global so please keep command-specific info elsewhere -->
!!! note

    The following parameters may be used to change the plugin's configuration options, which are defined on the [MQ2MoveUtils settings](index.md#ini-options) page. You'll need to read both pages for a full understanding of a parameter.

### Parameters

Use any of the four primary commands (`/stick`, `/moveto`, `/circle`, or `/makecamp`). Even if the parameter you're changing only affects a single command, you can still change it with any of the four commands.

```text
/<command> toggle <parameter>
/<command> set    <parameter> [value]
```

| Parameter | Description |
|-----------|-------------|
| **mpause \| mousepause** | Pause current command if [ keyboard \| mouse ] movement.<br>Resumes after a random amount of delay set with pausemindelay and pausemaxdelay below.<br>***Note: You may not have a pause and corresponding break on at the same time (e.g. no mpause and breakonkb at the same time).***<br>You may have opposing options different though (e.g. mousepause on and breakonkb on). |
| **breakonkb \| breakonmouse** | Break current command if [ keyboard \| mouse ] movement.<br>***Note: You may not have a pause and corresponding break on at the same time (e.g. no mpause and breakonkb at the same time).<br>You may have opposing options different though (e.g. mousepause on and breakonkb on). |
| **autosave** | Automatically save settings to INI file when a toggle or set command is issued. |
| **savebychar** | Save [server.Yourcharacter] section of INI file for individual character settings. |
| **feign** | Enable Feign Death support, which waits for you to stand up manually before moving. |
| **lockpause** | Plugin will never automatically resume from pause until user issues an unpause. |
| **autopause** | Pause movement if casting spells (non-bard), stunned, rooted, sitting, FD, or self targeted (non-hold). |
| **autopauseoutput** | If enabled, will display ChatWnd output when autopause is halting movement.<br>***Note: This bypasses totalsilence and must be configured individually.*** |
| **stucklogic** | If enabled, stucklogic automatically attempts to get unstuck if running into walls and large objects. |
| **trytojump** | If enabled, stucklogic also tries to jump to help get unstuck. |
| **turnhalf** | If enabled, stucklogic will reset heading and turn the other direction if it has rotated halfway without success. |
| **verbosity** | ChatWnd output for basic command information messages. |
| **fullverbosity** | ChatWnd output for more detailed information messages and output for more actions. |
| **totalsilence** | Silences most ChatWnd output except for critical information or user-requested messages. |
| **totalverbosity** | Enable display of every ChatWnd message in the plugin. |
| **hidehelp** | If enabled, the help output will not be displayed upon command failure (e.g. invalid parameters). |
| **window** | If enabled, MoveUtils will output any messages to a user-placed custom UI window dedicated to the plugin. |
| **wineq** | By default, MoveUtils uses actual keypresses to control movement (regardless of heading settings).<br>WinEQ2 has a bug where background sessions can have their alt keys and certain mouse buttons held down, causing movement to run in weird directions.<br>For Lavishsoft users who have not switched over to Innerspace, enabling wineq setting will use old-style emulated movement via ExecuteCmd(). |
| **breakontarget** | Break from stick if target changes (default behavior is switch over to sticking to new target). |
| **breakonsummon** | Halts current command and disables ability to use any commands if summoned beyond summondist.<br>***Note: Once this fires, you must use the imsafe parameter to unlock the plugin.*** |
| **breakongm** | Halts current command and disables ability to use any commands if visible GM enters the zone.<br>***Note: Once this fires, you must use the imsafe parameter to unlock the plugin.*** |
| **breakonwarp** | Breaks from stick if target warps out of breakdist range (user set, see below).<br>***Note: This does not limit your initial stick range. You may /stick from across the zone.***<br>This only triggers if the target increases distance from you and is beyond breakdist. |
| **pauseonwarp** | Pauses stick if target warps out of breakdist range until they are back in range (user set, see below).<br>***Note: This does not limit your initial stick range. You may /stick from across the zone.***<br>This only triggers if the target increases distance from you and is beyond breakdist. |
| **breakongate** | Breaks from stick if "target Gates" message occurs.<br>***Note: if using stick id or stick hold, it will break based on the held target name.*** |
| **breakonaggro** | Breaks from moveto command if you are aggro to an NPC.<br>***Note: This checks the player window for the crossed swords indicator.*** |
| **alwaysdrunk** | Use drunken by default when circling. |
| **alwaysbackwards** | Run backwards by default when circling. |
| **alwaysccw** | Circle in a counter-clockwise direction by default. |
| **nohottfront** | Allow for stick front to spin to front of the mob without Health of Target's Target Leader AA.<br>***Note: By default stick front will not stay stuck to the front unless you are on the HoTT window.***<br>If you toggle this setting it will ignore that requirement, causing you to spin in circles if you lose aggro. It is recommended that you use DelayStrafe. |
| **returnnoaggro** | Makecamp will auto-return to camp only if not aggro (checks crossed swords indicator). |
| **returnnotlooting** | Makecamp will not auto-return to camp if character has an open loot corpse window. |
| **returnhavetarget** | Makecamp will auto-return to camp even if you have a target.<br>***Note: By default, makecamp does not auto-return if you have any target.*** |
| **realtimeplayer** | Makecamp player will get realtime updates on player location, allowing for dynamic returns to players.<br>By default, makecamp player will return to a players last location when return begins and not get a new update until the return is complete.<br>The default behavior is a sort of ghetto MQ2AdvPath whereas enabling realtimeplayer will work more like an autofollow. |
| **leash** | If enabled, leash prevents moving beyond leashlength (user set value). |
| **usewalk** | If enabled, plugin will switch to walking when closing in on moveto destination or camp return. |
| **strafewalk** | If enabled, plugin will switch to walking when within strafe range for all stick commands. |
| **randomize** | If enabled, stick behind and !front will use random arc values to position. |
| **delaystrafe** | If enabled, strafe-based movement (stick front, !front, behind, pin) will use a delay before moving.<br>***Note: This helps prevent endless circling when aggro is gained, or spinning when mobs quick-turn to cast spells.***<br>Circling is one of the biggest signs that a player is using MoveUtils so it is recommended you ALWAYS leave this enabled. |
| **autoUW** | If enabled, stick and moveto will use the uw parameter whenever underwater (look up and down at target). |
| **usefleeing** | If enabled, stick front will not attempt to position in the front of a target that is fleeing. |
| **usescatter** | If enabled, camp returns will use scattered return locations instead of default behavior.<br>***Note: Default behavior attempts to get back within camp radius.***<br>See scatter numerical settings for more information. |

The following option is unique:

| Setting | Description |
|---------|-------------|
| **set heading true \| loose \| fast** | Changes plugin heading adjustments to use the specified type:<br>**true:** actual keypressing (does not work with wineq=on).<br>**loose:** simulated incremental turning.<br>**fast:** instantly set heading. |

The following `set` commands require a numeric value, and once again, can be used from any of the four main plugin commands.

| Setting | Valid Input | Description |
|---------|-------------|-------------|
| **set pulsecheck #** | 1 or higher | Number of pulses used to calculate average movement distance in stucklogic. |
| **set pulseunstuck #** | 1 or higher | Number of pulses successfully moved forward before considered unstuck. |
| **set diststuck #.##** | 0.01 or higher | Minimum distance needed to move or else considered stuck (compares against pulse average). |
| **set campmindelay #** | 125 or higher | Minimum delay before auto-returning to camp (in ms). |
| **set campmaxdelay #** | 125 more than campmindelay | Maximum delay before auto-returning to camp (in ms). |
| **set pausemindelay #** | 125 or higher | Minimum delay before resuming from mpause/mousepause (in ms). |
| **set pausemaxdelay #** | 125 more than pausemindelay | Maximum delay before resuming from mpause/mousepause (in ms). |
| **set strafemindelay #** | 125 or higher | Minimum delay before stick will strafe to move when delaystrafe is enabled (in ms).<br>***Note: higher values are better. default of 1500 (1.5s) is recommended.*** |
| **set strafemaxdelay #** | 125 higher than strafemindelay | Maximum delay before stick will strafe to move when delaystrafe is enabled (in ms).<br>***Note: higher values are better. default of 3000 (3s) is recommended.*** |
| **set ydist #.##** | 1.0 or higher | Acceptable distance to have "arrived" for precisey moveto. |
| **set xdist #.##** | 1.0 or higher | Acceptable distance to have "arrived" for precisex moveto. |
| **set snapdist #.##** | 1.0 or higher | Default distance to run past target before turning to snaproll. |
| **set summondist #.##** | 2.0 or higher | Distance character must be summoned in a single pulse for BreakOnSummon to fire. |
| **set turnrate #.#** | 1.0 to 100.0 | Rate at which loose heading turns. |
| **set !frontarc #.#** | 1.1 to 260.0 | Angular distance arc for stick !front. |
| **set behindarc #.#** | 1.1 to 260.0 | Angular distance arc for stick behind. |
| **set breakdist #.##** | 1.0 or higher | Distance to check for breakonwarp. |
| **set campradius #.##** | 5.0 or higher | Default camp radius and radius for active camp. |
| **set circleradius #.##** | 5.0 or higher | Default circle radius. |
| **set leashlength #.#** | greater or equal to camp radius | Default leash length and length for active leash. |
| **set bearing #.#** | any | Bearing (direction from center) used for scatter camp. |
| **set scatsize #.##** | 1.0 or higher | Radius size for scattering. |
| **set scatdist #.##** | 1.0 or higher | Distance from center of camp to scatter at. |
| **set allowmove #.##** | 10.1 or higher | Loose or True heading allow forward movement when reach this angular distance.<br>This is "anti-orbit" code to stop circling near close-range destinations. |
| **set font #** | 1 to 10 | Custom user window font size. |
| **set verbflags #** | see verbosity section on mq2moveutils settings. | Current plugin verbosity flags. |

<!--cmd-settings-end-->

!!! tip

    The following settings are specific to /stick.

| Setting | Valid Input | Description |
|---------|-------------|-------------|
| **/stick set backupdist #.##** | 1.0 or higher | Range that stick will walk backwards instead of turning to face target, if useback enabled. |
| **/stick [toggle \| set] alwaysUW [on \| off]** | | If enabled, stick will always adjust looking angle as if uw parameter was typed inline.<br>***Note: this is not the same as autoUW, which only enables uw when actually underwater.*** |
| **/stick [toggle \| set] breakonhit [on \| off]** | | Breaks from stick command if you are attacked by an NPC.<br>***Note: This parses chat for hits and misses. If you use the number only hitsmode then it will only parse for misses as parsing every line for only numbers is too much overhead. Consider switching to Normal or Abbreviated hitsmode.*** |
| **/stick [toggle \| set] useback [on \| off]** | | If enabled, stick will walk backwards to position itself when close to a target instead of turning to face it.<br>***Note: This requires loose or truehead style heading adjustments, and does not work with fast heading.*** |
| **/stick [toggle \| set] loose \| truehead [on \| off]** | | Change the heading type for currently active stick to this type of heading.<br>If WinEQ is enabled, truehead will fail to switch.<br>Once current command ends, heading type will return to previous. |

## Examples

| Command | Description |
|---------|-------------|
| `/stick 10` | Sticks to target to keep you within 10 radius. |
| `/stick status` | Shows current settings of your stick. |
| `/stick behind 10` | Sticks you behind target at 10 radius. |
| `/stick hold` | Sticks to current target even if target changes. |
| `/stick loose` | Makes your stick look more human like. |
| `/stick underwater` | Makes you look vertically at your target. |
| `/stick 120 healer` | Does not do face adjustments, keeps in 120 range. |

## See also

- [follow](../everquest/commands/cmd-follow.md)
- [afollow](../mq2advpath/cmd-afollow.md)
- [navigate](../mq2nav/cmd-navigate.md)