---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2twist.204"
support_link: "https://www.redguides.com/community/threads/mq2twist.66897/"
repository: "https://github.com/RedGuides/MQ2Twist"
config: "<server>_<character>.ini"
authors: "Koad, CyberTech, Cr4zyb4rd, Pheph, Simkin, MinosDis, dewey2461, htw, gSe7eN, eqmule, plure"
tagline: "Assists with the twisting of bard songs"
---

# MQ2Twist

<!--desc-start-->
Made to assist in the twisting of bard songs. It takes care of missed notes and other simple issues, and can also utilize items in a twist.
<!--desc-end-->

## Commands

<a href="cmd-sing/">
{% 
  include-markdown "projects/mq2twist/cmd-sing.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2twist/cmd-sing.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2twist/cmd-sing.md') }}

<a href="cmd-stoptwist/">
{% 
  include-markdown "projects/mq2twist/cmd-stoptwist.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2twist/cmd-stoptwist.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2twist/cmd-stoptwist.md') }}

<a href="cmd-twist/">
{% 
  include-markdown "projects/mq2twist/cmd-twist.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2twist/cmd-twist.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2twist/cmd-twist.md') }}

## Settings

```ini
[MQ2Twist] 
;Delay: time between twists (in 10ths of a second), adjust for your system/connection. Minimum 30 (standard gem refresh).
Delay=33 
;Adjust: Number of ticks before recasting during a long song (a long song is greater than 3 ticks in length). e.g. If a song lasts 10 ticks, adjust=1 will recast at the 8 tick mark, rather than 9 as normal.
Adjust=1
;Quiet: song listing and start/stop messages for one-shot twists 
Quiet=0
;Recast: spells with a recast, additional delay.
Recast=0
;Fife of Battle (just an example)
[Click_21]
;Casting Time, -1 to use the normal song delay 
CastTime=30
;How often to recast, 0 to twist normally. 
ReCastTime=0
;Name of item
Name=Fife of Battle
;Slot name
Slot=neck
;girdle of living thorns (current belt will be swapped out)
[Click_22]
CastTime=0
ReCastTime=11600
Name=Girdle of Living Thorns
Slot=waist
;nature's melody
[Click_23]
CastTime=-1
ReCastTime=135
Name=DISABLED
Slot=mainhand
;Shadowsong cloak
[Click_24]
CastTime=30
ReCastTime=350
Name=Shadowsong Cloak
Slot=DISABLED
```

Click_## currently goes from Click_21 to Click_40.

A note on items: If both a Name and [Slot](../macroquest/reference/general/slot-names.md) are defined for an item, the plugin will attempt to swap the item into that slot using the [/exchange](../mq2exchange/cmd-exchange.md) command. After casting, it will replace the original item. If only one is specified but not the other, "/itemnotify Slot rightmouseup" is used to perform item clicks.

## Examples

Sing gem 1 forever
:   `/twist 1`

Twist gems 1,2, and 3 forever
:   `/twist 1 2 3`

Twist gems 1,2,3, and clicky 23, forever
:   `/twist 1 2 3 23`

Sing gem 4 forever, until another singing-related /twist command is given
:   `/twist hold 4`

Same as above
:   `/sing 4`

Set Click_26 to CastTime=32, ReCastTime=120, Name="Cassindra's Chorus of Clarity", Slot=AA ; and save to INI
:   `/twist set 26 32 120 "Cassindra's Chorus of Clarity" AA`

## See also

- [MQ2Medley](../mq2medley/index.md)

## Top-Level Objects

## [Twist](tlo-twist.md)
{% include-markdown "projects/mq2twist/tlo-twist.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2twist/tlo-twist.md') }}

## DataTypes

## [twist](datatype-twist.md)
{% include-markdown "projects/mq2twist/datatype-twist.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2twist/datatype-twist.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2twist/datatype-twist.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2twist/datatype-twist.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}
