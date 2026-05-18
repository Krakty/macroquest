// MQ2Twist.cpp - Bard song twisting plugin for MacroQuest2
//
//    koad 03-24-04 Original plugin (http://macroquest.sourceforge.net/phpBB2/viewtopic.php?t=5962&start=2)
//    CyberTech 03-31-04 w/ code/ideas from Falco72 & Space-boy
//    Cr4zyb4rd 08-19-04 taking over janitorial duties
//    Pheph 08-24-04 cleaning up use of MQ2Data
//    Simkin 12-17-07 Updated for Secrets of Faydwer 10 Songs
//    MinosDis 05-01-08 Updated to fix /twist once
//    dewey2461 09-19-09 Updated to let you twist AA abilities and define clicky/AA songs from inside the game
//    htw 09-22-09+ See changes below
//    gSe7eN 04-03-12 Fixed Show to dShow for March 31st build
//    eqmule Sep 09 2016 Fix for buffer overflow
/*
   MQ2Twist

      Usage:
         /twist # # # # # - Twists in the order given.
            Valid options are 1 thru NUM_SPELL_GEMS (EQData.h) for song gems, and 21  thru 29 for item clicks.
            These may be mixed in any order, and repeats are allowable. Up to 10 may be
            specified.
            If a song is specified with a duration longer than standard (ie, selos)
            that song will be twisted based on it's duration.  For example, riz+mana+selos
            would be a 2 song twist with selos pulsed every 2.5 min.
         /twist once # # # # # - Twists in the order given, then reverts to original twist
         /twist hold <gem #> - Pause twisting and sing only the specified song
         /sing <gem#> - alias for /twist hold
         /twist stop/end/off - stop twisting, does not clear the twist queue
         /stoptwist - alias for above
         /twist or /twist start - Resume the twist after using /twist hold or /twist stop
         /twist reset - Reset timers for item clicks and long duration songs
         /twist delay # - 10ths of a second, minimum of 30, default 33
         /twist adjust # - in ticks, how early to recast long duration songs
         /twist reload - reload the INI file to update item clicks
         /twist slots - List the slots/items defined in the INI and their #'s
         /twist quiet - Toggles songs listing and start/stop messages for one-shot twists

      ----------------------------
      Item Click Method:
       MQ2Twist uses /nomodkey /itemnotify slotname rightmouseup to perform item clicks.

       The INI file allows you to specify items by name (with name=itemname), or by
       inventory slot (with slot=slotname).  If both a name and slot are defined for an
       item, the plugin will attempt to swap the item into that slot (via the /exchange
       command) and replace the original item when casting is complete.

       The example INI file below contains examples of the types of usage.

      ----------------------------
      Examples:
         /twist 1
            Sing gem 1 forever
         /twist 1 2 3
            Twist gems 1,2, and 3 forever
         /twist 1 2 3 10
            Twist gems 1,2,3, and clicky 10, forever
         /twist hold 4 or /sing 4
            Sing gem 4 until another singing-related /twist command is given
         /twist set 16 32 120 "Cassindra's Chorus of Clarity" AA
            Set Click_16 to CastTime=32, ReCastTime=120, Name="Cassindra's Chorus of Clarity", Slot=AA ; and save to INI

      ----------------------------
      MQ2Data Variables:
         bool   Twist         Currently Twisting: "TRUE" or "FALSE", if NULL plugin is not loaded
         Members:
            bool     Twisting  Currently twisting: true/false.
            int      Current   Returns the curent gem number being sung, -1 for item, or 0 if not twisting
            int      Next      Returns the next gem number to be sung, -1 for item, or 0 if not twistsing
            string   List      Returns the twist sequence in a format suitable for /twist

      ----------------------------

     The ini file has the format:
         [MQ2Twist]
         Delay=32       Delay between twists. Lag & System dependant.
         Adjust=1       This defines  how many ticks before the 'normal' recast time to cast a long song.
                        Long songs are defined as songs greater than 3 ticks in length.  If set to 1 tick,
                        and a song lasts 10 ticks, the song will be recast at the 8 tick mark, instead of
                        at the 9 tick mark as it normally would.

         [Click_21] thru [Click_29]
         CastTime=30              Casting Time, -1 to use the normal song delay
         ReCastTime=0             How often to recast, 0 to twist normally.
         Name="Fife of Battle"    Item name for /nomodkey /itemnotify
         Slot=neck                Slot name for /nomodkey /itemnotify (use AA for AA ability)

         Delay, CastTime and ReCastTime are specified in 10ths of a
         second, so 10 = 1 second, and so on.

         INI File Example:
            [MQ2Twist]
            Delay=31
            Quiet=0

            ;Shadowsong cloak
            [Click_21]
            CastTime=30
            ReCastTime=350
            Name=Shadowsong Cloak
            Slot=DISABLED

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

            ;lute of the flowing waters
            [Click_24]
            CastTime=0
            ReCastTime=0
            Name=Lute of the Flowing Waters
            Slot=DISABLED

            ;lute of the flowing waters
            [Click_25]
            CastTime=32
            ReCastTime=120
            Name=Cassindra's Chorus of Clarity
            Slot=AA

            [Click_26] ... [Click_30]
            CastTime=33
            ReCastTime=0
            Name=DISABLED
            Slot=DISABLED

      ----------------------------

Changes:
    // 2.0 - Eqmule 07-22-2016 - Added string safety.
   02-13-10
      Added silent switch to start/stop commands.
   10-13-09
      Added flag in SetGameState() to prevent INI init on zone
   10-12-09
      Corrected item swap in index, added a few more info lines if debug mode is on
   10-10-09
      Corrected max check for items on song rotation
   09-22-09
      Changed click/aa entries to STL, added class (a couple members for future direction), a few other changes
   09-19-09
      Updated to let you twist AA abilities and define clicky/AA songs from inside the game.
   12-23-08
      Updated array lengths to add more clicky slots

   12-17-07
      Support  items Secrets of Faydwer 10 Songs

   10-05-04
      Support "swap in and click" items

   09-15-04
      Support extra spell slot from Omens of War AA

   09-01-04
      Command: /twist quiet to toggle some of the spam on/off
      Various code fixes/speedups

   08-29-04
      Moved LONGSONG_ADJUST into INI file and made /twist adjust command to set it on
      the fly

   08-25-04
      Changed output for /twist once to be slightly less misleading
      Reset click/song timers every time they're called with /twist hold or /twist once;
      if the user's specifying that song, they obviously want to cast it anyway.
      Removed the variable MissedNote as close inspection revealed the only place it was
      checked for was the line that set it. /boggle
      Minor code tweaks, cleanups, formatting changes, etc

   08-24-04 (Pheph)
      Modified it to use only one TLO, as I found it somewhat messy having 4 different ones.
      All the functionality of the old TLO's are now members of ${Twist}
      ${Twising} is now ${Twist.Twisting}, or just ${Twist}
      ${TwistCurrent} is now ${Twist.Current}
      ${TwistNext} is now ${Twist.Next}
      ${TwistList} is now ${Twist.List}

   08-23-04
     Reset_ItemClick_Timers was being called far too often.  Now the only time we reset
     is if a new list of songs are specified.  "/twist ${TwistList}" is a useful alias
     if you for some reason want the old behavior.
     Sing or /twist hold now resets the cast/item timer for that song only, rather than
     the entire list.
     Command: /twist reset calls Reset_ItemClick_Timers without interfering with the
     state of the current twists.

   08-22-04
     Command: /twist once [songlist] will cycle through the songs entered once, then
     revert to the old twist, starting with the song that was interrupted.
     Removed command "/twist on", it was making the string compare for "once" annoying,
     and I didn't think it was worth the effort for a redundant command.
     /twist delay with no argument now returns the delay without resetting it.  Values
     less than 30 now give a warning...maybe they're not bards or have some other
     reason for using a low value.

   08-19-04
      Minor revamp of item notification.  Removed ITEMNOTIFY define and kludged in some
      changes from Virtuoso65 to get casting by item name working.  /cast is no longer
      used.
      Added INI file support for above change.  File now uses distinct entries for item
      names and slots.  *Quotes not required for multi-word item names in INI.*
      Fixed the MQ2Data value TwistCurrent to display the current song as-advertised, and
      added a new value TwistNext with the old behavior of showing the next song in the
      queue. (Useful in scripting)
      Removed a few DebugSpews that were mega-spamming my debugger output.
      CastTime of -1 in the INI file now causes the default delay to be used.

   06-01-04
      Added LONGSONG_ADJUST (default to 1 tick) to help with the timing of recasting long
      songs, such as selo's.
      Twisting is now paused when you sit (this would include camping).  This fixes
      problems reported by Chyld989 (twisting across chars) and Kiniktoo (new autostand on
      cast 'feature' in EQ makes twisting funky)

   05-19-04
      Added workaround for incorrect duration assumption for durationtype=5 songs, such as
      Cassindra's Chant of Clarity or Cassindra's Chorus of Clarity.
      Added check of char state before casting a song. Actually added for 1.05
         Checked states and resulting action are:
            Feigned, or Ducking = /stand
            Stunned = Delay
            Dead - Stop twisting.
         If you're a monk using this to click your epic, you'll want to disable the autostand on feign code =)


   05-05-05
      Fixed CTD on song unmem or death, while twisting.  Oops
      Removed circle functionality.  It's better suited for a plugin like the MQ2MoveUtils
         plugin by tonio at http://macroquest.sourceforge.net/phpBB2/viewtopic.php?t=6973

   05-01-04
      Fixed problem with using pchar before state->ingame causing CTD on eq load (thanks MTBR)
      Fixed vc6 compile error w/ reset_itemclick_timers
      Replaced various incantations of pChar and pSpawn with pLocalPC
      Fixed /circle behavior w/ unspecified y/x
      Fixed /circle on when already circling and you want to update loc
      Added output of parsed circle parameters on start.

   04-25-04
      Converted to MQ2Data
         Top Level Objects:
            bool   Twisting      (if NULL plugin is not loaded)
            int      TwistCurrent
            string   TwistList
      Removed $Param synatax for above
      Added check to make sure item twists specified are defined
      Fixed error with twist parameter processing
      Changed twist startup output to be more verbose
      Command: /twist on added as alias for /twist start
      INI File is now named per-character (MQ2Twist_Charname.ini)
         * Be sure to rename existing ini files
      Modified twist routine to take into account songs with
         non-0 recast times or longer than 3 tick durations,
         and only re-cast them after the appropriate delay.
         This is for songs like Selos 2.5 min duration, etc.
         * Note that this makes no attempt to recover if the song
         effect is dispelled, your macro will need to take care
         of that.
      Added ability to compile-time change the method used for
         clicking items.

   04-13-04
      Changed /circle command to allow calling w/o specifying loc
      Corrected a problem with multiple consecutive missed notes
      Added handling of attempting to sing while stunned
      Command: /twist slots, to list the slot to # associations
      Command: /twist reload, to reload the ini file on the fly
      Command: /twist end, /twist off as aliases for /twist stop
      Command: /sing #, as an alias for /twist hold #

      Added support for item clickies.  Clickies are specified
      as "gem" 10-19. For example, /twist 1 2 10 12

      Added INI file support for storing item clicky info
      and default twist delay.

   04-11-04
      Integrated the /circle code from Easar, runs in a circle.  type
      /circle for help.

*/

#include <mq/Plugin.h>

#include <vector>

PreSetup("MQ2Twist");
PLUGIN_VERSION(3.1);

constexpr auto CLICK_START = 21;
constexpr auto CLICK_MAX = 20;                  // Change this if you want more or less available clickies
                                                // For example, CLICK_MAX 30   - would allow Click_21 through Click_50 (assuming CLICK_START is 21)
constexpr auto MAX_TWIST = NUM_SPELL_GEMS + CLICK_MAX - 1;   // how many total songs can be twisted .. /twist 1 2 3 1 2 4 ...

class _ITEMCLICK
{
    public:
        int index;
        int cast_time;
        int recast;
        long castdue;
        bool disabled;
        bool nousename;
        char slot[MAX_STRING];
        char name[MAX_STRING];
        _ITEMCLICK();
        _ITEMCLICK(const _ITEMCLICK &);
        ~_ITEMCLICK(){};
        _ITEMCLICK &operator=(const _ITEMCLICK &in);
        int operator==(const _ITEMCLICK &in) const;
};

_ITEMCLICK::_ITEMCLICK()
{
    index = 0;
    cast_time = 0;
    recast = 0;
    castdue = 0;
    disabled = true;
    nousename = false;
    slot[0] = 0;
    name[0] = 0;
}

_ITEMCLICK::_ITEMCLICK(const _ITEMCLICK &in)
{
    index = in.index;
    cast_time = in.cast_time;
    recast = in.recast;
    castdue = in.castdue;
    disabled = in.disabled;
    nousename = in.nousename;
    strcpy_s(slot, in.slot);
    strcpy_s(name, in.name);
}

_ITEMCLICK& _ITEMCLICK::operator=(const _ITEMCLICK &in)
{
    this->index = in.index;
    this->cast_time = in.cast_time;
    this->recast = in.recast;
    this->castdue = in.castdue;
    this->disabled = in.disabled;
    this->nousename = in.nousename;
    strcpy_s(this->slot, in.slot);
    strcpy_s(this->name, in.name);
    return (*this);
}

int _ITEMCLICK::operator==(const _ITEMCLICK &in) const
{
    if(this->index != in.index) return 0;
    if(this->cast_time != in.cast_time) return 0;
    if(this->recast != in.recast) return 0;
    if(this->castdue != in.castdue) return 0;
    if(this->disabled != in.disabled) return 0;
    if(this->nousename != in.nousename) return 0;
    if(strcmp(this->slot, in.slot)) return 0;
    if(strcmp(this->name, in.name)) return 0;
    return 1;
}

std::vector<_ITEMCLICK> ItemClick;

bool MQ2TwistEnabled = false;
int LONGSONG_ADJUST=1;                // In TICKS, not seconds.  Used for long songs (greater than 3 ticks in duration). See docs.
int RECAST_ADJUST=0;                  // In 10's of seconds.  Used for recast delay adjustment.
int CAST_TIME=33;
int NumSongs=0;                       // number of songs in current twist
int AltNumSongs=0;                    // alt - if doing twist once - alt saves normal twist values.
int Song[MAX_TWIST];                  // song[n] = stores twist list
int AltSong[MAX_TWIST];               // when twist once - saves noraml twist
long SongNextCast[MAX_TWIST];         // twist list cast time
long AltSongNextCast[MAX_TWIST];
int CurrSong=0;                        // Something strange with CurrSong. Looks like CurrSong always 1+
int AltCurrSong=0;
bool RecastAlt=false;
bool UsingAlt=false;
int PrevSong=0;
int LastAltSong=0;
int HoldSong=0;
long CastDue=0;
bool bTwist=false;
bool altTwist=false;
bool quiet;
char SwappedOutItem[MAX_STRING];
char SwappedOutSlot[MAX_STRING];
bool DebugMode = false, Initialized = false;

void WriteDebug(const char* szFormat, ...)
{
    char szBuffer[MAX_STRING] = {0};
    if(!DebugMode)
        return;
    va_list vaList;
    va_start(vaList, szFormat);
    vsprintf_s(szBuffer, szFormat, vaList);
    WriteChatColor(szBuffer);
}

//get current timestamp in tenths of a second
long GetTime()
{
    SYSTEMTIME st;
    ::GetSystemTime(&st);
    long lCurrent=0;
    lCurrent  = st.wDay    * 24 * 60 * 60 * 10;
    lCurrent += st.wHour        * 60 * 60 * 10;
    lCurrent += st.wMinute           * 60 * 10;
    lCurrent += st.wSecond                * 10;
    lCurrent += (long)(st.wMilliseconds/100);
    return (lCurrent);
}

BOOL ItemFound(char *ItemName)
{
    char zOutput[MAX_STRING]={0};
    if(!ItemName)
        return false;
    sprintf_s(zOutput, "${FindItem[%s].Name}", ItemName);
    ParseMacroData(zOutput, sizeof(zOutput));
    WriteDebug("MQ2Twist::ItemFound(%s)", ItemName);
    if(!_stricmp(zOutput, "null"))
        return false;
    return true;
}

void MQ2TwistDoCommand(PlayerClient* pChar, const char* szLine)
{
    WriteDebug("MQ2Twist::MQ2TwistDoCommand(pChar, %s)", szLine);
    HideDoCommand(pChar, szLine, false); // We want to execute these commands immediately... If for some reason we want the commands delayed we will need to add a bool to deal with it
}

void DoSwapOut()
{
    if (pLocalPC && pLocalPlayer)
    {
        char szTemp[MAX_STRING];
        if (SwappedOutItem[0])
        {
            sprintf_s(szTemp, "/exchange \"%s\" %s", SwappedOutItem, SwappedOutSlot);
            WriteDebug("MQ2Twist::DoSwapOut() = '%s'", szTemp);
            MQ2TwistDoCommand(pLocalPlayer, szTemp);
            SwappedOutItem[0] = 0;
        }
    }
}

void DoSwapIn(unsigned int Index)
{
    if (pLocalPC && pLocalPlayer)
    {
        char szTemp[MAX_STRING];

        WriteDebug("MQ2Twist::DoSwapIn(%d)", Index);
        if (Index >= ItemClick.size()) {
            WriteDebug("MQ2Twist::DoSwapIn(%d) = Item not found in array, returning!", Index);
            return;
        }
        _ITEMCLICK& vr = ItemClick[Index];
        if (!_stricmp(vr.slot, "AA")) {
            WriteDebug("MQ2Twist::DoSwapIn(%d) = Item entry is AA, returning!", Index);
            return;
        }
        if (_strnicmp(vr.slot, "DISABLED", 8)) {
            sprintf_s(szTemp, "${Me.Inventory[%s].Name}", vr.slot);
            ParseMacroData(szTemp, sizeof(szTemp));
            strcpy_s(SwappedOutItem, szTemp);
            strcpy_s(SwappedOutSlot, vr.slot);
            WriteDebug("MQ2Twist::DoSwapIn(%d) = '%s'", Index, szTemp);
            sprintf_s(szTemp, "/exchange \"%s\" %s", vr.name, vr.slot);
            MQ2TwistDoCommand(pLocalPlayer, szTemp);
        }
        else
            WriteDebug("MQ2Twist::DoSwapIn(%d) = Fell through, no action taken!", Index);
    }
}

bool GemReady(unsigned int GemNum) // Gem 1 to NUM_SPELL_GEMS
{
    // May not work for bard, so returning true for now no matter what.  I'll check sometime in future.
    return true;
    /*
    unsigned long nGem=GemNum-1;
    if (nGem<NUM_SPELL_GEMS) {
        if (!((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[nGem])
            return false;
        return(((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[nGem]->spellstate!=1);
    }
    return false;
    */
}

void Reset_ItemClick_Timers()
{
    unsigned int i;
    for (i=0;i<ItemClick.size();i++)
      ItemClick[i].castdue = 0;
    for (i=0;i<MAX_TWIST;i++)
        SongNextCast[i] = 0;
}

void Update_INIFileName() {
    if (pLocalPC)
        sprintf_s(INIFileName,"%s\\%s_%s.ini", gPathConfig, GetServerShortName(), pLocalPC->Name);
    else
        sprintf_s(INIFileName,"%s\\MQ2Twist.ini", gPathConfig);
}

void Load_MQ2Twist_INI()
{
    char szTemp[MAX_STRING]={0};
    char szKey[MAX_STRING]={0};
    _ITEMCLICK ic;

    Update_INIFileName();

    CAST_TIME = GetPrivateProfileInt("MQ2Twist", "Delay", 33, INIFileName);
    WritePrivateProfileInt("MQ2Twist", "Delay", CAST_TIME, INIFileName);
    quiet = GetPrivateProfileBool("MQ2Twist", "Quiet", false, INIFileName);
    WritePrivateProfileBool("MQ2Twist", "Quiet", quiet, INIFileName);

    LONGSONG_ADJUST = GetPrivateProfileInt("MQ2Twist", "Adjust", 1, INIFileName);
    WritePrivateProfileInt("MQ2Twist", "Adjust", LONGSONG_ADJUST, INIFileName);

    RECAST_ADJUST = GetPrivateProfileInt("MQ2Twist", "Recast", 0, INIFileName);
    WritePrivateProfileInt("MQ2Twist", "Recast", RECAST_ADJUST, INIFileName);

    ItemClick.clear();
    for (int i=CLICK_START;i<CLICK_START+CLICK_MAX;i++) {
        sprintf_s(szKey, "%d_CastTime", i);
        ic.cast_time = GetPrivateProfileInt("MQ2Twist",szKey,0,INIFileName);
        sprintf_s(szKey, "%d_ReCastTime", i);
        ic.recast = GetPrivateProfileInt("MQ2Twist",szKey,0,INIFileName);
        sprintf_s(szKey, "%d_Name", i);
        GetPrivateProfileString("MQ2Twist",szKey,"DISABLED",ic.name,MAX_STRING,INIFileName);
        sprintf_s(szKey, "%d_Slot", i);
        GetPrivateProfileString("MQ2Twist",szKey,"DISABLED",ic.slot,MAX_STRING,INIFileName);
        if(!_strnicmp("DISABLED", ic.name, 8)) {
            ic.nousename = true;
            if (!_strnicmp("DISABLED", ic.slot, 8)) {
                ic.disabled = true;
                DebugSpew("MQ2Twist: Slot %d disabled",i);
                WriteDebug("MQ2Twist: Slot %d disabled",i);
            }
            else {
                ic.disabled = false;
            }
        }
        else {
            ic.disabled = false;
            ic.nousename = false;
        }
        // Write the values above back to disk, mostly to initialize it for easy editing.
        sprintf_s(szKey, "%d_CastTime", i);
        WritePrivateProfileInt("MQ2Twist", szKey, ic.cast_time, INIFileName);
        // If the CastTime is set to -1 in the INI file, use the default.
        ic.cast_time = ic.cast_time == -1 ? CAST_TIME : ic.cast_time;
        sprintf_s(szKey, "%d_ReCastTime", i);
        WritePrivateProfileInt("MQ2Twist", szKey, ic.recast, INIFileName);
        sprintf_s(szKey, "%d_Name", i);
        WritePrivateProfileString("MQ2Twist", szKey, ic.name, INIFileName);
        sprintf_s(szKey, "%d_Slot", i);
        WritePrivateProfileString("MQ2Twist", szKey, ic.slot, INIFileName);
        ic.index = i;
        ItemClick.push_back(ic);
        DebugSpewAlways("Initializing MQ2Twist: Processed section MQ2Twist (%d)", i);
        WriteDebug("Initializing MQ2Twist: Processed section MQ2Twist (%d)", i);
    }
}

void SingCommand(PlayerClient* pChar, const char* szLine)
{
    char szTemp[MAX_STRING]={};
    char szMsg[MAX_STRING]={};
    int Index;
    unsigned int fInd;
    unsigned int x;

    GetArg(szTemp,szLine,1);
    Index=atoi(szTemp);

    if (Index>0) {
        HoldSong = Index;
        bTwist=true;
        CastDue = -1;
        WriteChatf("\arMQ2Twist\au::\atHolding Twist and casting gem \ag%d\at.", HoldSong);
        MQ2TwistDoCommand(pChar,"/stopsong");
        if (Index > NUM_SPELL_GEMS) { //item?
            fInd = (unsigned int)ItemClick.size();
            for(x=0; x<ItemClick.size(); x++) {
                if(Index == ItemClick[x].index - CLICK_START) {
                    fInd = x;
                    break;
                }
            }
            if(fInd >= ItemClick.size()) {
                WriteChatf("\arMQ2Twist\au::\arInvalid gem specified (\ay%d\ar), ignoring.", Index);
                return;
            }
            ItemClick[fInd].castdue = 0;
        }
        else
            SongNextCast[Index] = 0;
    }
    else
        WriteChatf("\arMQ2Twist\au::\arInvalid gem specified (\ay%d\ar), ignoring.", Index);
}

void StopTwistCommand(PSPAWNINFO pChar, PCHAR szLine)
{
    char szTemp[MAX_STRING]={0};
    GetArg(szTemp,szLine,1);
    bTwist=false;
    HoldSong=0;
    MQ2TwistDoCommand(pChar,"/stopsong");
    if(_strnicmp(szTemp,"silent",6))
        WriteChatf("\arMQ2Twist\au::\atStopping Twist.");
}

void PrepNextSong() {
    if (CurrSong>NumSongs) {
        if (altTwist) {
            NumSongs=AltNumSongs;
            CurrSong=PrevSong=AltCurrSong;
            for (int i=0; i<NumSongs; i++)  {
                Song[i]=AltSong[i];
                SongNextCast[i]=AltSongNextCast[i];
            }
            altTwist=false;
            if (!quiet)
                WriteChatf("\arMQ2Twist\au::\atOne-shot twist ended, normal twist will resume next pulse.");
        }
        else
            CurrSong=1;
    }
}

void DisplayTwistHelp() {
    WriteChatf("\arMQ2Twist \au- \atTwist song or songs");
    WriteChatf("");
    WriteChatf("\ag/twist [#] [#] [...] \am- \ayTwists in the order given.");
    WriteChatf("\ay  Valid options are 1 thru %d for song gems, and %d thru %d for item clicks/AAs.", NUM_SPELL_GEMS, CLICK_START, CLICK_START+CLICK_MAX-1);
    WriteChatf("\ay  These may be mixed in any order, and repeats are allowable.");
    WriteChatf("\ag/twist hold # \am- \ayPause twisting and sing only the specified song.");
    WriteChatf("\ag/sing # \am- \ayalias for /twist hold.");
    WriteChatf("\ag/twist once # [#] [...] \am- \ayTwists once in the order given, then reverts to original twist.");
    WriteChatf("\ag/twist [start] \am- \ayResume the twist after using /twist hold or /twist stop.");
    WriteChatf("\ag/twist reset \am- \ayReset timers for item clicks and long duration songs.");
    WriteChatf("\ag/twist clear \am- \ayStop twist and clear song list.");
    WriteChatf("\ag/twist delay # \am- \ay10ths of a second, minimum of 30, default 33. (standard gem refresh delay)");
    WriteChatf("\ag/twist recast # \am- \ay10ths of a second, minimum of 0, default 0. (spells with a recast time, additional delay)");
    WriteChatf("\ag/twist adjust # \am- \ayin ticks, how early to recast long duration songs.");
    WriteChatf("\ag/twist stop|end|off \am- \aystop twisting, does not clear the twist queue.");
    WriteChatf("\ag/stoptwist \am- \ayalias for /twist stop.");
    WriteChatf("\ag/twist reload|load \am- \ayreload the INI file to update item clicks/AAs.");
    WriteChatf("\ag/twist slots|list \am- \ayList the slots defined in the INI and their #'s");
    WriteChatf("\ag/twist set \am- \aysets a slot to be defined in the INI.");
    WriteChatf("\ag/twist quiet \am- \ayToggles songs listing and start/stop messages for one-shot twists");
    WriteChatf("\ar---");
}


// **************************************************  *************************
// Function:      Set click info from in game
// Description:   Our /twist command. sing for me!
// **************************************************  *************************
void TwistSetClicky(PCHAR szLine)
{
    char szClick[MAX_STRING]={0};
    char szCast[MAX_STRING]={0};
    char szReCast[MAX_STRING]={0};
    char szName[MAX_STRING]={0};
    char szSlot[MAX_STRING]={0};
    char szKey[MAX_STRING]={0};
    int iClick=0;

    GetArg(szClick,szLine,2);
    GetArg(szCast,szLine,3);
    GetArg(szReCast,szLine,4);
    GetArg(szName,szLine,5);
    GetArg(szSlot,szLine,6);

    if (strlen(szClick)!=0)
        iClick = atoi(szClick);

    if (iClick < CLICK_START || iClick > CLICK_START+CLICK_MAX-1 || !strlen(szCast) || !strlen(szReCast) || !strlen(szName) || !strlen(szSlot)) {
        WriteChatf("\arMQ2Twist\au::\arError with set parameters.");
        WriteChatf("   \aySyntax:  set [click#] [Cast] [Recast] [Name] [Slot]");
        WriteChatf("   \ayExample: set 20 9 182 \"Blade of Vesagran\" Slot8");
        return;
    }

    WriteChatf("\arMQ2Twist\au::\aySetting:");
    WriteChatf("\ag%d_CastTime\am=\at%s",iClick,szCast);
    WriteChatf("\ag%d_ReCastTime\am=\at%s",iClick,szReCast);
    WriteChatf("\ag%d_Name\am=\at%s",iClick,szName);
    WriteChatf("\ag%d_Slot\am=\at%s",iClick,szSlot);

    sprintf_s(szKey, "%d_CastTime", iClick);
    WritePrivateProfileString("MQ2Twist",szKey,szCast,INIFileName);
    sprintf_s(szKey, "%d_ReCastTime", iClick);
    WritePrivateProfileString("MQ2Twist",szKey,szReCast,INIFileName);
    sprintf_s(szKey, "%d_Name", iClick);
    WritePrivateProfileString("MQ2Twist",szKey,szName,INIFileName);
    sprintf_s(szKey, "%d_Slot", iClick);
    WritePrivateProfileString("MQ2Twist",szKey,szSlot,INIFileName);

    Load_MQ2Twist_INI();
    return;
}

// **************************************************  *************************
// Function:      TwistCommand
// Description:   Our /twist command. sing for me!
// **************************************************  *************************
void TwistCommand(PSPAWNINFO pChar, PCHAR szLine)
{
    char szTemp[MAX_STRING]={0}, szTemp1[MAX_STRING]={0};
    char szMsg[MAX_STRING]={0};
    char szChat[MAX_STRING]={0};
    PSPELL pSpell;
    int i;
    unsigned int fInd;

    GetArg(szTemp,szLine,1);

    if ((NumSongs && (!strlen(szTemp)) || !_strnicmp(szTemp,"start", 5))) {
        GetArg(szTemp1,szLine,2);
        if(_strnicmp(szTemp1,"silent",6))
            WriteChatf("\arMQ2Twist\au::\atStarting Twist.");
        DoSwapOut();
        bTwist=true;
        HoldSong=0;
        CastDue = -1;
        return;
    }

    if (!_strnicmp(szTemp,"set", 3)) {
        TwistSetClicky(szLine);
        return;
    }

    if (!_strnicmp(szTemp,"debug", 5)) {
        DebugMode = !DebugMode;
        WriteChatf("\arMQ2Twist\au::\atDebug mode is now %s\ax.", DebugMode?"\ayON":"\agOFF");
        return;
    }

    if (!_strnicmp(szTemp,"stop", 4) || !_strnicmp(szTemp,"end", 3) || !_strnicmp(szTemp,"off", 3)) {
        DoSwapOut();
        GetArg(szTemp1,szLine,2);
        if(_strnicmp(szTemp1,"silent",6))
            StopTwistCommand(pChar, szTemp);
        else
            StopTwistCommand(pChar, szTemp1);
        return;
    }

    if (!_strnicmp(szTemp,"slots", 5) || !_strnicmp(szTemp,"list",4)) {
        WriteChatf("\arMQ2Twist\au::\at'Song' Numbers for right click effects:");
        for (unsigned int j=0; j<ItemClick.size(); j++) {
            if (ItemClick[j].disabled)
                continue;
            if (ItemClick[j].nousename)
                WriteChatf("\ag  %d: \ayslot=%s", ItemClick[j].index, ItemClick[j].slot);
            else
                WriteChatf("\ag  %d: \ayname=%s  slot=%s", ItemClick[j].index, ItemClick[j].name, ItemClick[j].slot);
        }
        WriteChatf("\ar---");
        return;
    }


    if (!_strnicmp(szTemp,"reload", 6) || !_strnicmp(szTemp,"load", 4)) {
        WriteChatf("\arMQ2Twist\au::\atReloading INI Values.");
        Load_MQ2Twist_INI();
        return;
    }

    if (!_strnicmp(szTemp,"delay", 5)) {
        GetArg(szTemp,szLine,2);
        if (strlen(szTemp)) {
            i=atoi(szTemp);
            if (i<=30)
                WriteChatf("\arMQ2Twist\au::\ayWARNING: \arDelay specified is less than standard song cast time.");
            CAST_TIME=i;
            Update_INIFileName();
            WritePrivateProfileInt("MQ2Twist", "Delay", CAST_TIME, INIFileName);
            WriteChatf("\arMQ2Twist\au::\atSet delay to \ag%d\at, INI updated.", CAST_TIME);
        }
        else
            WriteChatf("\arMQ2Twist\au::\atDelay \ag%d\at.", CAST_TIME);
        return;
    }

    if (!_strnicmp(szTemp,"quiet", 5)) {
        quiet = !quiet;
        WritePrivateProfileBool("MQ2Twist", "Quiet", quiet, INIFileName);
        WriteChatf("\arMQ2Twist\au::\atNow being %s\at.", quiet ? "\ayquiet" : "\agnoisy");
        return;
    }

    if (!_strnicmp(szTemp,"adjust", 6)) {
        GetArg(szTemp,szLine,2);
        if (strlen(szTemp)>0) {
            i=atoi(szTemp);
            LONGSONG_ADJUST=i;
            Update_INIFileName();
            WritePrivateProfileInt("MQ2Twist", "Adjust", LONGSONG_ADJUST, INIFileName);
            WriteChatf("\arMQ2Twist\au::\atLong song adjustment set to \ag%d\at, INI updated.", LONGSONG_ADJUST);
        }
        else
            WriteChatf("\arMQ2Twist\au::\atLong song adjustment: \ag%d", LONGSONG_ADJUST);
        return;
    }

    if (!_strnicmp(szTemp,"recast", 6)) {
        GetArg(szTemp,szLine,2);
        if (strlen(szTemp)>0) {
            i=atoi(szTemp);
            RECAST_ADJUST=i;
            Update_INIFileName();
            WritePrivateProfileInt("MQ2Twist", "Recast", RECAST_ADJUST, INIFileName);
            WriteChatf("\arMQ2Twist\au::\atRecast delay adjustment set to \ag%d\at, INI updated.", RECAST_ADJUST);
        }
        else
            WriteChatf("\arMQ2Twist\au::\atRecast delay adjustment: \ag%d", RECAST_ADJUST);
        return;
    }

    if (!_strnicmp(szTemp,"hold", 4)) {
        GetArg(szTemp,szLine,2);
        SingCommand(pChar, szTemp);
        return;
    }

    if (!_strnicmp(szTemp,"reset", 5)) {
        Reset_ItemClick_Timers();
        WriteChatf("\arMQ2Twist\au::\ayTimers reset.");
        return;
    }

    if (!_strnicmp(szTemp,"clear", 5)) {
        NumSongs=0;
        StopTwistCommand(pChar,szTemp);
        if(!quiet)
            WriteChatf("\arMQ2Twist\au::\ayTwist Cleared.");
        return;
    }

    // check help arg, or display if we have no songs defined and /twist was used
    if (!strlen(szTemp) || !_strnicmp(szTemp,"help", 4)) {
        DisplayTwistHelp();
        return;
    }

    // if we are "one-shot twisting", save the current song array and current song
    if (!_strnicmp(szTemp,"once", 4)) {
        WriteChatf("\arMQ2Twist\au::\ayOne-shot twisting.");
        if (altTwist) {
            CurrSong=NumSongs+1;
            PrepNextSong(); // If CurrSong > NumSongs reload the song list
        }
        if (NumSongs) {
            AltNumSongs=NumSongs;
            AltCurrSong=CurrSong;
            for (i=0; i<NumSongs; i++)  {
                AltSong[i]=Song[i];
                AltSongNextCast[i]=SongNextCast[i];
            }
        }
        altTwist=true;
    }
    else
        altTwist=false;

    DoSwapOut();
    DebugSpew("MQ2Twist::TwistCommand Parsing twist order");
    WriteDebug("MQ2Twist::TwistCommand Parsing twist order");
    NumSongs=0;
    HoldSong=0;
    if (!altTwist) {
        if (!quiet)
            WriteChatf("\arMQ2Twist\au::\atTwisting:");
        else
            WriteChatf("\arMQ2Twist\au::\atStarting Twist.");
    }
    for (i=0 + altTwist ? 1 : 0; i<MAX_TWIST; i++) {
        GetArg(szTemp,szLine,i+1);
        if (!strlen(szTemp))
            break;
        Song[NumSongs]=atoi(szTemp);
        if ((Song[NumSongs]>0 && Song[NumSongs]<=NUM_SPELL_GEMS) || (Song[NumSongs]>=CLICK_START && Song[NumSongs]<=CLICK_START+CLICK_MAX-1)) {
            bool IsDisabled = false;
            unsigned int x;
            if (Song[NumSongs]>NUM_SPELL_GEMS) {
                fInd = (unsigned int)ItemClick.size();
                for(x=0; x<ItemClick.size(); x++) {
                    if(ItemClick[x].index == Song[NumSongs]) {
                        if(ItemClick[x].disabled)
                            IsDisabled = true;
                        fInd = x;
                        break;
                    }
                }
                if(fInd >= ItemClick.size())
                    IsDisabled = true;
            }
            if(Song[NumSongs]>NUM_SPELL_GEMS && IsDisabled) {
                WriteChatf("\arMQ2Twist\au::\arUndefined item specified (\ay%s\ar) - ignoring (see INI file).", szTemp);
            }
            else {
                sprintf_s(szMsg, " \ag%s \am- \at", szTemp);
                if (Song[NumSongs]<=NUM_SPELL_GEMS) {
                    pSpell=GetSpellByID(GetPcProfile()->MemorizedSpells[Song[NumSongs]-1]);
                    if (altTwist)
                        SongNextCast[NumSongs] = 0;
                    if (pSpell)
                        strcat_s(szMsg, pSpell->Name);
                }
                else {
                    if (ItemClick[fInd].nousename) {
                        strcat_s(szMsg, ItemClick[fInd].slot);
                    }
                    else {
                        strcat_s(szMsg, ItemClick[fInd].name);
                    }
                    if (altTwist)
                        ItemClick[fInd].castdue = 0;
                }
                if (!quiet)
                    WriteChatf("%s", szMsg);
                NumSongs++;
            }
        }
        else {
            WriteChatf("\arMQ2Twist\au::\arInvalid gem specified (\ay%s\ar) - ignoring.", szTemp);
        }
    }

    if (!quiet)
        WriteChatf("\arMQ2Twist\au::\atTwisting \ag%d \atsong%s.", NumSongs, NumSongs>1 ? "s" : "");

    if (NumSongs>0)
        bTwist=true;
    CurrSong = 1;
    PrevSong = 1;
    CastDue = -1;
    MQ2TwistDoCommand(pChar,"/stopsong");
    if (!altTwist)
        Reset_ItemClick_Timers();
}

/*
Checks to see if character is in a fit state to cast next song/item

Note 1: Do not try to correct SIT state, or you will have to stop the
twist before re-memming songs

Note 2: Since the auto-stand-on-cast bullcrap added to EQ a few patches ago,
chars would stand up every time it tried to twist a song.  So now
we stop twisting at sit.
*/
bool CheckCharState()
{
    if (!bTwist)
        return false;

    if (pLocalPC) {
        if (!pLocalPlayer)
            return false;
        if (pLocalPC->Stunned==1)
            return false;
        switch (pLocalPC->standstate) {
            case STANDSTATE_SIT:
                WriteChatf("\arMQ2Twist\au::\ayStopping Twist.");
                bTwist = false;
                return false;
            case STANDSTATE_FEIGN:
                MQ2TwistDoCommand(pLocalPlayer,"/stand");
                return false;
            case STANDSTATE_DEAD:
                WriteChatf("\arMQ2Twist\au::\ayStopping Twist.");
                bTwist = false;
                return false;
            default:
                break;
        }
        if(InHoverState()) {
            bTwist=false;
            return false;
        }
    }

    // Don't try to twist if the casting window is up, it implies the previous song
    // is still casting, or the user is manually casting a song between our twists
    if (pCastingWnd && pCastingWnd->IsVisible() == true)
        return false;

    return true;
}

class MQ2TwistType : public MQ2Type
{
public:
    enum class TwistMembers {
        Twisting = 1,
        Next,
        Current,
        List,
    };

    MQ2TwistType() : MQ2Type("twist") {
        ScopedTypeMember(TwistMembers, Twisting);
        ScopedTypeMember(TwistMembers, Next);
        ScopedTypeMember(TwistMembers, Current);
        ScopedTypeMember(TwistMembers, List);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
    {
        MQTypeMember* pMember = MQ2TwistType::FindMember(Member);
        if (!pMember)
            return false;

        switch(static_cast<TwistMembers>(pMember->ID))
        {
        case TwistMembers::Twisting:
            /* Returns: bool
            0 - Not Twisting
            1 - Twisting
            */
            Dest.Int=bTwist;
            Dest.Type=mq::datatypes::pBoolType;
            return true;
        case TwistMembers::Next:
            /* Returns: int
            0 - Not Twisting
            -1 - Casting Item
            1-9 - Current Gem
            */
            Dest.Int=HoldSong ? HoldSong : Song[CurrSong-1];
            if (Dest.Int>NUM_SPELL_GEMS-1)
                Dest.Int = -1;
            if (!bTwist)
                Dest.Int = 0;
            Dest.Type=mq::datatypes::pIntType;
            return true;
        case TwistMembers::Current:
            Dest.Int=HoldSong ? HoldSong : Song[PrevSong-1];
            if (Dest.Int>NUM_SPELL_GEMS-1)
                Dest.Int = -1;
            if (!bTwist)
                Dest.Int = 0;
            Dest.Type=mq::datatypes::pIntType;
            return true;
        case TwistMembers::List:
            /* Returns: string
            Space separated list of gem and item #'s being twisted, in order
            */
            int a;
            char szTemp[MAX_STRING] = {0};
            char MQ2TwistTypeTemp[MAX_STRING] = {0};
            for (a=0; a<NumSongs; a++) {
                sprintf_s(szTemp, "%d ", Song[a]);
                strcat_s(MQ2TwistTypeTemp, szTemp);
            }
            strcpy_s(DataTypeTemp, MQ2TwistTypeTemp);
            Dest.Ptr=&DataTypeTemp[0];
            Dest.Type=mq::datatypes::pStringType;
            return true;
        }
        return false;
    }

    bool ToString(MQVarPtr VarPtr, PCHAR Destination) override
    {
        if (bTwist)
            strcpy_s(Destination,MAX_STRING,"TRUE");
        else
            strcpy_s(Destination,MAX_STRING,"FALSE");
        return true;
    }
};
MQ2TwistType* pTwistType = nullptr;

bool dataTwist(const char* szName, MQTypeVar& Dest)
{
    Dest.DWord = 1;
    Dest.Type = pTwistType;
    return true;
}


// ******************************
// **** MQ2 API Calls Follow ****
// ******************************

PLUGIN_API VOID InitializePlugin(VOID)
{
    DebugSpewAlways("Initializing MQ2Twist");
    AddCommand("/twist",TwistCommand,0,1,1);
    AddCommand("/sing",SingCommand,0,1,1);
    AddCommand("/stoptwist",StopTwistCommand,0,0,1);;
    AddMQ2Data("Twist",dataTwist);
    pTwistType = new MQ2TwistType;
    WriteChatf("\atMQ2Twist \agv%1.2f loaded.", MQ2Version);
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("MQ2Twist::Shutting down");
    WriteDebug("MQ2Twist::Shutting down");
    RemoveCommand("/twist");
    RemoveCommand("/sing");
    RemoveCommand("/stoptwist");
    RemoveMQ2Data("Twist");
    delete pTwistType;
}

PLUGIN_API VOID OnPulse(VOID)
{
    char szTemp[MAX_STRING] = {0};
    char mzTemp[MAX_STRING] = {0};
    PSPELL pSpell;
    int TmpRecastTimer;
    unsigned int fInd;
    bool Found;

    if (!MQ2TwistEnabled || !CheckCharState())
        return;

    Found = true;
    if ((HoldSong>0) || ((NumSongs==1) && !altTwist)) {
        WriteDebug("MQ2Twist::Pulse - Single Song");
        if (CastDue<0 || (((CastDue-GetTime()) <= 0) && (pLocalPlayer->CastingData.SpellID == -1))) {
            int SongTodo = HoldSong ? HoldSong : Song[0];
            if (SongTodo <= NUM_SPELL_GEMS) {
                if(!GemReady(SongTodo)) {
                    WriteDebug("MQ2Twist::Pulse - Single Song (Gem %d) - NOT READY", SongTodo);
                    return;
                }
                DebugSpew("MQ2Twist::Pulse - Single Song (Casting Gem %d)", SongTodo);
                WriteDebug("MQ2Twist::Pulse - Single Song (Casting Gem %d)", SongTodo);
                sprintf_s(szTemp,"/multiline ; /stopsong ; /cast %d", SongTodo);
                MQ2TwistDoCommand(pLocalPlayer, szTemp);
                CastDue = GetTime()+CAST_TIME;
            }
            else {
                unsigned int itemID;
                itemID = (unsigned int)ItemClick.size();
                for(fInd=0; fInd<ItemClick.size(); fInd++) {
                    if(ItemClick[fInd].index == SongTodo) {
                        itemID = fInd;
                        break;
                    }
                }
                if(itemID>=ItemClick.size())
                    return;
                if (ItemClick[itemID].castdue-GetTime() <= 0) {
                    if (_stricmp(ItemClick[itemID].slot,"AA")==0) {
                        DebugSpew("MQ2Twist::Pulse - Single Song (Casting AA %d - %s)", SongTodo, ItemClick[itemID].name);
                        WriteDebug("MQ2Twist::Pulse - Single Song (Casting AA %d - %s)", SongTodo, ItemClick[itemID].name);
                        sprintf_s(szTemp,"/aa act %s", ItemClick[itemID].name);
                    }
                    else if (ItemClick[itemID].nousename) {
                        DebugSpew("MQ2Twist::Pulse - Single Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].slot);
                        WriteDebug("MQ2Twist::Pulse - Single Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].slot);
                        sprintf_s(szTemp,"/multiline ; /stopsong ; /nomodkey /itemnotify %s rightmouseup", ItemClick[itemID].slot);
                    }
                    else {
                        DebugSpew("MQ2Twist::Pulse - Single Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].name);
                        WriteDebug("MQ2Twist::Pulse - Single Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].name);
                        if(ItemFound(ItemClick[itemID].name)) {
                            DoSwapIn(itemID);
                            sprintf_s(szTemp,"/multiline ; /stopsong ; /nomodkey /itemnotify \"%s\" rightmouseup", ItemClick[itemID].name); 
                        }
                        else {
                            WriteChatf("MQ2Twist::Pulse - Single Item \"%s\" not found in inventory, skipping.", ItemClick[itemID].name);
                            Found = false;
                        }
                    }
                    if(Found) {
                        MQ2TwistDoCommand(pLocalPlayer, szTemp);
                        ItemClick[itemID].castdue = ItemClick[itemID].recast ? (GetTime()+ItemClick[itemID].cast_time+ItemClick[itemID].recast) : (GetTime()+CAST_TIME);
                        CastDue = ItemClick[itemID].castdue;
                    }
                }
            }
        }
    }
    else {
        UsingAlt=false;
        int SongTodo = Song[CurrSong-1];
        if (RecastAlt) {
            SongTodo=LastAltSong;
            RecastAlt=false;
        }
        if (NumSongs && ((CastDue-GetTime()) <= 0)) {
            DoSwapOut();
            if (SongTodo <= NUM_SPELL_GEMS) {
                if (SongNextCast[CurrSong-1]-GetTime() <= 0) {
                    DebugSpew("MQ2Twist::OnPulse - Next Song = %d", SongTodo);
                    WriteDebug("MQ2Twist::OnPulse - Next Song = %d", SongTodo);
                    sprintf_s(szTemp,"/multiline ; /stopsong ; /cast %d", SongTodo);
                    if (altTwist) {
                        LastAltSong=SongTodo;
                        UsingAlt=true;
                    }
                    PrevSong=CurrSong;
                    if(!GemReady(SongTodo)) {
                        WriteDebug("MQ2Twist::OnPulse - Next Song = %s (Gem %d) - NOT READY, SKIPPING", szTemp, SongTodo);
                        CurrSong++;
                        PrepNextSong();
                        return;
                    }
                    MQ2TwistDoCommand(pLocalPlayer, szTemp);
                    pSpell=GetSpellByID(GetPcProfile()->MemorizedSpells[Song[CurrSong-1]-1]);
                    if(!pSpell) {
                        WriteChatf("\arMQ2Twist\au::\arSongs not present - suspending twist.  /twist to resume.");
                        bTwist = false;
                        return;
                    }
                    CastDue = GetTime()+CAST_TIME;
                    if(!(pSpell->RecastTime))
                        TmpRecastTimer = 0;
                    else
                        TmpRecastTimer = (pSpell->RecastTime+100)/100;
                    const int TmpSpellDuration = GetSpellDuration(pSpell, pLocalPlayer)*60; // duration in 10's of a second
                    // duration > 18 secs
                    if (TmpSpellDuration > 180) {
                        WriteDebug("MQ2Twist::%s has long duration, recast adjusted by +%d.", pSpell->Name, (TmpSpellDuration - (LONGSONG_ADJUST*60)));
                        SongNextCast[CurrSong-1] = CastDue + TmpSpellDuration - (LONGSONG_ADJUST*60);
                    }
                    // recast > 0 secs
                    else if (TmpRecastTimer > 0) {
                        WriteDebug("MQ2Twist::%s has recast time, recast adjusted by +%d.", pSpell->Name, TmpRecastTimer+RECAST_ADJUST);
                        SongNextCast[CurrSong-1] = CastDue + TmpRecastTimer + RECAST_ADJUST;
                    }
                    // normal
                    else
                        SongNextCast[CurrSong-1] = CastDue;
                    PrevSong=CurrSong;
                } // if it's not time for currsong to be re-sung, skip it in the twist
                CurrSong++;
                PrepNextSong();
            }
            else {
                unsigned int itemID;
                itemID = (unsigned int)ItemClick.size();
                for(fInd=0; fInd<ItemClick.size(); fInd++) {
                    if(ItemClick[fInd].index == SongTodo) {
                        itemID = fInd;
                        break;
                    }
                }
                if(itemID>=ItemClick.size())
                    return;
                if (ItemClick[itemID].castdue-GetTime() <= 0) {
                    if (_stricmp(ItemClick[itemID].slot,"AA")==0) {
                        DebugSpew("MQ2Twist::Pulse - Single Song (Casting AA %d - %s)", SongTodo, ItemClick[itemID].name);
                        WriteDebug("MQ2Twist::Pulse - Single Song (Casting AA %d - %s)", SongTodo, ItemClick[itemID].name);
                        sprintf_s(szTemp,"/aa act %s", ItemClick[itemID].name);
                    }
                    else if (ItemClick[itemID].nousename) {
                        DebugSpew("MQ2Twist::Pulse - Next Song (Casting Slot %d - %s)", SongTodo, ItemClick[itemID].slot);
                        WriteDebug("MQ2Twist::Pulse - Next Song (Casting Slot %d - %s)", SongTodo, ItemClick[itemID].slot);
                        sprintf_s(szTemp,"/multiline ; /stopsong ; /nomodkey /itemnotify %s rightmouseup", ItemClick[itemID].slot);
                    }
                    else {
                        DebugSpew("MQ2Twist::Pulse - Next Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].name);
                        WriteDebug("MQ2Twist::Pulse - Next Song (Casting Item %d - %s)", SongTodo, ItemClick[itemID].name);
                        WriteDebug("MQ2Twist::Pulse - Calling DoSwapIn(%d)", itemID);
                        if(ItemFound(ItemClick[itemID].name)) {
                            DoSwapIn(itemID);
                            sprintf_s(szTemp,"/multiline ; /stopsong ; /nomodkey /itemnotify \"%s\" rightmouseup", ItemClick[itemID].name);  
                        }
                        else {
                            WriteChatf("MQ2Twist::Pulse - Item \"%s\" not found in inventory, skipping.", ItemClick[itemID].name);
                            Found = false;
                        }
                    }
                    if(Found) {
                        WriteDebug("MQ2Twist::Pulse - Calling MQ2TwistDoCommand(pLocalPlayer, %s)", szTemp);
                        MQ2TwistDoCommand(pLocalPlayer, szTemp);
                        ItemClick[itemID].castdue = ItemClick[itemID].recast ? (GetTime()+ItemClick[itemID].cast_time+ItemClick[itemID].recast) : (GetTime()+CAST_TIME);
                        CastDue = GetTime()+ItemClick[itemID].cast_time;
                    }
                }
                PrevSong=CurrSong;   // Increment twist position even if we didn't do an itemnotify - this might have a long recast
                CurrSong++;         // interval set, and we just skip it until it's time to recast, rather than keep a separate timer.
                PrepNextSong();
            }
        }
    }
}

PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
    char szMsg[MAX_STRING]={0};
    if (!bTwist || !MQ2TwistEnabled)
        return 0;
    // DebugSpew("MQ2Twist::OnIncomingChat(%s)",Line);

    if ( (strstr(Line,"You miss a note, bringing your ") && strstr(Line," to a close!")) ||
                !strcmp(Line,"You haven't recovered yet...") ||
                (strstr(Line,"Your ") && strstr(Line," spell is interrupted.")) ) {
        DebugSpew("MQ2Twist::OnIncomingChat - Song Interrupt Event");
        WriteDebug("MQ2Twist::OnIncomingChat - Song Interrupt Event");
        if (!HoldSong)
            CurrSong=PrevSong;
        if (UsingAlt)
            RecastAlt=true;
        CastDue = -1;
        SongNextCast[CurrSong-1] = -1;
        return 0;
    }

    if (!strcmp(Line,"You can't cast spells while stunned!") ) {
        DebugSpew("MQ2Twist::OnIncomingChat - Song Interrupt Event (stun)");
        WriteDebug("MQ2Twist::OnIncomingChat - Song Interrupt Event (stun)");
        if (!HoldSong)
            CurrSong=PrevSong;
        if (altTwist)
            CurrSong=LastAltSong;
        CastDue = GetTime() + 10;
        // Wait one second before trying again, to avoid spamming the trigger text w/ cast attempts
        return 0;
    }
    return 0;
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
    DebugSpew("MQ2Twist::SetGameState()");
    WriteDebug("MQ2Twist::SetGameState()");
    if (gGameState==GAMESTATE_INGAME) {
        MQ2TwistEnabled = true;
        if(!Initialized) {
            Initialized = true;
            Load_MQ2Twist_INI();
        }
    }
    else {
        if (gGameState==GAMESTATE_CHARSELECT)
            Initialized = false;
        MQ2TwistEnabled = false;
    }
}
