/***** NOTICE ******
Many functions included in this source code are not copyrighted to the developer and are used with permissions
restricting their release to VIP board members of MacroQuest2.com. These functions have comments above them stating
to whom the copyright belongs to. If you intend to redistribute this source or binaries compiled from this source
outside of a direct link to the forum post in which it is released, you must get permission from these authors. Otherwise
link to the forum post directly to encourage any users of this plugin to donate to the developers at MacroQuest2.com
as required by the copyright holders of these functions, and desired by the developer. Please show your support!
****************/
// version information
//Version 12.0 Added string safety. Fixed Blech crash. - EqMule Jul 19 2016
//Version 12.1 Added new savecode and fixed the offsetfinder - EqMule Sep 12 2017
//Version 12.2 Updated patterns - SwiftyMUSE Feb 21 2018
//Version 12.3 Moved patterns - SwiftyMUSE Apr 23 2018
//Version 12.4 Fixed a NULL ptr crash - EqMule May 28 2018

#include <mq/Plugin.h>

#include <cmath>

constexpr auto PLUGIN_NAME = "MQ2MoveUtils";
PLUGIN_VERSION(12.5);

PreSetup(PLUGIN_NAME);

// uncomment these lines to enable debugspew spam
//#define DEBUGMAIN
// debugspew for stucklogic
//#define DEBUGSTUCK
// debugspew for trivial messages
//#define DEBUGMISC

Blech *pMoveEvent = 0;
// ------------------------------------------------------------------------------
// constants - the unsigned chars are aesthetics but we use different values
// so that nothing can be called incorrectly and erroneously match a switch case
// ------------------------------------------------------------------------------
// check calling command
const unsigned char CMD_STICK       = 1;
const unsigned char CMD_MOVETO      = 2;
const unsigned char CMD_CIRCLE      = 3;
const unsigned char CMD_MAKECAMP    = 4;
// walk handling
const unsigned char MU_WALKON       = 10;
const unsigned char MU_WALKOFF      = 11;
const unsigned char MU_WALKIGNORE   = 12;
// reset altcamp or make new altcamp
const unsigned char SET_ALT         = 20;
const unsigned char RESET_ALT       = 21;
// direction to move
const unsigned char GO_FORWARD      = 30;
const unsigned char GO_BACKWARD     = 31;
const unsigned char GO_LEFT         = 32;
const unsigned char GO_RIGHT        = 33;
const unsigned char KILL_STRAFE     = 34;
const unsigned char KILL_FB         = 35;
const unsigned char APPLY_TO_ALL    = 36;
// help output
const unsigned char HELP_SETTINGS   = 50;
// error messages
const unsigned char ERR_STICKSELF   = 60;
const unsigned char ERR_STICKNONE   = 61;
const unsigned char ERR_BADMOVETO   = 62;
const unsigned char ERR_BADMAKECAMP = 63;
const unsigned char ERR_BADCIRCLE   = 64;
const unsigned char ERR_BADSPAWN    = 65;
const unsigned char ERR_BADDELAY    = 66;
// debug output
const unsigned char DBG_MAIN        = 200;
const unsigned char DBG_STUCK       = 201;
const unsigned char DBG_MISC        = 202;
const unsigned char DBG_DISABLE     = 203;

// ------------------------------------------
// formulas & randomization
const float CIRCLE_QUARTER          = 90.0f;
const float CIRCLE_HALF             = 180.0f;
const float CIRCLE_MAX              = 360.0f;
const float HEADING_QUARTER         = 128.0f;
const float HEADING_HALF            = 256.0f;
const float HEADING_MAX             = 512.0f;
const int   BEHIND_ARC              = 45;
const int   FRONT_ARC               = 240;
const int   NOT_FRONT_ARC           = 135;
const int   PIN_ARC_MIN             = 112;
const int   PIN_ARC_MAX             = 144;
// initialization
const float H_INACTIVE              = 10000.0f;
const char  H_FAST                  = 0;
const char  H_LOOSE                 = 1;
const char  H_TRUE                  = 2;
const char  T_INACTIVE              = 0;
const char  T_WAITING               = 1;
const char  T_READY                 = 2;
// stucklogic ring size
const int   MAXRINGSIZE             = 32; // MovingAvg max pulses to average

// ------------------------------------------
// class instances
class CMUSettings*     SET      = NULL;
class CMUActive*       pMU      = NULL;
class CMUCharacter*    ME       = NULL;
class CMUWndHandler*   WINDOW   = NULL;
class CMUMovement*     MOVE     = NULL;
class CStickCmd*       STICK    = NULL;
class CMoveToCmd*      MOVETO   = NULL;
class CCircleCmd*      CIRCLE   = NULL;
class CStuckLogic*     STUCK    = NULL;
class CCampHandler*    CAMP     = NULL;
class CAltCamp*        ALTCAMP  = NULL;
class CCampCmd*        CURCAMP  = NULL;
class CPauseHandler*   PAUSE    = NULL;
class CMULoc*          SUMMON   = NULL;
class CCircleSettings* SET_C    = NULL;
class CMoveToSettings* SET_M    = NULL;
class CCampSettings*   SET_CAMP = NULL;
class CStickSettings*  SET_S    = NULL;

// ---------------------------------
// verbosity bit flags
enum VERBLEVEL {
    V_SILENCE       = 0,
    V_AUTOPAUSE     = 1,
    V_MOVEPAUSE     = 2,
    V_MOUSEPAUSE    = 4,
    V_FEIGN         = 8,
    V_HIDEHELP      = 16,
    V_STICKV        = 32,
    V_STICKFV       = 64,
    V_MOVETOV       = 128,
    V_MOVETOFV      = 256,
    V_MAKECAMPV     = 512,
    V_MAKECAMPFV    = 1024,
    V_CIRCLEV       = 2048,
    V_CIRCLEFV      = 4096,
    V_SETTINGS      = 8192,
    V_SAVED         = 16384,
    V_BREAKONWARP   = 32768,
    V_BREAKONAGGRO  = 65536,
    V_BREAKONHIT    = 131072,
    V_BREAKONSUMMON = 262144,
    V_BREAKONGM     = 524288,
    V_BREAKONGATE   = 1048576,
    V_STICKALWAYS   = 2097152,
    V_ERRORS        = 4194304,
    V_RANDOMIZE     = 8388608,
    V_PAUSED        = 16777216,
    V_VERBOSITY     = 2720, // normal verbosity msgs
    V_FULLVERBOSITY = 11736390, // full verbosity msgs
    V_EVERYTHING    = 33554431, // all messages on (dont add verb + fullverb in)
};
unsigned int uiVerbLevel   = V_EVERYTHING;
unsigned int uiRetainFlags = V_EVERYTHING; // stores flags for when totalsilence toggle

// -----------------------
// import / export

PLUGIN_API float fStickDistance = 0; // exporting to share the distance we're sticking.
PLUGIN_API bool bStickOn = false; // stick active or off, exported for other plugins to have a sure way of always knowing
PLUGIN_API void StickCommand(PlayerClient* pLPlayer, char* szLine); // exported wrapper for MQ2Melee support
// note to any developers: if you intend to use these exports and want to shut off stick, do not flip STICK->On directly,
// instead, call StickCommand(pLPlayer, "off")
bool* pbMULoaded = NULL; // imported from mq2melee in InitializePlugin()
bool bWrapped = false; // hi htw!

// -----------------------
// strings

char szMsg[MAX_STRING]         = {};                 // use for generic msg output
char szDebugName[MAX_STRING]   = {};                 // debug file name
char szCharName[MAX_STRING]    = {};                 // stores char name for INI read/write
const char szOn[10]            = "\agON\ax";         // used in outputs
const char szOff[10]           = "\arOFF\ax";        // used in outputs
const char szArriveMove[50]    = "/moveto location"; // output moveto arrival
const char szArriveCamp[50]    = "camp from /makecamp return"; // output camp return arrival
const char szArriveAlt[50]     = "camp from /makecamp altreturn"; // output altcamp return arrival

// ----------------------------------------
// events

unsigned int Event_AggroNorm   = NULL;
unsigned int Event_MissNorm    = NULL;
unsigned int Event_AggroAbbrev = NULL;
unsigned int Event_MissAbbrev  = NULL;
unsigned int Event_MissNumOnly = NULL;
unsigned int Event_Gates       = NULL;

// ----------------------------------------
// function prototypes

void SpewMUError(unsigned char ucErrorNum);
void SpewDebug(unsigned char ucDbgType, const char* szOuput, ...);
void OutputHelp(unsigned char ucCmdUsed, bool bOnlyCmdHelp = false);
void WriteLine(const char* szOutput, VERBLEVEL V_COMPARE);
void HandleOurCmd(unsigned char ucCmdUsed, const char* szInput);
void EndPreviousCmd(bool bKillMovement, unsigned char ucCmdUsed = APPLY_TO_ALL, bool bPreserveSelf = false);
void ChangeSetting(unsigned char ucCmdUsed, bool bToggle, char szSetting[MAX_STRING]);
void SaveConfig();
void LoadConfig();
void DebugToWnd(unsigned char ucCmdUsed);
void DebugToINI(unsigned char ucCmdUsed);
void DebugToDebugger(const char* szFormat, ...);
void SetupEvents(bool bAddEvent, bool bForceRemove = false);
inline bool ValidIngame(bool bCheckDead = true);

// ----------------------------------------
// ************* CLASSES *****************
// ----------------------------------------

// ----------------------
// inherit-only classes

class CMULoc
{
// locations & dist comparisons
public:
    float Y;
    float X;
    float Z;
    float CurDist;
    float DifDist;

    CMULoc()
    {
        Y       = 0.0f;
        X       = 0.0f;
        Z       = 0.0f;
        CurDist = 0.0f;
        DifDist = 0.0f;
    }
};

class CMUDelay
{
// delay & time calculations
public:
    int Min;
    int Max;

    void TimeStop()
    {
        Resume = T_INACTIVE;
    }

    void TimeStart()
    {
        GetSystemTime(&Began);
        Resume = rand() % (Max - Min + 1) + Min;
    }

    char TimeStatus()
    {
        if (Resume == T_INACTIVE)
        {
            return T_INACTIVE;
        }
        if (ElapsedMS() >= Resume)
        {
            return T_READY;
        }
        return T_WAITING;
    }

    void Validate()
    {
        MinDelay(Min);
        MaxDelay(Max);
    }

    void MinDelay(int iNew)
    {
        Min = iNew;
        if (Min < 125) Min = 125;
    }

    void MaxDelay(int iNew)
    {
        Max = iNew;
        if (Max < Min + 125) Max = Min + 125;
    }

    CMUDelay()
    {
        Min    = 0;
        Max    = 0;
        Resume = T_INACTIVE;
    }

protected:
    int        Resume; // calculated resume time
    SYSTEMTIME Began;  // timer start

    int ElapsedMS()
    {
        SYSTEMTIME     stCurr, stResult;
        FILETIME       ftPrev, ftCurr, ftResult;
        ULARGE_INTEGER prev,   curr,   result;

        GetSystemTime(&stCurr);
        SystemTimeToFileTime(&Began, &ftPrev);
        SystemTimeToFileTime(&stCurr, &ftCurr);
        prev.HighPart           = ftPrev.dwHighDateTime;
        prev.LowPart            = ftPrev.dwLowDateTime;
        curr.HighPart           = ftCurr.dwHighDateTime;
        curr.LowPart            = ftCurr.dwLowDateTime;
        result.QuadPart         = curr.QuadPart - prev.QuadPart;
        ftResult.dwHighDateTime = result.HighPart;
        ftResult.dwLowDateTime  = result.LowPart;
        FileTimeToSystemTime(&ftResult, &stResult);
        return ((int)(stResult.wSecond * 1000 + stResult.wMilliseconds));
    }
};

// ----------------------------------------
// character functions
class CMUCharacter
{
public:
    bool IsBard()
    {
        return pLocalPC && pLocalPlayer && pLocalPlayer->mActorClient.Class == Bard;
    }

    bool InCombat()
    {
        if (ValidIngame() && pPlayerWnd->CombatState == 0 && pPlayerWnd->GetChildItem("PW_CombatStateAnim"))
        {
            return true;
        }
        return false;
    }

    bool IsMe(PlayerClient* pCheck)
    {
        if (!pCheck || !pLocalPlayer) return false;
        if (pCheck->SpawnID == pCharSpawn->SpawnID || pCheck->SpawnID == pLocalPlayer->SpawnID)
        {
            return true;
        }
        return false;
    }
};

// ----------------------------------------------------------
// configuration classes - store default & INI-saved settings

class CStuckLogic : public CMULoc
{
public:
    bool         On;       // INI: stucklogic active or off
    bool         Jump;     // INI: if true, try to jump when stuck
    bool         TurnHalf; // INI: if true, reset heading and try other dir if gone halfway without freeing
    unsigned int Check;    // INI: # of pulses to average distance for stuck awareness
    unsigned int Unstuck;  // INI: if StuckDec == this, consider unstuck
    float        Dist;     // INI: dist needed to move else considered stuck, compared against pulse average
    float        TurnSize; // Turn increment value for left/right (sign flipped by TurnHalf)
    unsigned int StuckInc; // increments each pulse we haven't moved beyond Dist until we reach Check value which == we are stuck
    unsigned int StuckDec; // increments each pulse we've moved again after being stuck, when Unstuck = StuckDec, we force unstuck

    void Reset()
    {
        Y = X = Z = 0.0f;
        DifDist   = 0.0f;
        CurDist   = 1.0f;
        StuckInc  = 0;
        StuckDec  = 0;
    }

    CStuckLogic()
    {
        // plugin defaults established here
        On       = true;
        Jump     = false;
        TurnHalf = true;
        TurnSize = 10.0f;
        Dist     = 0.1f;
        Check    = 6;
        Unstuck  = 10;
        StuckInc = 0;
        StuckDec = 0;
        CurDist  = 1.0f; // baseline to not trigger when first starting movement
        // Y X Z DifDist already initialized by CMULoc() inherit
    }
};

class CStickSettings : public CMUDelay
{
public:
    bool  BreakGate;    // INI: stick breaks if "mob_name Gates." message
    bool  BreakHit;     // INI: stick breaks if npc is attacking
    bool  BreakTarget;  // INI: stick breaks if target switched
    bool  BreakWarp;    // INI: stick breaks if target warps out of DistBreak
    bool  Flex;         // INI: stick flexibility when close
    bool  PauseWarp;    // INI: stick pauses if target warps out of DistBreak
    bool  Randomize;    // INI: randomize strafe arcs during stick
    bool  DelayStrafe;  // INI: strafe sticks use a delay timer with TimedStrafe()
    bool  UseBack;      // INI: use backwards walking when close to target
    bool  UseFleeing;   // INI: 'front' will not strafe if target is fleeing
    bool  Walk;         // INI: stick walks when strafing
    bool  UW;           // look angle up or down at target (/stick uw)
    float ArcBehind;    // INI: arc size for stick behind
    float ArcNotFront;  // INI: arc size for stick !front
    float DistBack;     // INI: within this dist UseBack handles positioning if enabled
    float DistBreak;    // INI: target warps this distance, BreakWarp triggers if enabled
    float DistFlex;     // INI: distance for flexibility
    float DistMod;      // INI: raw modifier to Dist (CStickCmd) (best used if plugin is auto-setting dist)
    float DistModP;     // INI: % modifier to Dist (CStickCmd) (best used if plugin is auto-setting dist)
    float DistSnap;     // INI: default distance from target to snaproll

    CStickSettings()
    {
        // plugin defaults, established here
        Min         = 1500; // inherit: CMUDelay (for strafe)
        Max         = 3000; // inherit: CMUDelay (for strafe)
        BreakGate   = true;
        BreakHit    = false;
        BreakTarget = false;
        BreakWarp   = true;
        PauseWarp   = false;
        Flex        = false;
        Randomize   = false;
        DelayStrafe = true;
        UseBack     = true;
        UseFleeing  = true;
        Walk        = false;
        UW          = false;
        ArcBehind   = BEHIND_ARC;
        ArcNotFront = NOT_FRONT_ARC;
        DistBack    = 10.0f;
        DistBreak   = 250.0f;
        DistFlex    = 7.0f;
        DistMod     = 0.0f;
        DistModP    = 1.0f;
        DistSnap    = 10.0f;
    }
};

class CCampSettings : public CMUDelay
{
public:
    bool  HaveTarget; // INI: if true, auto camp return even with a target
    bool  NoAggro;    // INI: if true, auto camp return only if not aggro
    bool  NotLoot;    // INI: if true, auto camp return only if not looting
    bool  Scatter;    // INI: camp return scattering active or off
    bool  Realtime;   // INI: makecamp player updates pc anchor Y/X while returning
    bool  Leash;      // INI: camp leashing active or off
    float Bearing;    // INI: bearing for camp return scattering
    float Length;     // INI: length of leash checked against anchor
    float Radius;     // INI: default camp radius size
    float ScatSize;   // INI: camp return scatter radius size
    float ScatDist;   // INI: dist from anchor for camp return scattering

    CCampSettings()
    {
        Min        = 500;  // inherit: CMUDelay
        Max        = 1500; // inherit: CMUDelay
        HaveTarget = false;
        NoAggro    = false;
        NotLoot    = false;
        Scatter    = false;
        Realtime   = false;
        Leash      = false;
        Bearing    = 0.0f;
        Length     = 50.0f;
        Radius     = 40.0f;
        ScatSize   = 10.0f;
        ScatDist   = 10.0f;
    }

    void SetRadius(float fNew)
    {
        Radius = fNew;
        ValidateSizes();
    }

    void SetLeash(float fNew)
    {
        Length = fNew;
        ValidateSizes();
    }

protected:
    void ValidateSizes()
    {
        if (Radius < 5.0f)  Radius = 5.0f;  // enforce min Radius size 5.0f
        float fTemp = Radius + 5.0f;
        if (Length < fTemp) Length = fTemp; // enforce min leash size 5.0f >= Radius
    }
};

class CMoveToSettings
{
public:
    bool  BreakAggro; // INI: break moveto if aggro gained
    bool  BreakHit;   // INI: break moveto if attacked (blech event)
    bool  UseBack;    // INI: use backwards walking when initially close to destination
    bool  UW;         // INI: moveto uses UW face angle adjustments
    bool  Walk;       // INI: moveto walks if close to arrivaldist
    float Dist;       // INI: how close to moveto location is considered acceptable
    float DistBack;   // INI: within this dist UseBack handles positioning if enabled
    float DistY;      // INI: how close to moveto Y location is acceptable for precisey
    float DistX;      // INI: how close to moveto X location is acceptable for precisex
    float Mod;        // INI: Dist percent modifier

    CMoveToSettings()
    {
        BreakAggro = false;
        BreakHit   = false;
        UseBack    = false;
        UW         = false;
        Walk       = true;
        Dist       = 10.0f;
        DistBack   = 30.0f;
        DistY      = 10.0f;
        DistX      = 10.0f;
        Mod        = 0.0f;
    }
};

class CCircleSettings
{
public:
    bool  Backward; // INI: always backwards
    bool  CCW;      // INI: always counter-clockwise
    bool  Drunk;    // INI: always drunken
    float CMod;     // INI: default radius percent modifer
    float Radius;   // INI: default radius size

    CCircleSettings()
    {
        Backward = false;
        CCW      = false;
        Drunk    = false;
        CMod     = 0.0f;
        Radius   = 30.0f;
    }

    void SetRadius(float fNew)
    {
        // enforce min radius size 5.0f
        Radius = fNew;
        if (Radius < 5.0f) Radius = 5.0f;
    }
};

class CMUSettings
{
public:
    bool  AutoSave;    // INI: autosave ini file when using 'toggle' or 'set'
    bool  AutoPause;   // INI: pause automatically when casting/stunned/self targeted/sitting
    bool  AutoUW;      // INI: automatically use 'uw' when underwater
    bool  BreakGM;     // INI: command breaks if visible GM enters zone
    bool  BreakSummon; // INI: command breaks if you move too far in a single pulse (summoned)
    bool  BreakKB;     // INI: break command if movement key pressed
    bool  PauseKB;     // INI: pause command if movement key pressed
    bool  BreakMouse;  // INI: break command if mouselook active
    bool  PauseMouse;  // INI: pause command if mouselook active
    bool  Feign;       // INI: do not stand if currently FD
    bool  LockPause;   // INI: pause will not reset until unpaused
    bool  SaveByChar;  // INI: save some settings for individual characters
    bool  Spin;        // INI: if true, stick front ignores requiring being on HoTT
    bool  Window;      // INI: use dedicated UI window
    bool  WinEQ;       // INI: use old-style movement
    float AllowMove;   // INI: distance to allow forward movement while turning, CanLooseMove()
    float DistSummon;  // INI: distance your character moves in a single pulse to trigger BreakSummon
    float TurnRate;    // INI: rate at which to turn using loose heading (14 is default)
    int   Head;        // INI: heading adjustment type (0 = fast [H_FAST], 1 = loose [H_LOOSE], 2 = true [H_TRUE])

    CMUSettings()
    {
        SET_CAMP = new CCampSettings();
        SET_S    = new CStickSettings();
        SET_M    = new CMoveToSettings();
        SET_C    = new CCircleSettings();

        AutoSave    = true;
        AutoPause   = true;
        AutoUW      = false;
        BreakGM     = true;
        BreakSummon = false;
        BreakKB     = true;
        PauseKB     = false;
        BreakMouse  = false;
        PauseMouse  = false;
        Feign       = false;
        LockPause   = false;
        SaveByChar  = true;
        Spin        = false;
        Window      = false;
        WinEQ       = false;
        AllowMove   = 32.0f;
        DistSummon  = 8.0f;
        TurnRate    = 14.0f;
        Head        = H_TRUE;
    }

    ~CMUSettings()
    {
        delete SET_CAMP;
        SET_CAMP = NULL;
        delete SET_S;
        SET_S    = NULL;
        delete SET_M;
        SET_M    = NULL;
        delete SET_C;
        SET_C    = NULL;
    }
};

// ---------------------------------------------------
// command classes - instanced for individual cmd use

class CCircleCmd : public CMULoc, public CMUDelay, public CCircleSettings
{
public:
    bool On; // circling active or off

    bool Wait()
    {
        // drunken circling uses this formula
        if (ElapsedMS() > Max + GetDrunk(Min))
        {
            TimeStart();
            return false;
        }
        return true;
    }

    void AtMe()
    {
        // HandleOurCmd calls this to establish '/circle on' without loc supplied
        PlayerClient* pChSpawn = pCharSpawn;
        Y = pChSpawn->Y + Radius * sin(pChSpawn->Heading * (float)PI / HEADING_HALF);
        X = pChSpawn->X + Radius * cos(pChSpawn->Heading * (float)PI / HEADING_HALF);
        On = true;
    }

    void AtLoc(float fY, float fX)
    {
        // HandleOurCmd calls this with desired Y X supplied
        Y  = fY;
        X  = fX;
        On = true;
    }

    CCircleCmd() // ooo: CMULoc(), CMUDelay(), CCircleSettings(), CCircleCmd()
    {
        Min = 600;      // inherit: CMUDelay
        Max = 900;      // inherit: CMUDelay
        On  = false;
        UserDefaults(); // copy defaults from INI settings
        TimeStart();
    }

protected:
    void UserDefaults()
    {
        Backward = SET_C->Backward;
        CCW      = SET_C->CCW;
        Drunk    = SET_C->Drunk;
        CMod     = SET_C->CMod;
        Radius   = SET_C->Radius;
    }

    int GetDrunk(int iNum)
    {
        return (iNum * rand() / (RAND_MAX + 1));
    }
};

class CMoveToCmd : public CMULoc, public CMoveToSettings
{
public:
    bool  On;       // moveto active or off (MOVETO->On)
    bool  PreciseY; // moveto arrivaldist is only checked against Y
    bool  PreciseX; // moveto arrivaldist is only checked against X

    bool DidAggro();

    CMoveToCmd()
    {
        On       = false;
        PreciseY = false;
        PreciseX = false;
        UserDefaults();
    }

    void Activate(float fY, float fX, float fZ)
    {
        Y  = fY;
        X  = fX;
        Z  = fZ;
        On = true;
    }

protected:
    void UserDefaults()
    {
        BreakAggro = SET_M->BreakAggro;
        BreakHit   = SET_M->BreakHit;
        UseBack    = SET_M->UseBack;
        UW         = SET_M->UW;
        Walk       = SET_M->Walk;
        Dist       = SET_M->Dist;
        DistBack   = SET_M->DistBack;
        DistY      = SET_M->DistY;
        DistX      = SET_M->DistX;
        Mod        = SET_M->Mod;
    }
};

class CCampCmd : public CMULoc, public CCampSettings
{
public:
    bool          On;                  // camp active or off
    bool          Pc;                  // makecamp player on or off
    bool          RedoStick;           // set during camp return
    bool          RedoCircle;          // set during camp return
    unsigned long PcID;                // stores id of makecamp player
    eSpawnType    PcType;              // stores spawn type of makecamp player
    char          PcName[MAX_STRING];  // stores makecamp player displayedname

    CCampCmd()
    {
        On         = false;
        Pc         = false;
        RedoStick  = false;
        RedoCircle = false;
        PcID       = 0;
        PcType     = NONE;
        memset(&PcName, 0, MAX_STRING);
        UserDefaults();
    }

protected:
    void UserDefaults()
    {
        Min        = SET_CAMP->Min;
        Max        = SET_CAMP->Max;
        HaveTarget = SET_CAMP->HaveTarget;
        NoAggro    = SET_CAMP->NoAggro;
        NotLoot    = SET_CAMP->NotLoot;
        Scatter    = SET_CAMP->Scatter;
        Realtime   = SET_CAMP->Realtime;
        Leash      = SET_CAMP->Leash;
        Bearing    = SET_CAMP->Bearing;
        Length     = SET_CAMP->Length;
        Radius     = SET_CAMP->Radius;
        ScatSize   = SET_CAMP->ScatSize;
        ScatDist   = SET_CAMP->ScatDist;
    }
};

class CAltCamp : public CMULoc
{
public:
    bool  On;
    float Radius;

    void Update(CCampCmd* Cur)
    {
        Y      = Cur->Y;
        X      = Cur->X;
        Radius = Cur->Radius;
        On     = true;
    }

    CAltCamp()
    {
        On     = false;
        Radius = 0.0f;
    }
};

class CCampHandler : public CMUDelay, public CMULoc
{
public:
    bool Auto;      // state awareness (is camp return auto or manual)
    bool DoAlt;     // true when manually forcing a return to altcamp
    bool DoReturn;  // true when manually forcing a return to curcamp
    bool Returning; // true when any return is active (both auto and manual)

    void ResetBoth()
    {
        delete ALTCAMP;
        ALTCAMP = new CAltCamp();
        delete CURCAMP;
        CURCAMP = new CCampCmd();
    }

    void ResetCamp(bool bOutput)
    {
        ALTCAMP->Update(CURCAMP);
        NewCamp(bOutput);
    }

    void ResetPlayer(bool bOutput)
    {
        NewCamp(false);
        if (bOutput) OutputPC();
    }

    void NewCamp(bool bOutput)
    {
        if (ValidIngame() && MOVETO->On && Returning)
        {
            // kill active camp return
            EndPreviousCmd(true);
        }
        delete CURCAMP;
        CURCAMP   = new CCampCmd();
        VarReset();
        if (bOutput) Output();
    }

    void Activate(float fY, float fX)
    {
        if (CURCAMP->On && !CURCAMP->Pc)
        {
            ResetCamp(false);
        }
        else
        {
            NewCamp(false);
        }
        CURCAMP->On = true;
        CURCAMP->Y  = fY;
        CURCAMP->X  = fX;
        Validate(); // CMUDelay
    }

    void ActivatePC(PlayerClient* pCPlayer)
    {
        Activate(pCPlayer->Y, pCPlayer->X);
        sprintf_s(CURCAMP->PcName, "%s", pCPlayer->DisplayedName);
        CURCAMP->Pc     = true;
        CURCAMP->PcID   = pCPlayer->SpawnID;
        CURCAMP->PcType = GetSpawnType(pCPlayer);
    }

    void VarReset()
    {
        Auto      = false;
        DoAlt     = false;
        DoReturn  = false;
        Returning = false;
    }

    CCampHandler()
    {
        ALTCAMP   = new CAltCamp();
        CURCAMP   = new CCampCmd();
        Min       = SET_CAMP->Min;
        Max       = SET_CAMP->Max;
        VarReset();
    }

    ~CCampHandler()
    {
        delete ALTCAMP;
        ALTCAMP = NULL;
        delete CURCAMP;
        CURCAMP = NULL;
    }

protected:
    void Output()
    {
        sprintf_s(szMsg, "\ay%s\aw:: MakeCamp off.", PLUGIN_NAME);
        WriteLine(szMsg, V_MAKECAMPV);
    }

    void OutputPC()
    {
        sprintf_s(szMsg, "\ay%s\aw:: MakeCamp player off.", PLUGIN_NAME);
        WriteLine(szMsg, V_MAKECAMPV);
    }
};

class CSnaproll : public CMULoc
{
public:
    float Head;    // heading to snaproll
    float Bearing; // bearing of target to snaproll to

    CSnaproll()
    {
        Head    = 0.0f;
        Bearing = HEADING_HALF;
    }
};

class CStickCmd : public CMULoc, public CStickSettings
{
public:
    CSnaproll* Snap;  // instance a snaproll
    // active stick settings
    bool  SetDist;    // has dist to mob has been set
    float Dist;       // defaults to melee range or set to 10 by /stick 10 for example
    float RandMin;    // min arc for strafe (if randomize enabled)
    float RandMax;    // max arc for strafe (if randomize enabled)
    bool  RandFlag;   // flag to randomly randomize min or max
    // active stick type
    bool  MoveBack;   // maintains distance (if too close, back up)
    bool  Behind;     // uses rear arc of target
    bool  BehindOnce; // moves to rear arc of target then position is ignored
    bool  Front;      // uses front arc of target
    bool  NotFront;   // uses anywhere but the front arc of target (/stick !front)
    bool  Pin;        // uses left or right arc of the target
    bool  Snaproll;   // snaprolls to a fixed loc (polarmath) instead of strafing
    bool  Hold;       // maintains sticking to previous target if it changes
    bool  Healer;     // healer type (non-constant facing)
    bool  Always;     // restick when new target acquired (wont work with hold) (/stick __ always)
    bool  HaveTarget; // true when Always true and have a target
    bool  Strafe;     // true when using a cmd that will strafe other than 'front'
    bool  On;         // stick active or off, set this only through TurnOn()
    // spawn information
    unsigned long theLastID = 0;    // compare target change OnPulse
    unsigned long HoldID = 0;    // spawn id for stick hold
    eSpawnType    LastType;  // if target changes to a corpse, stop sticking
    eSpawnType    HoldType;  // if stick hold changes to a corpse, stop sticking

    void TurnOn()
    {
        On       = true;
        bStickOn = true; // mq2melee support until setup valid/updated pointer to the class member
    }

    void StopHold()
    {
        HoldID   = NULL;
        HoldType = NONE;
        Hold     = false;
    }

    void FirstAlways()
    {
        // reset hold values, dont allow 'hold' or 'id' with 'always'
        StopHold();
        Always = true;
        TurnOn();
        if (pTarget)
        {
            HaveTarget = true;
            return;
        }
        HaveTarget = false;
    }

    void NewSnaproll()
    {
        delete Snap;
        Snap = new CSnaproll();
    }

    void ResetLoc()
    {
        Y = X = Z = 0.0f;
        CurDist = DifDist = 0.0f;
    }

    bool Ready()
    {
        if (Always)
        {
            return AlwaysStatus();
        }
        return On;
    }

    void DoRandomize()
    {
        if (!Randomize) return;
        if (NotFront)
        {
            SetRandArc(NOT_FRONT_ARC);
        }
        else if (Behind)
        {
            SetRandArc(BEHIND_ARC);
        }
        else if (Pin)
        {
            SetRandArc(PIN_ARC_MIN);
        }
    }

    CStickCmd()
    {
        Snap        = new CSnaproll();
        SetDist     = false;
        Dist        = 0.0f;
        RandMin     = 0.0f;
        RandMax     = 0.0f;
        RandFlag    = true;
        MoveBack    = false;
        Behind      = false;
        BehindOnce  = false;
        Front       = false;
        NotFront    = false;
        Pin         = false;
        Snaproll    = false;
        Hold        = false;
        Healer      = false;
        Always      = false;
        HaveTarget  = false;
        Strafe      = false;
        On          = false;
        theLastID      = 0;
        HoldID      = 0;
        LastType    = NONE;
        HoldType    = NONE;

        AlwaysReady = true;
        UserDefaults();
    }

    ~CStickCmd()
    {
        delete Snap;
        Snap = NULL;
    }

protected:
    void UserDefaults()
    {
        Min         = SET_S->Min;
        Max         = SET_S->Max;
        BreakGate   = SET_S->BreakGate;
        BreakHit    = SET_S->BreakHit;
        BreakTarget = SET_S->BreakTarget;
        BreakWarp   = SET_S->BreakWarp;
        PauseWarp   = SET_S->PauseWarp;
        Randomize   = SET_S->Randomize;
        DelayStrafe = SET_S->DelayStrafe;
        UseBack     = SET_S->UseBack;
        UseFleeing  = SET_S->UseFleeing;
        UW          = SET_S->UW;
        Walk        = SET_S->Walk;
        ArcBehind   = SET_S->ArcBehind;
        ArcNotFront = SET_S->ArcNotFront;
        DistBack    = SET_S->DistBack;
        DistBreak   = SET_S->DistBreak;
        DistMod     = SET_S->DistMod;
        DistModP    = SET_S->DistModP;
        DistSnap    = SET_S->DistSnap;
    }

    void SetRandArc(int iArcType)
    {
        float  fTempArc    = 0.0f;
        float  fArcSize    = 0.0f;
        float* pfRandomArc = NULL;
        float* pfStableArc = NULL;

        RandFlag = !RandFlag;
        if (RandFlag)
        {
            pfRandomArc = &RandMin;
            pfStableArc = &RandMax;
        }
        else
        {
            pfRandomArc = &RandMax;
            pfStableArc = &RandMin;
        }

        switch (iArcType)
        {
        case PIN_ARC_MIN: // 112 to 144  ---  144 - 112 = 32 total size
            fTempArc = (float)(rand() % 32 + 5);
            fArcSize = (float)(rand() % 32 + 16);
            break;
        case BEHIND_ARC:
            fTempArc = (float)(rand() % 45 + 5);
            fArcSize = (float)(rand() % 90 + 40);
            break;
        case NOT_FRONT_ARC:
            fTempArc = (float)(rand() % 135 + 5);
            fArcSize = (float)(rand() % 270 + 80);
            break;
        }

        *pfRandomArc = fTempArc;
        *pfStableArc = fArcSize;

        sprintf_s(szMsg, "\ay%s\aw:: Arcs Randomized! Max: %.2f  Min: %.2f", PLUGIN_NAME, RandMax, RandMin);
        WriteLine(szMsg, V_RANDOMIZE);
    }

    bool AlwaysStatus();
    bool AlwaysReady;
};

// ------------------------------------------
class CPauseHandler : public CMUDelay
{
public:
    bool PausedCmd; // pauses via command (not overwritten until 'unpause' or off/new cmd)
    bool PausedMU;  // pauses all operations (used by many operations)
    bool UserKB;    // true while movement keybinds are pressed
    bool UserMouse; // true while mouselook held

    bool Waiting()
    {
        if (PausedCmd || PausedMU) return true;
        return false;
    }

    void HandlePause()
    {
        if (PausedCmd) return;
        if (MouseCheck()) return;
        MouseFree();
        PauseTimers();
    }

    void PauseTimers()
    {
        if (PausedMU && !UserKB)
        {
            // verify we didnt ditch our stick target
            if (STICK->On && (!STICK->Always || (STICK->Always && STICK->HaveTarget)))
            {
                PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget);
                if (!psTarget || (STICK->Hold && STICK->HoldType != GetSpawnType(psTarget)))
                {
                    Reset();
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: You are no longer sticking to anything.", PLUGIN_NAME);
                    WriteLine(szMsg, V_STICKV);
                    return;
                }
            }
            switch(TimeStatus())
            {
            case T_INACTIVE:
            case T_WAITING:
                break;
            case T_READY:
            default:
                PausedMU = false;
                if (!CAMP->Auto)
                {
                    sprintf_s(szMsg, "\ay%s\aw:: Resuming previous command from movement pause.", PLUGIN_NAME);
                    WriteLine(szMsg, V_MOVEPAUSE);
                }
                Reset();
                break;
            }
        }
    }

    void MouseFree()
    {
        if (HandleMouse)
        {
            HandleMouse = false;
            if (PauseNeeded())
            {
                TimeStart();
            }
        }
    }

    bool PauseNeeded()
    {
        if (CIRCLE->On || STICK->On || MOVETO->On || CAMP->Auto)
        {
            if (STICK->On && (!STICK->Always || (STICK->Always && STICK->HaveTarget)))
            {
                PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget);
                if (!psTarget || (STICK->Hold && STICK->HoldType != GetSpawnType(psTarget)))
                {
                    Reset();
                    if (!UserKB) PausedMU = false;
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: You are no longer sticking to anything.", PLUGIN_NAME);
                    WriteLine(szMsg, V_STICKV);
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    bool MouseCheck();
    void Reset();

    CPauseHandler()
    {
        Min         = 500;
        Max         = 5000;
        PausedCmd   = PausedMU  = false;
        UserKB      = UserMouse = false;
        HandleMouse = HandleKB  = false;
    }
protected:
    bool HandleMouse;
    bool HandleKB;
};

class CMUActive
{
    // this class instances everything to be used during plugin
    // runtime under pMU. cmd instances are deleted and reinstanced
    // per use, copying their defaults from SET, inline params applied after
public:
    bool  Aggro;         // ${MoveUtils.Aggro}
    bool  Bard;          // true if a bard (used by autopause)
    bool  BrokeGM;       // true if BreakOnGM fired
    bool  BrokeSummon;   // true if BreakOnSummon fired
    bool  CmdFwd;        // awareness if plugin has attempted to move forward
    bool  CmdStrafe;     // awareness if plugin has attempted to strafe
    bool  FixWalk;       // true if we are dead, unwalk in the event we rez
    bool  Keybinds;      // loaded keybinds or not
    bool  KeyKilled;     // keybind has stopped movement
    bool  Loaded;        // auto ini load
    bool  LockPause;     // pause locked until unpause
    bool  MovetoBroke;   // Moveto BreakOnHit or BreakOnAggro fired
    bool  StickBroke;    // Stick BreakOnHit fired
    bool  Rooted;        // rootme command active
    bool  StoppedMoveto; // MoveTo.Stopped, last cmd arrived success
    int   Head;          // current heading type (0=fast, 1=loose, 2=true)

    void NewStick()
    {
        CURCAMP->RedoStick = false;
        bStickOn = false;
        delete STICK;
        STICK = new CStickCmd();
    }

    void NewMoveTo()
    {
        CAMP->VarReset();
        delete MOVETO;
        MOVETO = new CMoveToCmd();
    }

    void NewCircle()
    {
        CURCAMP->RedoCircle = false;
        delete CIRCLE;
        CIRCLE = new CCircleCmd();
    }

    void NewCmds()
    {
        NewStick();
        NewMoveTo();
        NewCircle();
    }

    void NewSummon()
    {
        delete SUMMON;
        SUMMON = new CMULoc();
    }

    void NewDefaults()
    {
        NewCmds();
        NewSummon();
        STUCK->Reset();
        CAMP->ResetBoth();
        Defaults();
    }

    bool Active()
    {
        if (STICK->On || MOVETO->On || CIRCLE->On || CAMP->Returning)
        {
            return true;
        }
        return false;
    }

    void AggroTLO();

    bool Broken()
    {
        if (BrokeGM || BrokeSummon) return true;
        return false;
    }

    CMUActive()
    {
        STICK  = new CStickCmd();
        MOVETO = new CMoveToCmd();
        CIRCLE = new CCircleCmd();
        CAMP   = new CCampHandler();
        PAUSE  = new CPauseHandler();
        STUCK  = new CStuckLogic();
        SUMMON = new CMULoc();

        Aggro         = false;
        Bard          = false;
        BrokeGM       = false;
        BrokeSummon   = false;
        CmdFwd        = false;
        CmdStrafe     = false;
        FixWalk       = false;
        Keybinds      = false;
        KeyKilled     = false;
        Loaded        = false;
        LockPause     = false;
        MovetoBroke   = false;
        Rooted        = false;
        StickBroke    = false;
        StoppedMoveto = false;
        Head          = H_TRUE;
        Defaults();
    }

    ~CMUActive()
    {
        delete STICK;
        STICK  = NULL;
        delete MOVETO;
        MOVETO = NULL;
        delete CIRCLE;
        CIRCLE = NULL;
        delete CAMP;
        CAMP   = NULL;
        delete PAUSE;
        PAUSE  = NULL;
        delete STUCK;
        STUCK  = NULL;
        delete SUMMON;
        SUMMON = NULL;
    }

    void Defaults()
    {
        Head      = SET->Head;
        LockPause = SET->LockPause;
    }
};

// ----------------------------------------
// movement & heading handling class
class CMUMovement
{
public:
    float ChangeHead; // heading changes set to this
    float RootHead;   // lock rooted head to this

    void AutoHead()   // called OnPulse to adjust heading for loose/true
    {
        if (ChangeHead == H_INACTIVE) return;
        TurnHead(ChangeHead);
    }

    void NewHead(float fNewHead)
    {
        // this is called to apply new heading changes
        // if loose/true, set ChangeHead to be adjusted appropriately OnPulse
        // if fast, apply change immediately
        if (!ValidIngame()) return;
        switch (pMU->Head)
        {
        case H_LOOSE:
        case H_TRUE:
            ChangeHead = fNewHead;
            break;
        case H_FAST:
        default:
            FastTurn(fNewHead);
            break;
        }
    }

    void NewFace(double dNewFace)
    {
        // set look angle adjustments (uw param & autouw)
        // loose/true: let mq2's gLookAngle auto-adjust for us
        // fast: apply change immediately
        if (!ValidIngame()) return;
        switch (pMU->Head)
        {
        case H_LOOSE:
        case H_TRUE:
            gLookAngle = dNewFace;
            break;
        case H_FAST:
        default:
            pCharSpawn->CameraAngle = (float)dNewFace;
            break;
        }
    }

    void StopHeading()
    {
        // if loose/true is currently trying to adjust head
        // set adjustment to current heading to halt it
        // we do it this way because true heading holds down the turn keys
        if (ChangeHead != H_INACTIVE)
        {
            TurnHead(pCharSpawn->Heading);
        }
    }

    float SaneHead(float fHeading)
    {
        // places new heading adjustments within bounds
        if (fHeading >= HEADING_MAX) fHeading -= HEADING_MAX;
        if (fHeading < 0.0f)         fHeading += HEADING_MAX;
        return fHeading;
    }

    void DoRoot()
    {
        // turns '/rootme' on
        if (!pMU->Rooted || !ValidIngame())
            return;

        if (SET->WinEQ)
        {
            StopRoot();
            return;
        }
        ChangeHead = H_INACTIVE;
        pLocalPlayer->Heading = RootHead;
        pLocalPlayer->SpeedHeading = 0.0f;
        pKeypressHandler->CommandState[CMD_LEFT] = 0;
        pEverQuestInfo->keyDown[CMD_LEFT] = 0;
        pKeypressHandler->CommandState[CMD_RIGHT] = 0;
        pEverQuestInfo->keyDown[CMD_RIGHT] = 0;
        TrueMoveOff(APPLY_TO_ALL);
    }

    void StopRoot()
    {
        if (!pMU->Rooted)
            return;
        // turns '/rootme' off
        pMU->Rooted = false;
        RootHead    = 0.0f;
        char szTempOut[MAX_STRING] = {};
        sprintf_s(szTempOut, "\ay%s\aw:: You are no longer rooted.", PLUGIN_NAME);
        WriteLine(szTempOut, V_SILENCE);
    }

    float AngDist(float fH1, float fH2)
    {
        // calculates current angular heading distance between two headings
        // used primarily by strafing stick cmds
        if(fabs(fH1 - fH2) > HEADING_HALF)
        {
            (fH1 < fH2 ? fH1 : fH2) += HEADING_MAX;
        }
        return (fabs(fH1 - fH2) > HEADING_HALF) ? (fH2 - fH1) : (fH1 - fH2);
    }

    bool CanMove(float fHead, float fY, float fX)
    {
        // anti-orbit formula credit: deadchicken
        // should always return true for fast heading
        // loose/true heading will not allow movement until
        // within a close enough turn to prevent orbiting around
        // a destination or running away from a close destination
        if (!ValidIngame()) return false;
        PlayerClient* pChSpawn = pCharSpawn;
        float fHeadDiff = fabs(pChSpawn->Heading - fHead);

        if (fHeadDiff > SET->AllowMove)
        {
            // if we are more than an 1/8th turn
            return false;
        }

        if ((fHeadDiff / 2.0f) > fabs(GetDistance(pChSpawn->Y, pChSpawn->X, fY, fX)))
        {
            // if half our needed adjustment is > distance between us and destination
            return false;
        }
        // else safe to move
        return true;
    }

    void SetWalk(bool bWalkOn)
    {
        // turns walking on or off when desired (if appropriate)
        if (!ValidIngame()) return; // ExecuteCmd in any other state = CTD

        bool bWalking = (*EQADDR_RUNWALKSTATE) ? false : true;
        if (pCharSpawn->SpeedMultiplier < 0.0f || pCharSpawn->RunSpeed < 0.0f)
        {
            //if negative speed, we are snared, and do not want walk on
            bWalkOn = false;
        }
        if (bWalkOn != bWalking)
        {
            ExecuteCmd(CMD_RUN_WALK, 1, 0);
            ExecuteCmd(CMD_RUN_WALK, 0, 0);
        }
    }

    void DoStand()
    {
        // stand up when desired (if appropriate)
        // feignsupport is handled *here only*
        if (!ValidIngame()) return;
        PlayerClient* pChSpawn = pCharSpawn;

        switch (pChSpawn->StandState)
        {
        case STANDSTATE_SIT:
            EzCommand("/stand"); // fix for preventing sit/stand bug
            break;
        case STANDSTATE_FEIGN:
            if (SET->Feign)
            {
                sprintf_s(szMsg, "\ay%s\aw:: Not standing as you are currently Feign Death", PLUGIN_NAME);
                WriteLine(szMsg, V_FEIGN);
                break;
            }
            EzCommand("/stand");
            break;
        case STANDSTATE_DUCK:
            /*ExecuteCmd(iDuckKey, 1, 0);
            ExecuteCmd(iDuckKey, 0, 0);*/ // rare server desync can happen from doing it this way
            EzCommand("/stand");
            break;
        case STANDSTATE_STAND:
        case STANDSTATE_DEAD:
        case STANDSTATE_BIND:
        case STANDSTATE_CASTING:
            break;
        default:
            SpewDebug(DBG_MAIN, "StandIfNeeded():: no StandState matches for %d", pChSpawn->StandState);
            break;
        }
    }

    void Walk(unsigned char ucDirection)
    {
        // wrapper for calling movement with walking on
        DoMove(ucDirection, true, MU_WALKON);
    }

    void TryMove(unsigned char ucDirection, unsigned char ucWalk, float fHead, float fY, float fX)
    {
        // wrapper for calling movement with anti-orbit formula calculated
        DoMove(ucDirection, CanMove(fHead, fY, fX), ucWalk);
    }

    void DoMove(unsigned char ucDirection, bool bTurnOn = true, unsigned char ucWalk = MU_WALKOFF); // main movement function

    void StopMove(unsigned char ucDirection)
    {
        // wrapper for stopping movement
        // old or new style support handled by the class instead of function
        if (SET->WinEQ)
        {
            SimMoveOff(ucDirection);
            return;
        }
        TrueMoveOff(ucDirection);
    }

    void StickStrafe(unsigned char ucDirection)
    {
        if (STICK->DelayStrafe)
        {
            TimedStrafe(ucDirection);
            return;
        }
        DoMove(ucDirection, true, STICK->Walk ? MU_WALKON : MU_WALKIGNORE);
    }

    CMUMovement()
    {
        ChangeHead = H_INACTIVE;
        RootHead   = 0.0f;
    }

private:
    void TimedStrafe(unsigned char ucDirection)
    {
        // function to determine if we can begin strafing based on
        // called if delaystrafe is on, calculates values via
        // CMUDelay inherit functions using Min/Max
        unsigned char ucResult = STICK->TimeStatus();
        if (!pMU->CmdStrafe && ucResult == T_INACTIVE)
        {
            StopMove(KILL_STRAFE);
            STICK->TimeStart();
            return; // return if we are start moving, not continue moving
        }
        if (ucResult == T_READY)
        {
            DoMove(ucDirection, true, STICK->Walk ? MU_WALKON : MU_WALKIGNORE);
        }
    }

    void TurnHead(float fHeading)
    {
        // this is called by AutoHead if ChangeHead has a value
        // do a loose or true heading adjustment determined here
        if (!ValidIngame()) return;
        switch(pMU->Head)
        {
        case H_LOOSE:
            LooseTurn(fHeading);
            break;
        case H_TRUE:
            if (SET->WinEQ)
            {
                LooseTurn(fHeading);
                break;
            }
            TrueTurn(fHeading);
            break;
        default:
            break;
        }
    }

    void FastTurn(float fNewHead)
    {
        if (!ValidIngame()) return;
        gFaceAngle = H_INACTIVE;
        pCharSpawn->Heading = fNewHead;
    }

    void LooseTurn(float fNewHead)
    {
        if (!ValidIngame()) return;
        gFaceAngle = H_INACTIVE;
        PlayerClient* pChSpawn = pCharSpawn;
        if (fabs(pChSpawn->Heading - fNewHead) < SET->TurnRate)
        {
            // if we are within one turn away, set heading to desired heading
            pChSpawn->Heading      = fNewHead;
            pChSpawn->SpeedHeading = 0.0f;
            ChangeHead             = H_INACTIVE;
        }
        else
        {
            float fCompHead = pChSpawn->Heading + HEADING_HALF;

            if (fNewHead < pChSpawn->Heading) fNewHead += HEADING_MAX;
            if (fNewHead < fCompHead)
            {
                pChSpawn->Heading      = SaneHead(pChSpawn->Heading + SET->TurnRate);
                pChSpawn->SpeedHeading = 12.0f;
            }
            else
            {
                pChSpawn->Heading      = SaneHead(pChSpawn->Heading - SET->TurnRate);
                pChSpawn->SpeedHeading = -12.0f;
            }
        }
    }

    void TrueTurn(float fNewHead)
    {
        if (!ValidIngame()) return;
        gFaceAngle = H_INACTIVE;
        PlayerClient* pChSpawn = pCharSpawn;
        if (fabs(pChSpawn->Heading - fNewHead) < 14.0f)
        {
            pKeypressHandler->CommandState[CMD_LEFT] = 0;
            pEverQuestInfo->keyDown[CMD_LEFT] = 0;
            pKeypressHandler->CommandState[CMD_RIGHT] = 0;
            pEverQuestInfo->keyDown[CMD_RIGHT] = 0;
            pChSpawn->Heading = fNewHead;
            pChSpawn->SpeedHeading = 0.0f;
            ChangeHead = H_INACTIVE;
        }
        else
        {
            float fCompHead = pChSpawn->Heading + HEADING_HALF;

            if (fNewHead < pChSpawn->Heading) fNewHead += HEADING_MAX;
            if (fNewHead < fCompHead)
            {
                pKeypressHandler->CommandState[CMD_RIGHT] = 0;
                pEverQuestInfo->keyDown[CMD_RIGHT] = 0;
                pKeypressHandler->CommandState[CMD_LEFT]  = 1;
                pEverQuestInfo->keyDown[CMD_LEFT] = 1;
            }
            else
            {
                pKeypressHandler->CommandState[CMD_LEFT]  = 0;
                pEverQuestInfo->keyDown[CMD_LEFT] = 0;
                pKeypressHandler->CommandState[CMD_RIGHT] = 1;
                pEverQuestInfo->keyDown[CMD_RIGHT] = 1;
            }
        }
    }

    void TrueMoveOn(unsigned char ucDirection)
    {
        switch(ucDirection)
        {
        case GO_FORWARD:
            pMU->CmdFwd = true;
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_BACK] = 0;
            pEverQuestInfo->keyDown[CMD_BACK] = 0;
            pKeypressHandler->CommandState[CMD_FORWARD] = 1;
            pEverQuestInfo->keyDown[CMD_FORWARD] = 1;
            break;
        case GO_BACKWARD:
            pMU->CmdFwd = false;
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_FORWARD] = 0;
            pEverQuestInfo->keyDown[CMD_FORWARD] = 0;
            pKeypressHandler->CommandState[CMD_BACK] = 1;
            pEverQuestInfo->keyDown[CMD_BACK] = 1;
            break;
        case GO_LEFT:
            pMU->CmdStrafe = true;
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_RIGHT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT] = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_LEFT] = 1;
            pEverQuestInfo->keyDown[CMD_STRAFE_LEFT] = 1;
            break;
        case GO_RIGHT:
            pMU->CmdStrafe = true;
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_LEFT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_LEFT] = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_RIGHT] = 1;
            pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT] = 1;
            break;
        }
    }

    void TrueMoveOff(unsigned char ucDirection)
    {
        switch(ucDirection)
        {
        case APPLY_TO_ALL:
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_LEFT]  = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_LEFT] = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_RIGHT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT] = 0;
            pKeypressHandler->CommandState[CMD_FORWARD] = 0;
            pEverQuestInfo->keyDown[CMD_FORWARD] = 0;
            pKeypressHandler->CommandState[CMD_BACK] = 0;
            pEverQuestInfo->keyDown[CMD_BACK] = 0;
            pMU->CmdFwd = pMU->CmdStrafe = false;
            STICK->TimeStop();
            break;
        case GO_FORWARD:
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_FORWARD] = 0;
            pEverQuestInfo->keyDown[CMD_FORWARD] = 0;
            pMU->CmdFwd = false;
            break;
        case GO_BACKWARD:
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_BACK] = 0;
            pEverQuestInfo->keyDown[CMD_BACK] = 0;
            pMU->CmdFwd = false;
            break;
        case GO_LEFT:
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_LEFT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_LEFT] = 0;
            pMU->CmdStrafe = false;
            STICK->TimeStop();
            break;
        case GO_RIGHT:
            pKeypressHandler->CommandState[CMD_AUTORUN] = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_RIGHT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT] = 0;
            pMU->CmdStrafe = false;
            STICK->TimeStop();
            break;
        case KILL_STRAFE:
            pKeypressHandler->CommandState[CMD_STRAFE_LEFT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_LEFT] = 0;
            pKeypressHandler->CommandState[CMD_STRAFE_RIGHT] = 0;
            pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT] = 0;
            pMU->CmdStrafe = false;
            STICK->TimeStop();
            break;
        case KILL_FB:
            pKeypressHandler->CommandState[CMD_AUTORUN]  = 0;
            pEverQuestInfo->AutoRun = 0;
            pKeypressHandler->CommandState[CMD_FORWARD]  = 0;
            pEverQuestInfo->keyDown[CMD_FORWARD] = 0;
            pKeypressHandler->CommandState[CMD_BACK] = 0;
            pEverQuestInfo->keyDown[CMD_BACK] = 0;
            pMU->CmdFwd = false;
            break;
        }
    }

    void SimMoveOn(unsigned char ucDirection)
    {
        switch (ucDirection)
        {
        case GO_FORWARD:
            pMU->CmdFwd = true;
            ExecuteCmd(CMD_BACK, 0, 0);
            ExecuteCmd(CMD_FORWARD, 1, 0);
            break;
        case GO_BACKWARD:
            pMU->CmdFwd = false;
            ExecuteCmd(CMD_FORWARD, 0, 0);
            ExecuteCmd(CMD_BACK, 1, 0);
            break;
        case GO_LEFT:
            pMU->CmdStrafe = true;
            ExecuteCmd(CMD_STRAFE_RIGHT, 0, 0);
            ExecuteCmd(CMD_STRAFE_LEFT, 1, 0);
            break;
        case GO_RIGHT:
            pMU->CmdStrafe = true;
            ExecuteCmd(CMD_STRAFE_LEFT, 0, 0);
            ExecuteCmd(CMD_STRAFE_RIGHT, 1, 0);
            break;
        }
    }

    void SimMoveOff(unsigned char ucDirection)
    {
        switch (ucDirection)
        {
        case APPLY_TO_ALL:
            ExecuteCmd(CMD_FORWARD, 0, 0);
            ExecuteCmd(CMD_BACK, 1, 0);
            ExecuteCmd(CMD_BACK, 0, 0);
            ExecuteCmd(CMD_STRAFE_LEFT, 0, 0);
            ExecuteCmd(CMD_STRAFE_RIGHT, 1, 0);
            ExecuteCmd(CMD_STRAFE_RIGHT, 0, 0);
            pMU->CmdFwd = pMU->CmdStrafe = false;
            break;
        case KILL_FB:
        case GO_FORWARD:
        case GO_BACKWARD:
            ExecuteCmd(CMD_FORWARD, 1, 0);
            ExecuteCmd(CMD_FORWARD, 0, 0);
            ExecuteCmd(CMD_BACK, 1, 0);
            ExecuteCmd(CMD_BACK, 0, 0);
            pMU->CmdFwd = false;
            break;
        case KILL_STRAFE:
        case GO_LEFT:
        case GO_RIGHT:
            ExecuteCmd(CMD_STRAFE_RIGHT, 1, 0);
            ExecuteCmd(CMD_STRAFE_RIGHT, 0, 0);
            ExecuteCmd(CMD_STRAFE_LEFT,  1, 0);
            ExecuteCmd(CMD_STRAFE_LEFT,  0, 0);
            pMU->CmdStrafe = false;
            break;
        }
    }
};

//--------------------------------------------------------------
// class methods that are gayed because of declaration placement

void CMUMovement::DoMove(unsigned char ucDirection, bool bTurnOn, unsigned char ucWalk)
{
    // this one due to prototype

    switch(ucWalk)
    {
    case MU_WALKON:
        SetWalk(true);
        break;
    case MU_WALKOFF:
        SetWalk(false);
        break;
    case MU_WALKIGNORE:
        break;
    }

    if (SET->WinEQ)
    {
        if (bTurnOn)
        {
            SimMoveOn(ucDirection);
            return;
        }
        SimMoveOff(ucDirection);
        return;
    }
    if (bTurnOn)
    {
        TrueMoveOn(ucDirection);
        return;
    }
    TrueMoveOff(ucDirection);
}

bool CMoveToCmd::DidAggro()
{
    if (!CURCAMP->On && On && ME->InCombat())
    {
        pMU->MovetoBroke = true;
        EndPreviousCmd(true);
        sprintf_s(szMsg, "\ay%s\aw:: Aggro gained during /moveto, Halting command...", PLUGIN_NAME);
        WriteLine(szMsg, V_BREAKONAGGRO);
        return true;
    }
    return false;
}

bool CStickCmd::AlwaysStatus()
{
    if (!pTarget || pTarget->Type != SPAWN_NPC)
    {
        if (AlwaysReady)
        {
            sprintf_s(szMsg, "\ay%s\aw:: Stick awaiting next valid NPC target...", PLUGIN_NAME);
            WriteLine(szMsg, V_STICKALWAYS);
            MOVE->StopMove(APPLY_TO_ALL);
            DoRandomize();
            AlwaysReady = false;
        }
        HaveTarget = false;
        return false;
    }

    if (!AlwaysReady)
    {
        EndPreviousCmd(true, CMD_STICK, true);
        PAUSE->Reset();
        MOVE->DoStand();
        MOVE->StopHeading();
        AlwaysReady = true;
    }
    HaveTarget = true;
    return true;
}

bool CPauseHandler::MouseCheck()
{
    if (*pMouseLook)
    {
        UserMouse = true;
        if ((SET->PauseMouse || SET->BreakMouse) && PauseNeeded())
        {
            CAMP->TimeStop();
            TimeStop();
            MOVE->StopHeading();
            if (SET->BreakMouse)
            {
                EndPreviousCmd(true);
                if (!CAMP->Auto)
                {
                    sprintf_s(szMsg, "\ay%s\aw:: Current command ended from mouse movement.", PLUGIN_NAME);
                    WriteLine(szMsg, V_MOUSEPAUSE);
                }
                return false;
            }
            PausedMU = true;
            MOVE->DoMove(APPLY_TO_ALL, false, MU_WALKOFF);
            HandleMouse = true;
            return true;
        }
        return false;
    }
    UserMouse = false;
    return false;
}

void CPauseHandler::Reset()
{
    CAMP->TimeStop();
    STUCK->Reset();
    pMU->CmdFwd    = false;
    pMU->CmdStrafe = false;
    pMU->NewSummon();
    STICK->NewSnaproll();
    STICK->TimeStop();
    STICK->ResetLoc();
}

void CMUActive::AggroTLO()
{
    if (pTarget)
    {
        if (fabs(MOVE->AngDist(pTarget->Heading, pCharSpawn->Heading)) > 190.0f)
        {
            Aggro = true;
            return;
        }
    }
    Aggro = false;
}

// ----------------------------------------
// custom UI window
class CMoveUtilsWnd : public CCustomWnd
{
public:
	CStmlWnd* OutWnd;

    CMoveUtilsWnd(const CXStr& Template) : CCustomWnd(Template)
    {
        OutWnd = (CStmlWnd*)GetChildItem("CW_ChatOutput");
        OutWnd->SetClickThrough(true);
        SetEscapable(false);
        OutWnd->MaxLines = 0x190;
        AddStyle(CWS_TITLE | CWS_MINIMIZE | CWS_RESIZEBORDER);
        RemoveStyle(CWS_TRANSPARENT | CWS_CLOSE);
    }

    virtual int WndNotification(CXWnd* pWnd, unsigned int uiMessage, void* pData) override
    {
        if (pWnd == nullptr && uiMessage == XWM_CLOSE)
        {
            SetVisible(true);
            return 0;
        }
        return CSidlScreenWnd::WndNotification(pWnd, uiMessage, pData);
    }

    void Clear()
    {
        if (OutWnd)
        {
            OutWnd->SetSTMLText("", false);
            OutWnd->ForceParseNow();
            OutWnd->SetVScrollPos(OutWnd->GetVScrollMax());
        }
    }
};

class CMUWndHandler
{
public:
    bool Active()
    {
        if (OurWnd) return true;
        return false;
    }

    void Create()
    {
        if (!ValidIngame() || !SET->Window || OurWnd) return;
        NewWnd();
    }

    void Destroy(bool bSave)
    {
        if (!OurWnd) return;
        if (bSave) SaveWnd();
        delete OurWnd;
        OurWnd = NULL;
    }

    void Clear()
    {
        if (!OurWnd) return;
        OurWnd->Clear();
    }

    void Hover()
    {
        if (!OurWnd) return;
        OurWnd->DoAllDrawing();
    }

    void Min()
    {
        if (!OurWnd) return;
        OurWnd->OnMinimizeBox();
    }

    void Save()
    {
        if (!OurWnd) return;
        SaveWnd();
    }

    void Write(const char* szText)
    {
        if (!OurWnd) return;
        Output(szText);
    }

    void NewFont(int iSize)
    {
        if (!OurWnd || iSize < 0) return;
        SetFontSize(iSize);
    }

    CMUWndHandler()
    {
        OurWnd   = NULL;
        FontSize = 2;
    }
protected:
    void NewWnd()
    {
        char szWindowText[MAX_STRING] = {};
        OurWnd = new CMoveUtilsWnd(CXStr("ChatWindow"));
        //left top right bottom
        OurWnd->SetLocation({ (LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatLeft",     10,   INIFileName),
            (LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatTop",      10,   INIFileName),
            (LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatRight",    410,  INIFileName),
            (LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatBottom",   210,  INIFileName) });

        OurWnd->SetFades((GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Fades",  0,    INIFileName) ? true:false));
        OurWnd->SetAlpha(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Alpha",        255,  INIFileName));
        OurWnd->SetFadeToAlpha(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FadeToAlpha",  255,  INIFileName));
        OurWnd->SetFadeDuration(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Duration",     500,  INIFileName));
        OurWnd->SetLocked((GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Locked", 0,    INIFileName) ? true:false));
        OurWnd->SetFadeDelay(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Delay",        2000, INIFileName));
        OurWnd->SetBGType(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGType",       1,    INIFileName));
        ARGBCOLOR col = {};

        col.A       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.alpha",   255,  INIFileName);
        col.R       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.red",   0,  INIFileName);
        col.G       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.green", 0,  INIFileName);
        col.B       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.blue",  0,  INIFileName);
        OurWnd->SetBGColor(col.ARGB);

        NewFont(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FontSize", 2, INIFileName));
        GetPrivateProfileString(SET->SaveByChar ? szCharName : "Window", "WindowTitle", "MoveUtils", szWindowText, MAX_STRING, INIFileName);
        OurWnd->SetWindowText(szWindowText);
        //SetCXStr(&OurWnd->WindowText, szWindowText);
        OurWnd->Show(1, 1);
        OurWnd->OutWnd->RemoveStyle(CWS_CLOSE);
        //BitOff(OurWnd->OutStruct->WindowStyle, CWS_CLOSE);
    }

    void SaveWnd()
    {
        char szTemp[MAX_STRING] = {};

        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatTop",      OurWnd->GetLocation().top,    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatBottom",   OurWnd->GetLocation().bottom, INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatLeft",     OurWnd->GetLocation().left,   INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatRight",    OurWnd->GetLocation().right,  INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Fades",        OurWnd->GetFades(),           INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Alpha",        OurWnd->GetAlpha(),           INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FadeToAlpha",  OurWnd->GetFadeToAlpha(),     INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Duration",     OurWnd->GetFadeDuration(),    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Locked",       OurWnd->IsLocked(),           INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Delay",        OurWnd->GetFadeDelay(),       INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGType",       OurWnd->GetBGType(),          INIFileName);
        ARGBCOLOR col = { 0 };
        col.ARGB = OurWnd->GetBGColor();
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.alpha", col.A,    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.red",   col.R,    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.green", col.G,    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.blue",  col.B,    INIFileName);
        WritePrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FontSize",     FontSize, INIFileName);

        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "WindowTitle", OurWnd->GetWindowText().c_str(), INIFileName);
    }

    void Output(const char* szText)
    {
        OurWnd->Show(true);
        char szProcessed[MAX_STRING] = {};
        StripMQChat(szText, szProcessed);
        CheckChatForEvent(szProcessed);
        MQToSTML(szText, szProcessed, MAX_STRING);
        strcat_s(szProcessed, "<br>");
        OurWnd->OutWnd->AppendSTML(szProcessed);
        OurWnd->OutWnd->SetVScrollPos(OurWnd->OutWnd->GetVScrollMax());
    }

    void SetFontSize(int uiSize)
    {
        if (uiSize < 0 || uiSize > pWndMgr->FontsArray.GetCount())
            return;

        CXStr ContStr(OurWnd->OutWnd->GetSTMLText());
        OurWnd->OutWnd->SetFont(pWndMgr->FontsArray[uiSize]);
        OurWnd->OutWnd->SetSTMLText(ContStr, false);
        OurWnd->OutWnd->ForceParseNow();
        OurWnd->OutWnd->SetVScrollPos(OurWnd->OutWnd->GetVScrollMax());

        FontSize = uiSize;
    }

    CMoveUtilsWnd* OurWnd;
    unsigned long FontSize;
};

// --------------------------------
// Begin Custom Top-Level Objects

class MQ2MakeCampType*   pMakeCampType   = nullptr;
class MQ2StickType*      pStickType      = nullptr;
class MQ2MoveToType*     pMoveToType     = nullptr;
class MQ2CircleType*     pCircleType     = nullptr;
class MQ2MoveUtilsType*  pMoveUtilsType  = nullptr;

class MQ2MakeCampType : public MQ2Type
{
public:
    enum MakeCampMembers
    {
        Status = 1,
        Leash,
        AnchorY,
        AnchorX,
        LeashLength,
        CampRadius,
        MinDelay,
        MaxDelay,
        Returning,
        AltAnchorY,
        AltAnchorX,
        CampDist,
        AltCampDist,
        AltRadius,
        Scatter,
        ReturnNoAggro,
        ReturnNotLooting,
        ReturnHaveTarget,
        Bearing,
        ScatDist,
        ScatSize,
    };

    MQ2MakeCampType() : MQ2Type("makecamp")
    {
        TypeMember(Status);
        TypeMember(Leash);
        TypeMember(AnchorY);
        TypeMember(AnchorX);
        TypeMember(LeashLength);
        TypeMember(CampRadius);
        TypeMember(MinDelay);
        TypeMember(MaxDelay);
        TypeMember(Returning);
        TypeMember(AltAnchorY);
        TypeMember(AltAnchorX);
        TypeMember(CampDist);
        TypeMember(AltCampDist);
        TypeMember(AltRadius);
        TypeMember(Scatter);
        TypeMember(ReturnNoAggro);
        TypeMember(ReturnNotLooting);
        TypeMember(ReturnHaveTarget);
        TypeMember(Bearing);
        TypeMember(ScatDist);
        TypeMember(ScatSize);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar &Dest) override
    {
        auto pMember = MQ2MakeCampType::FindMember(Member);
        if (!pMember || !ValidIngame(false)) return false;
        switch(static_cast<MakeCampMembers>(pMember->ID))
        {
        case Status:
            strcpy_s(DataTypeTemp, "OFF");
            if (PAUSE->PausedMU || PAUSE->PausedCmd)
            {
                strcpy_s(DataTypeTemp, "PAUSED");
            }
            else if (CURCAMP->On)
            {
                strcpy_s(DataTypeTemp, "ON");
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case Leash:
            Dest.DWord = CURCAMP->Leash;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case AnchorY:
            Dest.Float = CURCAMP->Y;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case AnchorX:
            Dest.Float = CURCAMP->X;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case LeashLength:
            Dest.Float = CURCAMP->Length;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case CampRadius:
            Dest.Float = CURCAMP->Radius;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case MinDelay:
            Dest.DWord = CAMP->Min;
            Dest.Type  = mq::datatypes::pIntType;
            return true;
        case MaxDelay:
            Dest.DWord = CAMP->Max;
            Dest.Type  = mq::datatypes::pIntType;
            return true;
        case Returning:
            Dest.DWord = CAMP->DoReturn;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case AltAnchorY:
            Dest.Float = ALTCAMP->Y;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case AltAnchorX:
            Dest.Float = ALTCAMP->X;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case CampDist:
            Dest.Float = 0.0f;
            if (CURCAMP->On)
            {
                PlayerClient* pLPlayer = pLocalPlayer;
                Dest.Float = GetDistance(pLPlayer->Y, pLPlayer->X, CURCAMP->Y, CURCAMP->X);
            }
            Dest.Type = mq::datatypes::pFloatType;
            return true;
        case AltCampDist:
            Dest.Float = 0.0f;
            if (ALTCAMP->On)
            {
                PlayerClient* pLPlayer = pLocalPlayer;
                Dest.Float = GetDistance(pLPlayer->Y, pLPlayer->X, ALTCAMP->Y, ALTCAMP->X);
            }
            Dest.Type = mq::datatypes::pFloatType;
            return true;
        case AltRadius:
            Dest.Float = ALTCAMP->Radius;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case Scatter:
            Dest.DWord = CURCAMP->Scatter;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case ReturnNoAggro:
            Dest.DWord = CURCAMP->NoAggro;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case ReturnNotLooting:
            Dest.DWord = CURCAMP->NotLoot;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case ReturnHaveTarget:
            Dest.DWord = CURCAMP->HaveTarget;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Bearing:
            Dest.Float = CURCAMP->Bearing;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case ScatDist:
            Dest.Float = CURCAMP->ScatDist;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case ScatSize:
            Dest.Float = CURCAMP->ScatSize;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        }
        return false;
    }

    bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        strcpy_s(Destination, MAX_STRING, "OFF");
        if (PAUSE->PausedMU || PAUSE->PausedCmd)
        {
            strcpy_s(Destination, MAX_STRING, "PAUSED");
        }
        else if (CURCAMP->On)
        {
            strcpy_s(Destination, MAX_STRING, "ON");
        }
        return true;
    }
};

bool dataMakeCamp(const char* szName, MQTypeVar &Ret)
{
    Ret.DWord = 1;
    Ret.Type  = pMakeCampType;
    return true;
}

class MQ2StickType : public MQ2Type
{
public:
    enum StickMembers
    {
        Status = 1,
        Active,
        Distance,
        MoveBehind,
        MoveBack,
        Loose,
        Paused,
        Behind,
        Stopped,
        Pin,
        StickTarget,
        StickTargetName,
        DistMod,
        DistModPercent,
        Always,
        Broken,
    };

    MQ2StickType() : MQ2Type("stick")
    {
        TypeMember(Status);
        TypeMember(Active);
        TypeMember(Distance);
        TypeMember(MoveBehind);
        TypeMember(MoveBack);
        TypeMember(Loose);
        TypeMember(Paused);
        TypeMember(Behind);
        TypeMember(Stopped);
        TypeMember(Pin);
        TypeMember(StickTarget);
        TypeMember(StickTargetName);
        TypeMember(DistMod);
        TypeMember(DistModPercent);
        TypeMember(Always);
        TypeMember(Broken);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
    {
        auto pMember = MQ2StickType::FindMember(Member);
        if (!pMember || !ValidIngame(false)) return false;
        switch(static_cast<StickMembers>(pMember->ID))
        {
        case Status:
            strcpy_s(DataTypeTemp, "OFF");
            if (PAUSE->PausedMU || PAUSE->PausedCmd)
            {
                strcpy_s(DataTypeTemp, "PAUSED");
            }
            else if (STICK->On)
            {
                strcpy_s(DataTypeTemp, "ON");
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case Active:
            Dest.DWord = STICK->On;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Distance:
            Dest.Float = STICK->Dist;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case MoveBehind:
            Dest.DWord = STICK->Behind;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case MoveBack:
            Dest.DWord = STICK->MoveBack;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Loose:
            Dest.DWord = (pMU->Head == H_LOOSE) ? true : false;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Paused:
            Dest.DWord = PAUSE->PausedMU;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Behind:
            Dest.DWord = false;
            if (PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget))
            {
                PlayerClient* pChSpawn = pCharSpawn;
                Dest.DWord = (fabs(GetDistance(pChSpawn, psTarget)) <= ((STICK->Dist > 0.0f ? STICK->Dist : (psTarget->StandState ? get_melee_range(pLocalPlayer, (PlayerClient*)psTarget) : 15.0f)) * STICK->DistModP + STICK->DistMod) && fabs(MOVE->AngDist(psTarget->Heading, pChSpawn->Heading)) <= STICK->ArcBehind) ? true : false;
            }
            Dest.Type = mq::datatypes::pBoolType;
            return true;
        case Stopped:
            Dest.DWord = false;
            if (PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget))
            {
                Dest.DWord = (fabs(GetDistance(pCharSpawn, psTarget)) <= STICK->Dist) ? true : false;
            }
            Dest.Type = mq::datatypes::pBoolType;
            return true;
        case Pin:
            Dest.DWord = STICK->Pin;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case StickTarget:
            Dest.Int = 0;
            if (PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget))
            {
                Dest.Int = psTarget->SpawnID;
            }
            Dest.Type = mq::datatypes::pIntType;
            return true;
        case StickTargetName:
            strcpy_s(DataTypeTemp, "NONE");
            if (PlayerClient* psTarget = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget))
            {
                strcpy_s(DataTypeTemp, psTarget->DisplayedName);
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case DistMod:
            Dest.Float = STICK->DistMod;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case DistModPercent:
            Dest.Float = STICK->DistModP;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case Always:
            Dest.DWord = STICK->Always;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Broken:
            Dest.DWord = pMU->StickBroke;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        }
        return false;
    }

    bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        strcpy_s(Destination, MAX_STRING, "OFF");
        if (PAUSE->PausedMU || PAUSE->PausedCmd)
        {
            strcpy_s(Destination, MAX_STRING, "PAUSED");
        }
        else if (STICK->On)
        {
            strcpy_s(Destination, MAX_STRING, "ON");
        }

        return true;
    }
};

bool dataStick(const char* szName, MQTypeVar &Ret)
{
    Ret.DWord = 1;
    Ret.Type  = pStickType;
    return true;
}

class MQ2MoveToType : public MQ2Type
{
public:
    enum MoveToMembers
    {
        Moving = 1,
        Stopped,
        CampStopped,
        UseWalk,
        ArrivalDist,
        ArrivalDistY,
        ArrivalDistX,
        Broken,
    };

    MQ2MoveToType() : MQ2Type("moveto")
    {
        TypeMember(Moving);
        TypeMember(Stopped);
        TypeMember(CampStopped);
        TypeMember(UseWalk);
        TypeMember(ArrivalDist);
        TypeMember(ArrivalDistY);
        TypeMember(ArrivalDistX);
        TypeMember(Broken);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar &Dest) override
    {
        auto pMember = MQ2MoveToType::FindMember(Member);
        if (!pMember || !ValidIngame(false)) return false;
        switch(static_cast<MoveToMembers>(pMember->ID))
        {
        case Moving:
            Dest.DWord = MOVETO->On;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Stopped:
            /*if (pLocalPlayer)
            {
                Dest.DWord = (fabs(GetDistance(((PlayerClient*)pCharSpawn)->Y, ((PlayerClient*)pCharSpawn)->X, MOVETO->Y, MOVETO->X)) <= MOVETO->Dist) ? true : false;
            }*/
            Dest.DWord = pMU->StoppedMoveto;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case CampStopped:
            Dest.DWord = false;
            if (pLocalPlayer)
            {
                Dest.DWord = (fabs(GetDistance(pCharSpawn->Y, pCharSpawn->X, CAMP->Y, CAMP->X)) <= MOVETO->Dist) ? true : false;
            }
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case UseWalk:
            Dest.DWord = MOVETO->Walk;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case ArrivalDist:
            Dest.Float = MOVETO->Dist;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case ArrivalDistY:
            Dest.Float = MOVETO->DistY;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case ArrivalDistX:
            Dest.Float = MOVETO->DistX;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case Broken:
            Dest.DWord = pMU->MovetoBroke;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        }
        return false;
    }

    virtual bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        strcpy_s(Destination, MAX_STRING, "OFF");
        if (PAUSE->PausedMU || PAUSE->PausedCmd)
        {
            strcpy_s(Destination, MAX_STRING, "PAUSED");
        }
        else if (MOVETO->On)
        {
            strcpy_s(Destination, MAX_STRING, "ON");
        }

        return true;
    }
};

bool dataMoveTo(const char* szName, MQTypeVar &Ret)
{
    Ret.DWord = 1;
    Ret.Type  = pMoveToType;
    return true;
}

class MQ2CircleType : public MQ2Type
{
public:
    enum CircleMembers
    {
        Status = 1,
        CircleY,
        CircleX,
        Drunken,
        Rotation,
        Direction,
        Clockwise,
        Backwards,
        Radius,
    };

    MQ2CircleType() : MQ2Type("circle")
    {
        TypeMember(Status);
        TypeMember(CircleY);
        TypeMember(CircleX);
        TypeMember(Drunken);
        TypeMember(Rotation);
        TypeMember(Direction);
        TypeMember(Clockwise);
        TypeMember(Backwards);
        TypeMember(Radius);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar &Dest) override
    {
        auto pMember = MQ2CircleType::FindMember(Member);
        if (!pMember || !ValidIngame(false)) return false;
        switch(static_cast<CircleMembers>(pMember->ID))
        {
        case Status:
            strcpy_s(DataTypeTemp, "OFF");
            if (PAUSE->PausedMU || PAUSE->PausedCmd)
            {
                strcpy_s(DataTypeTemp, "PAUSED");
            }
            else if (CIRCLE->On)
            {
                strcpy_s(DataTypeTemp, "ON");
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case CircleY:
            Dest.Float = CIRCLE->Y;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case CircleX:
            Dest.Float = CIRCLE->X;
            Dest.Type = mq::datatypes::pFloatType;
            return true;
        case Drunken:
            Dest.DWord = CIRCLE->Drunk;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Rotation:
            strcpy_s(DataTypeTemp, "CW");
            if (CIRCLE->CCW)
            {
                strcpy_s(DataTypeTemp, "CCW");
            }
            Dest.Ptr = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case Direction:
            strcpy_s(DataTypeTemp, "FORWARDS");
            if (CIRCLE->Backward)
            {
                strcpy_s(DataTypeTemp, "BACKWARDS");
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case Clockwise:
            Dest.DWord = !CIRCLE->CCW;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Backwards:
            Dest.DWord = CIRCLE->Backward;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Radius:
            Dest.Float = CIRCLE->Radius;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        }
        return false;
    }

    virtual bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        strcpy_s(Destination, MAX_STRING, "OFF");
        if (PAUSE->PausedMU || PAUSE->PausedCmd)
        {
            strcpy_s(Destination, MAX_STRING, "PAUSED");
        }
        else if (CIRCLE->On)
        {
            strcpy_s(Destination, MAX_STRING, "ON");
        }
        return true;
    }
};

bool dataCircling(const char* szName, MQTypeVar &Ret)
{
    Ret.DWord = 1;
    Ret.Type  = pCircleType;
    return true;
}

class MQ2MoveUtilsType : public MQ2Type
{
public:
    enum MoveUtilsMembers
    {
        Command = 1,
        Stuck,
        Summoned,
        StuckLogic,
        Verbosity,
        FullVerbosity,
        TotalSilence,
        Aggro,
        PauseMinDelay,
        PauseMaxDelay,
        PulseCheck,
        PulseUnstuck,
        TryToJump,
        DistStuck,
        Version,
        MovePause,
        GM,
    };

    MQ2MoveUtilsType() : MQ2Type("MoveUtils")
    {
        TypeMember(Command);
        TypeMember(Stuck);
        TypeMember(Summoned);
        TypeMember(StuckLogic);
        TypeMember(Verbosity);
        TypeMember(FullVerbosity);
        TypeMember(TotalSilence);
        TypeMember(Aggro);
        TypeMember(PauseMinDelay);
        TypeMember(PauseMaxDelay);
        TypeMember(PulseCheck);
        TypeMember(PulseUnstuck);
        TypeMember(TryToJump);
        TypeMember(DistStuck);
        TypeMember(Version);
        TypeMember(MovePause);
        TypeMember(GM);
    }

    virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar &Dest) override
    {
        auto pMember = MQ2MoveUtilsType::FindMember(Member);
        if (!pMember || !ValidIngame(false)) return false;
        switch(static_cast<MoveUtilsMembers>(pMember->ID))
        {
        case Command:
            strcpy_s(DataTypeTemp, "NONE");
            if (STICK->On)
            {
                strcpy_s(DataTypeTemp, "STICK");
            }
            else if (CIRCLE->On)
            {
                strcpy_s(DataTypeTemp, "CIRCLE");
            }
            else if (MOVETO->On)
            {
                strcpy_s(DataTypeTemp, "MOVETO");
            }
            else if (CURCAMP->On)
            {
                strcpy_s(DataTypeTemp, "MAKECAMP");
            }
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case Stuck:
            Dest.DWord = false;
            if (STUCK->StuckInc)
            {
                Dest.DWord = true;
            }
            Dest.Type = mq::datatypes::pBoolType;
            return true;
        case Summoned:
            Dest.DWord = pMU->BrokeSummon;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case StuckLogic:
            Dest.DWord = STUCK->On;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Verbosity:
            Dest.DWord = (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY ? true : false;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case FullVerbosity:
            Dest.DWord = (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY ? true : false;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case TotalSilence:
            Dest.DWord = (uiVerbLevel == V_SILENCE) ? true : false;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case Aggro:
            Dest.DWord = pMU->Aggro;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case PauseMinDelay:
            Dest.Int  = PAUSE->Min;
            Dest.Type = mq::datatypes::pIntType;
            return true;
        case PauseMaxDelay:
            Dest.Int  = PAUSE->Max;
            Dest.Type = mq::datatypes::pIntType;
            return true;
        case PulseCheck:
            Dest.Int  = STUCK->Check;
            Dest.Type = mq::datatypes::pIntType;
            return true;
        case PulseUnstuck:
            Dest.Int  = STUCK->Unstuck;
            Dest.Type = mq::datatypes::pIntType;
            return true;
        case TryToJump:
            Dest.DWord = STUCK->Jump;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case DistStuck:
            Dest.Float = STUCK->Dist;
            Dest.Type  = mq::datatypes::pFloatType;
            return true;
        case Version:
            sprintf_s(DataTypeTemp, "%1.4f", MQ2Version);
            Dest.Ptr  = &DataTypeTemp[0];
            Dest.Type = mq::datatypes::pStringType;
            return true;
        case MovePause:
            Dest.DWord = SET->PauseKB;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        case GM:
            Dest.DWord = pMU->BrokeGM;
            Dest.Type  = mq::datatypes::pBoolType;
            return true;
        }
        return false;
    }

    virtual bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        strcpy_s(Destination, MAX_STRING, "NONE");
        if (STICK->On)
        {
            strcpy_s(Destination, MAX_STRING, "STICK");
        }
        else if (CIRCLE->On)
        {
            strcpy_s(Destination, MAX_STRING, "CIRCLE");
        }
        else if (MOVETO->On)
        {
            strcpy_s(Destination, MAX_STRING, "MOVETO");
        }
        else if (CURCAMP->On)
        {
            strcpy_s(Destination,MAX_STRING, "MAKECAMP");
        }
        return true;
    }
};

bool dataMoveUtils(const char* szName, MQTypeVar &Ret)
{
    Ret.DWord = 1;
    Ret.Type  = pMoveUtilsType;
    return true;
}

// End Custom Top-Level Objects
// -----------------------------
// Blech Events

void __stdcall CheckAggro_Event(unsigned int ID, void *pData, PBLECHVALUE pValues)
{
    // do not process if makecamp is on, as that would interfere with camp returns
    if (!ValidIngame() || CURCAMP->On) return;
    if (MOVETO->On && MOVETO->BreakHit)
    {
        pMU->MovetoBroke = true;
        sprintf_s(szMsg, "\ay%s\aw:: Aggro gained during /moveto, Halting command...", PLUGIN_NAME);
    }
    else if (STICK->On && STICK->BreakHit)
    {
        pMU->StickBroke = true;
        sprintf_s(szMsg, "\ay%s\aw:: Aggro gained during /stick, Halting command...", PLUGIN_NAME);
    }
    else
    {
        return;
    }
    EndPreviousCmd(true);
    WriteLine(szMsg, V_BREAKONHIT);
}

void __stdcall CheckGates_Event(unsigned int ID, void *pData, PBLECHVALUE pValues)
{
    if (!ValidIngame() || !STICK->On || !STICK->BreakGate)
        return;
    PlayerClient* psTarget = ((STICK->Hold && STICK->HoldID) ? GetSpawnByID(STICK->HoldID) : pTarget);
    if (psTarget && pValues)
    {
        if (pValues->Value == psTarget->DisplayedName)
        {
            EndPreviousCmd(true);
            sprintf_s(szMsg, "\ay%s\aw:: Mob gating ended previous command.", PLUGIN_NAME);
            WriteLine(szMsg, V_BREAKONGATE);
        }
    }
}

void SetupEvents(bool bAddEvent, bool bForceRemove)
{
    if (bAddEvent && pMoveEvent)
    {
        if (SET_M->BreakHit || MOVETO->BreakHit || SET_S->BreakHit || STICK->BreakHit)
        {
            if (!Event_AggroNorm)
                Event_AggroNorm    = pMoveEvent->AddEvent("#Name# #Hurts# YOU for #Damage#",          CheckAggro_Event);
            if (!Event_MissNorm)
                Event_MissNorm     = pMoveEvent->AddEvent("#Name# tries to #Hit# YOU, but #YouRock#", CheckAggro_Event);
            if (!Event_AggroAbbrev)
                Event_AggroAbbrev  = pMoveEvent->AddEvent("#Name# #Hurt#s for #Damage#",              CheckAggro_Event);
            if (!Event_MissAbbrev)
                Event_MissAbbrev   = pMoveEvent->AddEvent("#Name# missed",                            CheckAggro_Event);
            if (!Event_MissNumOnly)
                Event_MissNumOnly  = pMoveEvent->AddEvent("miss",                                     CheckAggro_Event);
            // not going to parse for just numbers as that would require parsing every line
            // and evaluating if its only numerical - this would be a bit over the top
        }
        if (SET_S->BreakGate || STICK->BreakGate)
        {
            if (!Event_Gates)
                Event_Gates = pMoveEvent->AddEvent("#Name# Gates.",CheckGates_Event);
        }
    }
    else
    {
        if (pMoveEvent) {
            if (bForceRemove || (!SET_M->BreakHit && !SET_S->BreakHit))
            {
                if (Event_AggroNorm)
                    pMoveEvent->RemoveEvent(Event_AggroNorm);
                Event_AggroNorm = NULL;

                if (Event_MissNorm)
                    pMoveEvent->RemoveEvent(Event_MissNorm);
                Event_MissNorm = NULL;

                if (Event_AggroAbbrev)
                    pMoveEvent->RemoveEvent(Event_AggroAbbrev);
                Event_AggroAbbrev = NULL;

                if (Event_MissAbbrev)
                    pMoveEvent->RemoveEvent(Event_MissAbbrev);
                Event_MissAbbrev = NULL;

                if (Event_MissNumOnly)
                    pMoveEvent->RemoveEvent(Event_MissNumOnly);
                Event_MissNumOnly = NULL;
            }
            if (bForceRemove || !SET_S->BreakGate)
            {
                if (Event_Gates) {
                    pMoveEvent->RemoveEvent(Event_Gates);
                    Event_Gates = NULL;
                }
            }
        }
    }
}

// ----------------------------------------
// deadchicken formula functions


// CampReturn copyright: deadchicken
// not to be released outside of VIP without permission
inline void CampReturn(float fY, float fX, float fUseRadius, float* fUseY, float* fUseX)
{
    float fRandHead = rand() / RAND_MAX * CIRCLE_MAX;
    float fRandDist = rand() / RAND_MAX * fUseRadius;

    SpewDebug(DBG_MISC, "MoveUtils::CampReturn() fRandHead = %.2f, fRandDist = %.2f", fRandHead, fRandDist);

    *fUseY = fY + (fRandDist * cos(fRandHead));
    *fUseX = fX + (fRandDist * sin(fRandHead));
}

// PolarSpot copyright: deadchicken
// not to be released outside of VIP without permission
// MOB:  PolarSpot(targetX, targetY, targetHeading, desiredHeading, distanceAway, scatter);
// CAMP: PolarSpot(anchorX, anchorY, heading doesnt matter, bearing = which dir from center, dist = how far from center, scatter=size of scatter);
inline void PolarSpot(float fY, float fX, float fPHeading, float fPBearing, float fPDist, float fPScatter, float* fUseY, float* fUseX)
{
    // if camp returning
    if (fPScatter != 0.0f)
    {
        float fRandHead = MOVE->SaneHead(rand() / RAND_MAX * HEADING_MAX);
        float fRandDist = rand() / RAND_MAX * fPScatter;

        *fUseY = fY + (fRandDist * cos(fRandHead));
        *fUseX = fX + (fRandDist * sin(fRandHead));

        // 0.0175f converts degrees to radians which sinf/cosf expect
        /*fUseX = fOurGotoX + fRandDist * sinf(fRandHead*0.0175f));
        fUseY = fOurGotoY + fRandDist * cosf(fRandHead*0.0175f));*/

        return;
    }
    // else snaproll

    //float fRelHead = (fPHeading / fPBearing) * -(float)PI;
    //STICK->Snap->Y = fY  - (float)cos(fRelHead)* fPDist;
    //STICK->Snap->X = fX  + (float)sin(fRelHead)* fPDist;
    float fRelHead = MOVE->SaneHead(fPHeading - fPBearing);
    fRelHead       = ((fRelHead / HEADING_MAX) * CIRCLE_MAX) * 0.0175f;
    *fUseY         = fY + (fPDist * cos(fRelHead));
    *fUseX         = fX + (fPDist * sin(fRelHead));
}

// MovingAvg copyright: deadchicken
// not to be released outside of VIP without permission
// returns a moving average of size iEntries by adding fNew to the ring and computing
// fNew = New value to add to the ring
// iEntries = number of entries in ring, used for divisior and to re-init ring on change/init.
// Returns the moving average based on said values
// Notes: MaxRing size is 32 by #define up top and not checked, should be
float MovingAvg(float fNew, int iEntries)
{
    static float fRing[MAXRINGSIZE] = { 0.0f };
    static int            iOldStuck = 0;
    static int              iRinger = 0;
    static int            iRingFull = 0;
           int                    i = 0;
         float                 fAvg = 0.0f;

    // Bounds checking
    if (iEntries > MAXRINGSIZE || iEntries < 2 ) return fNew;
    // Do we have a new ring size?
    if (iOldStuck != iEntries)
    {
        // Do some shit to make us right
        SpewDebug(DBG_MISC, "MoveUtils::MovingAvg() Entry # changed, filling ring with %3.2f!  %d != %d", fNew, iOldStuck, iEntries);
        // Fill the array with this latest value
        // maybe this should be our default preload value of 2.0f?
        for(i = 0; i < iEntries; i++) fRing[i] = fNew;
        // update iOldStuck and reset counter to 0
        iRinger   = 0;
        iOldStuck = iEntries;
        return fNew;
    }
    else
    {
        // Plain old ring buffer
        fRing[iRinger] = fNew;
        SpewDebug(DBG_MISC, "MoveUtils::MovingAvg() Added %3.2f to fRing[%d]", fNew, iRinger);
        // Increment iRinger and wrap if needed, if we wrap then it's full
        iRinger++;
        if (iRinger >= iEntries)
        {
            iRinger   = 0;
            iRingFull = 1;
        }
        // Get the sum of the ring
        //for( i=0; i<(iRingFull?iEntries:iRinger); i++) {  <-- this was a bad idea
        for(i = 0; i < iEntries; i++)
        {
            SpewDebug(DBG_MISC, "MoveUtils::MovingAvg() i=%d and fRing[%d]=%3.2f", i, i, fRing[i]);
            fAvg += fRing[i];
        }
    }
    return (fAvg / (float)iEntries);
}

// ----------------------------------------
// Utility
inline bool ValidIngame(bool bCheckDead)
{
    // CTD prevention function
    PlayerClient* pChSpawn = pCharSpawn;
    if (GetGameState() != GAMESTATE_INGAME || !pLocalPlayer || !pChSpawn->SpawnID || !pMU || (bCheckDead && pChSpawn->RespawnTimer > 0))
    {
        return false;
    }
    return true;
}

// ----------------------------------------
// Begin user command handling

// ends active commands
void EndPreviousCmd(bool bStopMove, unsigned char ucCmdUsed, bool bPreserveSelf)
{
    //reset pause
    PAUSE->PausedMU = PAUSE->PausedCmd = false;
    PAUSE->TimeStop();
    PAUSE->Reset();

    // break any active commands
    if (ucCmdUsed != CMD_CIRCLE || !bPreserveSelf)
    {
        pMU->NewCircle();
    }
    if (ucCmdUsed != CMD_MOVETO || !bPreserveSelf)
    {
        pMU->NewMoveTo();
    }
    if (ucCmdUsed != CMD_STICK || !bPreserveSelf)
    {
        pMU->NewStick();
    }

    pMU->Defaults();
    MOVE->SetWalk(false);
    SetupEvents(false);
    if (bStopMove)
    {
        MOVE->StopHeading();
        MOVE->StopMove(APPLY_TO_ALL);
    }
}

// main MU command handler, called by wrappers
void HandleOurCmd(unsigned char ucCmdUsed, const char* szInput)
{
    // don't allow commands from char select or cfg files that load before entering world
    if (!ValidIngame(false)) return;

    char szCurrentArg[MAX_STRING]       = {};   // stores current argument from szInput
    char szTempID[MAX_STRING]           = {};   // stores output msg for stick
    unsigned int uiArgNum               = 1;    // argument number to evaluate
    float fTempY                        = 0.0f; // store cmd input to temp var before setting to prevent inconsistency on failed input

    PlayerClient* pTargetUsed = nullptr; // stick id, moveto id
    PlayerClient* pCampPlayer = nullptr; // makecamp player
    PlayerClient* psTarget    = pTarget;
    PlayerClient* pLPlayer    = pLocalPlayer;
    PlayerClient* pChSpawn    = pCharSpawn;

    // switch direction of turnhalf randomly
    if (rand() % 100 > 50) STUCK->TurnSize *= -1.0f;
    // call bardcheck function upon command usage instead of onpulse
    pMU->Bard = ME->IsBard();
    // reset state of ${MoveTo.Broken} upon next '/moveto' command
    if (ucCmdUsed == CMD_MOVETO)
    {
        // added moveto.stopped here as well
        pMU->StoppedMoveto = false;
        pMU->MovetoBroke   = false;
    }
    // reset state of ${Stick.Broken} upon next '/stick' command
    if (ucCmdUsed == CMD_STICK)
    {
        pMU->StickBroke = false;
    }

    // get first argument
    GetArg(szCurrentArg, szInput, uiArgNum++);
    // if no argument supplied
    if (!*szCurrentArg)
    {
        // halt all other commands due to BreakOnGM/BreakOnSummon
        if (pMU->BrokeGM || pMU->BrokeSummon)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to \ay%s\ax triggering.", PLUGIN_NAME, pMU->BrokeGM ? "BreakOnGM" : "BreakOnSummon");
            WriteLine(szMsg, V_SILENCE);
            return;
        }
        // halt all other commands due to rootme being on
        if (pMU->Rooted)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to \ayrootme\ax active.", PLUGIN_NAME);
            WriteLine(szMsg, V_SILENCE);
            return;
        }
        // halt new commands if plugin is paused & lockpause is on
        if (PAUSE->PausedCmd && pMU->LockPause)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to plugin paused with lockpause.", PLUGIN_NAME);
            WriteLine(szMsg, V_PAUSED);
            return;
        }
        PAUSE->PausedMU = false;
        switch(ucCmdUsed)
        {
        case CMD_MAKECAMP:
            if (!CURCAMP->On)
            {
                PAUSE->TimeStop();
                CAMP->Activate(pChSpawn->Y, pChSpawn->X);
                sprintf_s(szMsg, "\ay%s\aw:: MakeCamp actived. Y(\ag%.2f\ax) X(\ag%.2f\ax) Radius(\ag%.2f\ax) Leash(%s) LeashLen(\ag%.2f\ax) Min(\ag%d\ax) Max(\ag%d\ax)", PLUGIN_NAME, CURCAMP->Y, CURCAMP->X, CURCAMP->Radius, CURCAMP->Leash ? "\agon\ax" : "\aroff\ax", CURCAMP->Length, CAMP->Min, CAMP->Max);
                WriteLine(szMsg, V_MAKECAMPV);
                break;
            }
            CAMP->ResetCamp(true);
            break;
        case CMD_STICK:
            EndPreviousCmd(true);
            if (psTarget)
            {
                // prevent sticking to self/mount
                if (ME->IsMe(psTarget))
                {
                    SpewMUError(ERR_STICKSELF);
                    break;
                }
                STICK->TurnOn();
                MOVE->DoStand();
                sprintf_s(szMsg, "\ay%s\aw:: You are now sticking to \ag%s\ax.", PLUGIN_NAME, psTarget->DisplayedName);
                WriteLine(szMsg, V_STICKV);
                break;
            }
            SpewMUError(ERR_STICKNONE);
            break;
        case CMD_MOVETO:
        case CMD_CIRCLE:
        default:
            EndPreviousCmd(true);
            // possible future use, currently '/circle' and '/moveto' designed to fail
            sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) /moveto or /circle command used with no parameter.", PLUGIN_NAME);
            WriteLine(szMsg, V_ERRORS);
            break;
        }
        return;
    }

    if (*szCurrentArg)
    {
        // generic parameters that we want to enforce first-parameter syntax
        if (!_strnicmp(szCurrentArg, "help", 5))
        {
            unsigned char ucCaller = ucCmdUsed;
            GetArg(szCurrentArg, szInput, uiArgNum);
            if (!_strnicmp(szCurrentArg, "settings", 9))
            {
                ucCaller = HELP_SETTINGS;
            }
            OutputHelp(ucCaller);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "debug", 6))
        {
            DebugToINI(ucCmdUsed);
            sprintf_s(szMsg, "\ay%s\aw:: Debug file created.", PLUGIN_NAME);
            WriteLine(szMsg, V_SAVED);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "status", 7))
        {
            GetArg(szCurrentArg, szInput, uiArgNum);
            if (!_strnicmp(szCurrentArg, "all", 4))
            {
                DebugToWnd(APPLY_TO_ALL);
                return;
            }
            DebugToWnd(ucCmdUsed);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "pause", 6))
        {
            bool bDisplayLock = false;
            GetArg(szCurrentArg, szInput, uiArgNum);
            if (!_strnicmp(szCurrentArg, "lock", 5))
            {
                pMU->LockPause = true;
                bDisplayLock = true;
            }
            if (!PAUSE->PausedCmd)
            {
                PAUSE->PausedCmd = true;
                PAUSE->TimeStop();
                MOVE->StopHeading();
                MOVE->StopMove(APPLY_TO_ALL);
                sprintf_s(szMsg, "\ay%s\aw:: \arPAUSED\ax%s", PLUGIN_NAME, pMU->LockPause ? " \ayLOCKED" : "");
                WriteLine(szMsg, V_PAUSED);
                return;
            }
            if (!bDisplayLock)
            {
                sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Plugin was already paused.", PLUGIN_NAME);
                WriteLine(szMsg, V_ERRORS);
            }
            else
            {
                sprintf_s(szMsg, "\ay%s\aw:: Pause \ayLOCKED", PLUGIN_NAME);
                WriteLine(szMsg, V_PAUSED);
            }
            return;
        }
        else if (!_strnicmp(szCurrentArg, "unpause", 8))
        {
            if (PAUSE->PausedMU || PAUSE->PausedCmd)
            {
                PAUSE->PausedMU = PAUSE->PausedCmd = false;
                pMU->LockPause = SET->LockPause; // reset one-time usage if non-default
                PAUSE->TimeStop();
                PAUSE->Reset();
                sprintf_s(szMsg, "\ay%s\aw:: \agRESUMED", PLUGIN_NAME);
                WriteLine(szMsg, V_PAUSED);
                return;
            }
            if (!bWrapped)
            {
                sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Plugin was not paused.", PLUGIN_NAME);
                WriteLine(szMsg, V_ERRORS);
            }
            bWrapped = false; // cheap fix
            return;
        }
        else if (!_strnicmp(szCurrentArg, "save", 5))
        {
            SaveConfig();
            sprintf_s(szMsg, "\ay%s\aw:: Saved settings to %s", PLUGIN_NAME, INIFileName);
            WriteLine(szMsg, V_SAVED);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "load", 5))
        {
            EndPreviousCmd(true);
            LoadConfig();
            sprintf_s(szMsg, "\ay%s\aw:: Loaded settings from %s", PLUGIN_NAME, INIFileName);
            WriteLine(szMsg, V_SAVED);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "imsafe", 7))
        {
            pMU->BrokeSummon = pMU->BrokeGM = false;
            EndPreviousCmd(true);
            sprintf_s(szMsg, "\ay%s\aw:: Command usage allowed once again.", PLUGIN_NAME);
            WriteLine(szMsg, V_SILENCE);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "set", 4))
        {
            char szTempSet[MAX_STRING] = {};
            sprintf_s(szTempSet, "%s", GetNextArg(szInput, 1, FALSE, 0));
            ChangeSetting(ucCmdUsed, false, szTempSet);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "toggle", 7))
        {
            char szTempSet[MAX_STRING] = {};
            sprintf_s(szTempSet, "%s", GetNextArg(szInput, 1, FALSE, 0));
            ChangeSetting(ucCmdUsed, true, szTempSet);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "min", 4))
        {
            WINDOW->Min();
            return;
        }
        else if (!_strnicmp(szCurrentArg, "clear", 6))
        {
            WINDOW->Clear();
            return;
        }
        else if (!_strnicmp(szCurrentArg, "verbflags", 10))
        {
            sprintf_s(szMsg, "\ay%s\aw:: Current verbosity flags \ag%d", PLUGIN_NAME, uiVerbLevel);
            WriteLine(szMsg, V_SILENCE);
            return;
        }

        // halt all other commands due to BreakOnGM/BreakOnSummon
        if (pMU->BrokeGM || pMU->BrokeSummon)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to \ay%s\ax triggering.", PLUGIN_NAME, pMU->BrokeGM ? "BreakOnGM" : "BreakOnSummon");
            WriteLine(szMsg, V_SILENCE);
            return;
        }
        // halt all other commands due to rootme being on
        if (pMU->Rooted)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to \ayrootme\ax active.", PLUGIN_NAME);
            WriteLine(szMsg, V_SILENCE);
            return;
        }
        // halt new commands if plugin is paused & lockpause is on
        if (PAUSE->PausedCmd && pMU->LockPause)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arCommand failed due to plugin paused with lockpause.", PLUGIN_NAME);
            WriteLine(szMsg, V_PAUSED);
            return;
        }

        // non-generic parameters that we want to enforce first-parameter syntax
        if (!_strnicmp(szCurrentArg, "on", 3) && (ucCmdUsed == CMD_MAKECAMP || ucCmdUsed == CMD_CIRCLE))
        {
            switch (ucCmdUsed)
            {
            case CMD_MAKECAMP:
                PAUSE->PausedMU = false;
                PAUSE->TimeStop();
                CAMP->Activate(pChSpawn->Y, pChSpawn->X);
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CURCAMP->SetRadius((float)atof(szCurrentArg));
                    GetArg(szCurrentArg, szInput, uiArgNum); // because when we break from this we enter 'while' for NEW args
                }
                break;
            case CMD_CIRCLE:
                EndPreviousCmd(true);
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CIRCLE->SetRadius((float)atof(szCurrentArg));
                    GetArg(szCurrentArg, szInput, uiArgNum); // because when we break from this we enter 'while' for NEW args
                }
                CIRCLE->AtMe();
                break;
            }
        }
        else if (!_strnicmp(szCurrentArg, "mod", 4) && ucCmdUsed == CMD_STICK)
        {
            GetArg(szCurrentArg, szInput, uiArgNum++);
            if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
            {
                STICK->DistMod = (float)atof(szCurrentArg);
                sprintf_s(szMsg, "\ay%s\aw:: Stick modifier changed to Mod(\ag%.2f\ax) Mod%%(\ag%.2f%%\ax)", PLUGIN_NAME, STICK->DistMod, STICK->DistModP);
                WriteLine(szMsg, V_SETTINGS);
                return;
            }
            sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) \ay/stick mod [#]\ax supplied incorrectly.", PLUGIN_NAME);
            WriteLine(szMsg, V_ERRORS);
            return;
        }
        else if(!_strnicmp(szCurrentArg, "loc", 4) && ucCmdUsed != CMD_STICK)
        {
            GetArg(szCurrentArg, szInput, uiArgNum++);
            switch (ucCmdUsed)
            {
            case CMD_MOVETO:
                EndPreviousCmd(true);
                if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                {
                    fTempY = (float)atof(szCurrentArg);
                    GetArg(szCurrentArg, szInput, uiArgNum++);
                    if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                    {
                        float fTempX = (float)atof(szCurrentArg);
                        GetArg(szCurrentArg, szInput, uiArgNum++);
                        if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                        {
                            MOVETO->Activate(fTempY, fTempX, (float)atof(szCurrentArg));
                            GetArg(szCurrentArg, szInput, uiArgNum);
                            break;
                        }
                        MOVETO->Activate(fTempY, fTempX, 0.0f);
                        break;
                    }
                }
                SpewMUError(ERR_BADMOVETO);
                return;
            case CMD_MAKECAMP:
                PAUSE->PausedMU = false;
                PAUSE->TimeStop();
                if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                {
                    fTempY = (float)atof(szCurrentArg);
                    GetArg(szCurrentArg, szInput, uiArgNum++);
                    if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                    {
                        CAMP->Activate(fTempY, (float)atof(szCurrentArg));
                        GetArg(szCurrentArg, szInput, uiArgNum);
                        break;
                    }
                }
                SpewMUError(ERR_BADMAKECAMP);
                return;
            case CMD_CIRCLE:
                EndPreviousCmd(true, ucCmdUsed, true); // dont reset circle variables
                if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                {
                    fTempY = (float)atof(szCurrentArg);
                    GetArg(szCurrentArg, szInput, uiArgNum++);
                    if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.')
                    {
                        CIRCLE->AtLoc(fTempY, (float)atof(szCurrentArg));
                        GetArg(szCurrentArg, szInput, uiArgNum);
                        break;
                    }
                }
                SpewMUError(ERR_BADCIRCLE);
                return;
            }
        }
        else if((!_strnicmp(szCurrentArg, "yloc", 5) || !_strnicmp(szCurrentArg, "xloc", 5)) && ucCmdUsed == CMD_MOVETO)
        {
            bool bUsingY = (!_strnicmp(szCurrentArg, "yloc", 5)) ? true : false;
            EndPreviousCmd(true);
            GetArg(szCurrentArg, szInput, uiArgNum++);
            if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '-' || szCurrentArg[0] == '.' )
            {
                if (bUsingY)
                {
                    MOVETO->Activate((float)atof(szCurrentArg), pChSpawn->X, 0.0f);
                }
                else
                {
                    MOVETO->Activate(pChSpawn->Y, (float)atof(szCurrentArg), 0.0f);
                }
                GetArg(szCurrentArg, szInput, uiArgNum);
            }
            else
            {
                sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) \ay/moveto %s\ax was supplied incorrectly.", PLUGIN_NAME, bUsingY ? "yloc [Y]" : "xloc [X]");
                WriteLine(szMsg, V_ERRORS);
                return;
            }
        }

        // reset stick vars one time
        if (ucCmdUsed == CMD_STICK)
        {
            EndPreviousCmd(true);
        }
    }

    while (*szCurrentArg)
    {
        if (!_strnicmp(szCurrentArg, "off", 4))
        {
            switch(ucCmdUsed)
            {
            case CMD_MAKECAMP:
                CAMP->ResetCamp(true);
                MOVE->StopHeading();
                return;
            case CMD_STICK:
                sprintf_s(szMsg, "\ay%s\aw:: You are no longer sticking to anything.", PLUGIN_NAME);
                WriteLine(szMsg, V_STICKV);
                break;
            case CMD_CIRCLE:
                sprintf_s(szMsg, " \ay%s\aw:: Circling radius (\ag%.2f\ax), center (\ag%.2f\ax, \ag%.2f\ax) : \arOFF", PLUGIN_NAME, CIRCLE->Radius, CIRCLE->Y, CIRCLE->X);
                WriteLine(szMsg, V_CIRCLEV);
                break;
            case CMD_MOVETO:
                sprintf_s(szMsg, "\ay%s\aw:: Moveto off.", PLUGIN_NAME);
                WriteLine(szMsg, V_MOVETOV);
                break;
            }
            EndPreviousCmd(true);
            return;
        }
        else if (!_strnicmp(szCurrentArg, "id", 3) && ucCmdUsed != CMD_MAKECAMP)
        {
            EndPreviousCmd(true, ucCmdUsed, true);
            PlayerClient* pByID = NULL;
            GetArg(szCurrentArg, szInput, uiArgNum);
            if (*szCurrentArg)
            {
                char* pNotNum = NULL;
                int iValid = strtoul(szCurrentArg, &pNotNum, 10);
                // strtoul verifies the arg is 100% numerical, atoi/atof do not
                if (iValid < 1 || *pNotNum)
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) SpawnID must be a positive numerical value.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                pByID = GetSpawnByID((unsigned long)iValid);
                if (pByID)
                {
                    if (ME->IsMe(pByID))
                    {
                        EndPreviousCmd(true);
                        sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You cannot use yourself or your mount.", PLUGIN_NAME);
                        WriteLine(szMsg, V_ERRORS);
                        return;
                    }
                    pTargetUsed = pByID;
                    uiArgNum++; // incremeted if # is valid, but not otherwise so that someone can use '/stick id behind' to use target. bad form but nonetheless
                }
                else
                {
                    pTargetUsed = NULL;
                }
            }
            else if (psTarget)
            {
                if (ME->IsMe(psTarget))
                {
                    SpewMUError(ERR_BADSPAWN);
                    return;
                }
                pTargetUsed = psTarget; // only use target if its not ourself
            }
            if (!pTargetUsed)
            {
                SpewMUError(ERR_BADSPAWN);
                return;
            }
            //if we've made it this far, pTargetUsed is valid
            switch (ucCmdUsed)
            {
            case CMD_STICK:
                STICK->Always   = false; // turns off 'always' when using 'id'
                STICK->HoldID   = pTargetUsed->SpawnID;
                STICK->HoldType = GetSpawnType(pTargetUsed);
                STICK->Hold     = true;
                STICK->TurnOn();
                break;
            case CMD_MOVETO:
                // moveto id with GetDistance3D is causing some users issues
                MOVETO->Activate(pTargetUsed->Y, pTargetUsed->X, 0.0f);
                break;
            case CMD_CIRCLE:
                CIRCLE->AtLoc(pTargetUsed->Y, pTargetUsed->X);
                break;
            }
        }
        // stick specific parameters
        else if (ucCmdUsed == CMD_STICK)
        {
            if (strstr(szCurrentArg, "%"))
            {
                STICK->DistModP = (float)atof(szCurrentArg) / 100.0f;
                // shouldnt do this here, need logic to output this only if used by itself
                // cant do it on an 'else' for the pTarget 'if' because of 'always' param
                sprintf_s(szMsg, "\ay%s\aw:: Stick mod changed Mod(\ag%.2f\ax) ModPercent(\ag%.2f%%\ax)", PLUGIN_NAME, STICK->DistMod, STICK->DistModP);
                WriteLine(szMsg, V_SETTINGS);
                if (STICK->SetDist && STICK->Dist * STICK->DistModP > 0.0f) STICK->Dist *= STICK->DistModP; // possible float math error here?
                STICK->TurnOn();
            }
            else if (szCurrentArg[0] == '-')
            {
                STICK->DistMod = (float)atof(szCurrentArg);
                sprintf_s(szMsg, "\ay%s\aw:: Stick mod changed Mod(\ag%.2f\ax) ModPercent(\ag%.2f%%\ax)", PLUGIN_NAME, STICK->DistMod, STICK->DistModP);
                WriteLine(szMsg, V_SETTINGS);
                if (STICK->SetDist && STICK->Dist + STICK->DistMod >= 0.0f) STICK->Dist += STICK->DistMod; // possible float math error here?
                STICK->TurnOn();
            }
            else if (isdigit(szCurrentArg[0]) || szCurrentArg[0] == '.' )
            {
                if ((float)atof(szCurrentArg) * STICK->DistModP + STICK->DistMod > 0.0f)
                {
                    STICK->Dist = fStickDistance = GetFloatFromString(szCurrentArg, 0.0f) * STICK->DistModP + STICK->DistMod;
                }
                STICK->SetDist = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "moveback", 9))
            {
                STICK->MoveBack = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "loose", 6))
            {
                pMU->Head = H_LOOSE;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "truehead", 9))
            {
                pMU->Head = H_TRUE;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "healer", 7))
            {
                STICK->Healer = true;
                STICK->Behind = STICK->BehindOnce = STICK->Front = STICK->Snaproll = false;
                STICK->NotFront = STICK->Pin = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "uw", 3) || !_strnicmp(szCurrentArg, "underwater", 11))
            {
                STICK->UW = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "hold", 5))
            {
                if (psTarget)
                {
                    if (ME->IsMe(psTarget))
                    {
                        sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You cannot stick hold to yourself.", PLUGIN_NAME);
                        WriteLine(szMsg, V_ERRORS);
                        EndPreviousCmd(true);
                        return;
                    }
                    pTargetUsed     = psTarget;
                    STICK->HoldID   = pTargetUsed->SpawnID;
                    STICK->HoldType = GetSpawnType(pTargetUsed);
                    STICK->Hold     = true;
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    SpewMUError(ERR_STICKNONE);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "on", 3))
            {
                // useless param, but removing it breaks popular macros
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "behind", 7))
            {
                STICK->Behind = true;
                STICK->Healer = STICK->Snaproll = STICK->BehindOnce = STICK->Pin = STICK->NotFront = STICK->Front = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "behindonce", 11))
            {
                STICK->BehindOnce = true;
                STICK->Healer = STICK->Snaproll = STICK->Behind = STICK->Pin = STICK->NotFront = STICK->Front = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "!front", 7))
            {
                STICK->NotFront = true;
                STICK->Healer = STICK->Snaproll = STICK->Behind = STICK->Pin = STICK->BehindOnce = STICK->Front = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "front", 6))
            {
                STICK->Front = true;
                STICK->Healer = STICK->Snaproll = STICK->Behind = STICK->Pin = STICK->BehindOnce = STICK->NotFront = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "pin", 4))
            {
                STICK->Pin = true;
                STICK->Healer = STICK->Snaproll = STICK->Behind = STICK->BehindOnce = STICK->NotFront = STICK->Front = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "snaproll", 9))
            {
                STICK->Snaproll = true;
                STICK->Healer = STICK->Behind = STICK->BehindOnce = STICK->NotFront = STICK->Front = STICK->Pin = false;
                STICK->Snap->Bearing = HEADING_HALF;
                GetArg(szCurrentArg, szInput, uiArgNum);
                if (!_strnicmp(szCurrentArg, "face", 6))
                {
                    uiArgNum++;
                    STICK->Snap->Bearing = 1.0f;
                }
                else if (!_strnicmp(szCurrentArg, "right", 6))
                {
                    uiArgNum++;
                    STICK->Snap->Bearing = HEADING_QUARTER;
                }
                else if (!_strnicmp(szCurrentArg, "left", 5))
                {
                    uiArgNum++;
                    STICK->Snap->Bearing = (HEADING_HALF + HEADING_QUARTER);
                }
                else if (!_strnicmp(szCurrentArg, "rear", 5))
                {
                    uiArgNum++; // uses HEADING_HALF (set above)
                }
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "breakontarget", 14))
            {
                STICK->BreakTarget = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "breakongate", 12))
            {
                STICK->BreakGate = true;
                SetupEvents(true);
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "breakonwarp", 12))
            {
                STICK->BreakWarp = true;
                STICK->PauseWarp = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "breakonhit", 11))
            {
                STICK->BreakHit = true;
                SetupEvents(true);
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "pauseonwarp", 12))
            {
                STICK->PauseWarp = true;
                STICK->BreakWarp = false;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "randomize", 10))
            {
                STICK->Randomize = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "delaystrafe", 12))
            {
                STICK->DelayStrafe = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "useback", 8))
            {
                STICK->UseBack = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "usefleeing", 11))
            {
                STICK->UseFleeing = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "useflex", 8))
            {
                STICK->Flex = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "strafewalk", 11))
            {
                STICK->Walk = true;
                STICK->TurnOn();
            }
            else if (!_strnicmp(szCurrentArg, "mindelay", 9) || !_strnicmp(szCurrentArg, "maxdelay", 9))
            {
                bool bDoMax = !_strnicmp(szCurrentArg, "maxdelay", 9) ? true : false;
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    if (bDoMax)
                    {
                        STICK->MaxDelay(atoi(szCurrentArg));
                    }
                    else
                    {
                        STICK->MinDelay(atoi(szCurrentArg));
                    }
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    SpewMUError(ERR_BADDELAY);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "backupdist", 11))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    STICK->DistBack = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: \arbackupdist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "breakdist", 10))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    STICK->DistBreak = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: \arbreakdist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "snapdist", 9))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    STICK->DistSnap = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: \arsnapdist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "flexdist", 9))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) >= 2.0f && (float)atof(szCurrentArg) <= 20.0f)
                {
                    STICK->DistFlex = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: \arflexdist must be between 2.0 and 20.0.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "!frontarc", 10))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f && (float)atof(szCurrentArg) <= 260.0f)
                {
                    STICK->ArcNotFront = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: \ar!frontarc must be between 1.0 and 260.0", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "behindarc", 10))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f && (float)atof(szCurrentArg) <= 260.0f)
                {
                    STICK->ArcBehind = (float)atof(szCurrentArg);
                    STICK->TurnOn();
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: \arbehindarc must be between 1.0 and 260.0", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "always", 7))
            {
                STICK->FirstAlways();
                sprintf_s(szMsg, "\ay%s\aw:: You will now stick to every valid NPC target supplied.", PLUGIN_NAME);
                WriteLine(szMsg, V_STICKV);
                MOVE->DoStand();
                return; // 'always' must be the last parameter
            }
            else
            {
                EndPreviousCmd(true);
                if (uiVerbLevel != 0 && (uiVerbLevel & V_HIDEHELP) != V_HIDEHELP) OutputHelp(ucCmdUsed, true);
                return;
            }
        }
        // moveto specific parameters
        else if (ucCmdUsed == CMD_MOVETO)
        {
            if (!_strnicmp(szCurrentArg, "loose", 6))
            {
                pMU->Head = H_LOOSE;
            }
            else if (!_strnicmp(szCurrentArg, "truehead", 9))
            {
                pMU->Head = H_TRUE;
            }
            else if (!_strnicmp(szCurrentArg, "dist", 5))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (szCurrentArg[0] == '-')
                {
                    MOVETO->Mod = (float)atof(szCurrentArg);
                    MOVETO->Dist += MOVETO->Mod;
                }
                else if (isdigit(szCurrentArg[0]))
                {
                    MOVETO->Dist = ((float)atof(szCurrentArg) >= 1.0f) ? (float)atof(szCurrentArg) : MOVETO->Dist;
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Incorrectly used \ay/moveto dist [#]\ax", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                SET_M->Dist = MOVETO->Dist;
                SET_M->Mod  = MOVETO->Mod;
                sprintf_s(szMsg, "\ay%s\aw:: Moveto distance mod changed to \ag%.2f\ax.", PLUGIN_NAME, MOVETO->Dist);
                WriteLine(szMsg, V_SETTINGS);
                return;
            }
            else if (!_strnicmp(szCurrentArg, "mdist", 6))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (szCurrentArg[0] == '-')
                {
                    MOVETO->Mod = (float)atof(szCurrentArg);
                    MOVETO->Dist += MOVETO->Mod;
                }
                else if (isdigit(szCurrentArg[0]))
                {
                    MOVETO->Dist = ((float)atof(szCurrentArg) >= 1.0f) ? (float)atof(szCurrentArg) : MOVETO->Dist;
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Incorrectly used \ay/moveto mdist [#]\ax", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                SET_M->Dist = MOVETO->Dist;
                SET_M->Mod  = MOVETO->Mod;
            }
            else if (!_strnicmp(szCurrentArg, "precisey", 9))
            {
                MOVETO->PreciseY = true;
                MOVETO->PreciseX = false;
            }
            else if (!_strnicmp(szCurrentArg, "precisex", 9))
            {
                MOVETO->PreciseX = true;
                MOVETO->PreciseY = false;
            }
            else if (!_strnicmp(szCurrentArg, "uw", 3) || !_strnicmp(szCurrentArg, "underwater", 11))
            {
                MOVETO->UW = true;
            }
            else if (!_strnicmp(szCurrentArg, "breakonaggro", 13))
            {
                MOVETO->BreakAggro = true;
                SetupEvents(true);
            }
            else if (!_strnicmp(szCurrentArg, "breakonhit", 11))
            {
                MOVETO->BreakHit = true;
                SetupEvents(true);
            }
            else if (!_strnicmp(szCurrentArg, "usewalk", 8))
            {
                MOVETO->Walk = true;
            }
            else if (!_strnicmp(szCurrentArg, "useback", 8))
            {
                MOVETO->UseBack = true;
            }
            else if (!_strnicmp(szCurrentArg, "backupdist", 11))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    MOVETO->DistBack = (float)atof(szCurrentArg);
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: \armoveto backup must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "ydist", 6))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    MOVETO->DistY = (float)atof(szCurrentArg);
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: \armoveto ydist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "xdist", 6))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    MOVETO->DistX = (float)atof(szCurrentArg);
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: \armoveto xdist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else
            {
                EndPreviousCmd(true);
                if (uiVerbLevel != 0 && (uiVerbLevel & V_HIDEHELP) != V_HIDEHELP) OutputHelp(ucCmdUsed, true);
                return;
            }
        }
        // makecamp specific parameters
        else if (ucCmdUsed == CMD_MAKECAMP)
        {
            if (!_strnicmp(szCurrentArg, "leash", 6))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CURCAMP->SetLeash((float)atof(szCurrentArg));
                    CURCAMP->Leash = true;
                }
                else
                {
                    CURCAMP->Leash = !CURCAMP->Leash;
                }
            }
            else if (!_strnicmp(szCurrentArg, "radius", 7))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CURCAMP->SetRadius((float)atof(szCurrentArg));
                }
                else
                {
                    CAMP->NewCamp(false);
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) \ay/makecamp [radius <dist>]\ax was supplied incorrectly.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "mindelay", 9) || !_strnicmp(szCurrentArg, "maxdelay", 9))
            {
                bool bDoMax = !_strnicmp(szCurrentArg, "maxdelay", 9) ? true : false;
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    if (bDoMax)
                    {
                        CAMP->MaxDelay(atoi(szCurrentArg));
                    }
                    else
                    {
                        CAMP->MinDelay(atoi(szCurrentArg));
                    }
                }
                else
                {
                    CAMP->NewCamp(false);
                    SpewMUError(ERR_BADDELAY);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "return", 7))
            {
                if (!CURCAMP->On)
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You do not have an active camp.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                CAMP->DoReturn = true;
                MOVE->DoStand();
                sprintf_s(szMsg, "\ay%s\aw:: MakeCamp returning to within camp radius immediately.", PLUGIN_NAME);
                WriteLine(szMsg, V_MAKECAMPV);
                return;
            }
            else if (!_strnicmp(szCurrentArg, "altreturn", 10))
            {
                if (CURCAMP->Pc)
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You cannot use this command with a player-camp active.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                if (!ALTCAMP->On || (ALTCAMP->X == 0.0f && ALTCAMP->Y == 0.0f))
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You cannot use this command until you've established an altcamp location.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                sprintf_s(szMsg, "\ay%s\aw:: MakeCamp returning to altcamp immediately.%s", PLUGIN_NAME, CURCAMP->On ? " Current camp now \arOFF\ax." : "");
                CAMP->NewCamp(false);
                CAMP->DoAlt = true;
                MOVE->DoStand();
                WriteLine(szMsg, V_MAKECAMPV);
                return;
            }
            else if (!_strnicmp(szCurrentArg, "player", 7))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (*szCurrentArg)
                {
                    pCampPlayer = GetSpawnByName(szCurrentArg);
                }
                else if (psTarget && psTarget->Type == SPAWN_PLAYER)
                {
                    pCampPlayer = GetSpawnByID(psTarget->SpawnID);
                }
                if (!pCampPlayer)
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Invalid player name and do not have a valid player target.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                if (ME->IsMe(pCampPlayer))
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) You cannot makecamp yourself.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
                // if we made it this far, pCampPlayer is valid
                CAMP->ActivatePC(pCampPlayer);
            }
            else if (!_strnicmp(szCurrentArg, "returnhavetarget", 17))
            {
                CURCAMP->HaveTarget = true;
            }
            else if (!_strnicmp(szCurrentArg, "returnnoaggro", 14))
            {
                CURCAMP->NoAggro = true;
            }
            else if (!_strnicmp(szCurrentArg, "returnnotlooting", 17))
            {
                CURCAMP->NotLoot = true;
            }
            else if (!_strnicmp(szCurrentArg, "realtimeplayer", 15))
            {
                CURCAMP->Realtime = true;
            }
            else if (!_strnicmp(szCurrentArg, "scatter", 8))
            {
                CURCAMP->Scatter = true;
            }
            else if (!_strnicmp(szCurrentArg, "bearing", 8))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CURCAMP->Bearing = (float)atof(szCurrentArg);
                }
                else
                {
                    CAMP->NewCamp(false);
                    sprintf_s(szMsg, "\ay%s\aw:: \armakecamp bearing parameter must be a number.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "scatsize", 9))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    CURCAMP->ScatSize = (float)atof(szCurrentArg);
                }
                else
                {
                    CAMP->NewCamp(false);
                    sprintf_s(szMsg, "\ay%s\aw:: \armakecamp scatsize must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else if (!_strnicmp(szCurrentArg, "scatdist", 9))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if ((float)atof(szCurrentArg) > 1.0f)
                {
                    CURCAMP->ScatDist = (float)atof(szCurrentArg);
                }
                else
                {
                    CAMP->NewCamp(false);
                    sprintf_s(szMsg, "\ay%s\aw:: \armakecamp scatdist must be 1.0 or larger.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else
            {
                // reset camp on failed input?
                if (uiVerbLevel != 0 && (uiVerbLevel & V_HIDEHELP) != V_HIDEHELP) OutputHelp(ucCmdUsed, true);
                return;
            }
        }
        // circle specific parameters
        else if (ucCmdUsed == CMD_CIRCLE)
        {
            if (!_strnicmp(szCurrentArg, "drunken", 8))
            {
                CIRCLE->Drunk = true;
            }
            else if (!_strnicmp(szCurrentArg, "clockwise", 10) || !_strnicmp(szCurrentArg, "cw", 3))
            {
                CIRCLE->CCW = false;
            }
            else if (!_strnicmp(szCurrentArg, "ccw", 4) || !_strnicmp(szCurrentArg, "counterclockwise", 17) || !_strnicmp(szCurrentArg, "reverse", 8))
            {
                CIRCLE->CCW = true;
            }
            else if (!_strnicmp(szCurrentArg, "forward", 7)) // allow 's'
            {
                CIRCLE->Backward = false;
            }
            else if (!_strnicmp(szCurrentArg, "backward", 8)) // allow 's'
            {
                CIRCLE->Backward = true;
            }
            else if (!_strnicmp(szCurrentArg, "radius", 7))
            {
                GetArg(szCurrentArg, szInput, uiArgNum++);
                if (isdigit(szCurrentArg[0]))
                {
                    CIRCLE->SetRadius((float)atof(szCurrentArg));
                    GetArg(szCurrentArg, szInput, uiArgNum);
                }
                else
                {
                    sprintf_s(szMsg, "\ay%s\aw:: (\arERROR\ax) Use \ay/circle radius [#]\ax to set radius.", PLUGIN_NAME);
                    WriteLine(szMsg, V_ERRORS);
                    return;
                }
            }
            else
            {
                EndPreviousCmd(true);
                if (uiVerbLevel != 0 && (uiVerbLevel & V_HIDEHELP) != V_HIDEHELP) OutputHelp(ucCmdUsed, true);
                return;
            }
        }
        // no valid parameter given
        else
        {
            EndPreviousCmd(true);
            if (uiVerbLevel != 0 && (uiVerbLevel & V_HIDEHELP) != V_HIDEHELP) OutputHelp(ucCmdUsed);
            return;
        }
        GetArg(szCurrentArg, szInput, uiArgNum++);
    }

    // Output Messages
    char szTempHead[50] = {};
    switch (pMU->Head)
    {
    case H_TRUE:
        sprintf_s(szTempHead, "\agTrue\ax");
        break;
    case H_LOOSE:
        sprintf_s(szTempHead, "\ayLoose\ax");
        break;
    case H_FAST:
    default:
        sprintf_s(szTempHead, "\arFast\ax");
        break;
    }

    if (ucCmdUsed == CMD_STICK)
    {
        if (!STICK->Hold)
        {
            if (!psTarget)
            {
                // dont continue if no target unless 'always' (returns) or 'id || hold'
                EndPreviousCmd(true);
                SpewMUError(ERR_STICKNONE);
                return;
            }
            // if self targeted and not 'always' or 'id || hold'
            if (ME->IsMe(psTarget))
            {
                EndPreviousCmd(true);
                SpewMUError(ERR_STICKSELF);
                return;
            }
            // else setup output msg
            sprintf_s(szTempID, "%s", psTarget->DisplayedName);
        }
        else if (pTargetUsed || (STICK->Hold && psTarget))
        {
            // setup output msg for 'id || hold'
            sprintf_s(szTempID, "%s", pTargetUsed->DisplayedName);
        }
        else
        {
            // error checking in command line parsing should prevent this from ever happening
            // if user reports this error, needs investigation
            EndPreviousCmd(true);
            char szTempOut[MAX_STRING] = {};
            sprintf_s(szTempOut, "\ay%s\aw:: \ar/stick NO TARGET ERROR", PLUGIN_NAME);
            WriteLine(szTempOut, V_SILENCE);
            return;
        }

        // randomize arcs
        STICK->DoRandomize();

        char szDir[25] = "\agAny\ax";
        if (STICK->Behind)
        {
            sprintf_s(szDir, "\ayBehind\ax");
        }
        else if (STICK->Pin)
        {
            sprintf_s(szDir, "\aySide\ax");
        }
        else if (STICK->NotFront)
        {
            sprintf_s(szDir, "\ayNot Front\ax");
        }
        else if (STICK->Front)
        {
            sprintf_s(szDir, "\ayFront\ax");
        }

        char szTempHold[25] = {};
        if (STICK->Hold && pTargetUsed) sprintf_s(szTempHold, " ID(\ay%d\ax)", pTargetUsed->SpawnID);
        char szTempMod[50] = {};
        bool bOutputMod = false;
        if (STICK->DistMod != 0.0f)
        {
            bOutputMod = true;
            if (STICK->DistModP != 1.0f)
            {
                sprintf_s(szTempMod, "Mod(\ag%.2f\ax | \ag%.2f%%\ax) ", STICK->DistMod, STICK->DistModP);
            }
            else
            {
                sprintf_s(szTempMod, "Mod(\ag%.2f\ax) ", STICK->DistMod);
            }
        }
        else if (STICK->DistModP != 1.0f)
        {
            bOutputMod = true;
            sprintf_s(szTempMod, "Mod(\ag%.2f%%\ax) ", STICK->DistModP);
        }

        if (STICK->Behind || STICK->BehindOnce || STICK->Pin || STICK->NotFront) STICK->Strafe = true;
        sprintf_s(szMsg, "\ay%s\aw:: You are now sticking to %s.", PLUGIN_NAME, szTempID);
        WriteLine(szMsg, V_STICKV);
        sprintf_s(szMsg, "\ay%s\aw:: Dir(%s) Dist(\ag%.2f\ax) %sHead(%s)%s%s%s%s", PLUGIN_NAME, szDir, STICK->Dist, bOutputMod ? szTempMod : "", szTempHead, STICK->Hold ? szTempHold : "", STICK->UW ? " \agUW\ax" : "", STICK->MoveBack ? " \agMB\ax" : "", STICK->Healer ? " \agHEALER\ax" : "");
        WriteLine(szMsg, V_STICKFV);
    }
    else if (ucCmdUsed == CMD_MOVETO)
    {
        char szInfoY[35] = {};
        char szInfoX[35] = {};
        char szZInfo[35] = {};
        sprintf_s(szZInfo, " %.2f.", MOVETO->Z);
        sprintf_s(szInfoY, " YDist(\ag%.2f\ax)", MOVETO->DistY);
        sprintf_s(szInfoX, " XDist(\ag%.2f\ax)", MOVETO->DistX);
        sprintf_s(szMsg, "\ay%s\aw:: Moving to loc %.2f %.2f%s  Dist(\ag%.2f\ax) Head(%s)%s", PLUGIN_NAME, MOVETO->Y, MOVETO->X, MOVETO->Z != 0.0f ? szZInfo : ".", MOVETO->Dist, szTempHead, MOVETO->PreciseY ? szInfoY : (MOVETO->PreciseX ? szInfoX : ""));
        WriteLine(szMsg, V_MOVETOV);
    }
    else if (ucCmdUsed == CMD_CIRCLE)
    {
        sprintf_s(szMsg, " \ay%s\aw:: Circling radius (\ag%.2f\ax), center (\ag%.2f\ax, \ag%.2f\ax)%s%s%s - Head(%s) : %s", PLUGIN_NAME, CIRCLE->Radius, CIRCLE->Y, CIRCLE->X, CIRCLE->CCW ? ", Reverse" : "", CIRCLE->Backward ? ", Backwards" : "", CIRCLE->Drunk ? ", \agDrunken\ax" : "", szTempHead, CIRCLE->On ? "\agON" : "\arOFF");
        WriteLine(szMsg, V_CIRCLEV);
    }
    else if (ucCmdUsed == CMD_MAKECAMP)
    {
        if (!CURCAMP->Pc)
        {
            sprintf_s(szMsg, "\ay%s\aw:: MakeCamp (%s). Y(\ag%.2f\ax) X(\ag%.2f\ax) Radius(\ag%.2f\ax) Leash(%s - \ag%.2f\ax) Delay(\ag%d\ax to \ag%d\ax)", PLUGIN_NAME, CURCAMP->On ? "\agon\ax" : "\aroff\ax", CURCAMP->Y, CURCAMP->X, CURCAMP->Radius, CURCAMP->Leash ? "\agon\ax" : "\aroff\ax", CURCAMP->Length, CAMP->Min, CAMP->Max);
        }
        else
        {
            sprintf_s(szMsg, "\ay%s\aw:: MakeCamp Player (\ag%s\ax). Radius(\ag%.2f\ax) Leash(%s - \ag%.2f\ax) Delay(\ag%d\ax to \ag%d\ax)", PLUGIN_NAME, CURCAMP->PcName, CURCAMP->Radius, CURCAMP->Leash ? "\agon\ax" : "\aroff\ax", CURCAMP->Length, CAMP->Min, CAMP->Max);
        }
        WriteLine(szMsg, V_MAKECAMPV);
        return; // return so makecamp doesnt stand up
    }

    bWrapped = false;
    MOVE->DoStand();
}


// exported wrapper for MQ2Melee support
void StickCommand(PlayerClient* pLPlayer, char* szLine)
{
    bWrapped = true;
    HandleOurCmd(CMD_STICK, szLine);
}

// main command wrappers
void StickWrapper(PlayerClient* pLPlayer, char* szLine)
{
    HandleOurCmd(CMD_STICK, szLine);
    bRunNextCommand = true;
}
void MoveToWrapper(PlayerClient* pLPlayer, char* szLine)
{
    HandleOurCmd(CMD_MOVETO, szLine);
    bRunNextCommand = true;
}
void CircleWrapper(PlayerClient* pLPlayer, char* szLine)
{
    HandleOurCmd(CMD_CIRCLE, szLine);
    bRunNextCommand = true;
}
void MakeCampWrapper(PlayerClient* pLPlayer, char* szLine)
{
    HandleOurCmd(CMD_MAKECAMP, szLine);
    bRunNextCommand = true;
}

// calcangle command
void CalcOurAngle(PlayerClient* pLPlayer, char* szLine)
{
    if (!ValidIngame() || !pTarget) return;
    PlayerClient* psTarget = pTarget;
    PlayerClient* pChSpawn = pCharSpawn;
    float fAngle   = MOVE->AngDist(psTarget->Heading, pChSpawn->Heading);
    float fReqHead = MOVE->SaneHead(atan2(psTarget->X - pChSpawn->X, psTarget->Y - pChSpawn->Y) * HEADING_HALF / (float)PI);
    fReqHead = pChSpawn->Heading - fReqHead;
    float fMeleeRng = get_melee_range(pLocalPlayer, psTarget);
    float fStickRng = fMeleeRng * STICK->DistModP + STICK->DistMod;
    float fSaneH = MOVE->SaneHead(fReqHead);
    float fDist   = GetDistance(pChSpawn, psTarget);
    float fDist3D = GetDistance3D(pChSpawn->X, pChSpawn->Y, pChSpawn->Z, psTarget->X, psTarget->Y, psTarget->Z);
    char szTempOut[MAX_STRING] = {};
    sprintf_s(szTempOut, "\ay%s\aw:: AngularDist ( \ag%.2f\ax ) Adjust ( \ag%.2f\ax ) Sane ( \ag%.2f\ax ) Dist ( \ag%.2f\ax ) Dist3D ( \ag%.2f\ax ) Melee ( \ag%f\ax ) Stick ( \ag%f\ax)", PLUGIN_NAME, fAngle, fReqHead, fSaneH, fDist, fDist3D, fMeleeRng, fStickRng);
    WriteChatf("%s",szTempOut);
    sprintf_s(szTempOut, " - Walking ( %s )  RunSpeed ( \ag%.2f\ax ) SpeedMultiplier ( \ag%.2f\ax )", (*EQADDR_RUNWALKSTATE) ? "\arno\ax" : "\agyes\ax", pChSpawn->RunSpeed, pChSpawn->SpeedMultiplier);
    WriteChatf("%s",szTempOut);
}

// root command
void RootCmd(PlayerClient* pLPlayer, char* szLine)
{
    if (!ValidIngame()) return;
    char szTempOut[MAX_STRING] = {};

    if (SET->WinEQ)
    {
        sprintf_s(szTempOut, "\ay%s\aw:: \arUnable to use this command due to use of old movement type.", PLUGIN_NAME);
        WriteLine(szTempOut, V_SILENCE);
        return;
    }

    char szArg1[MAX_STRING]    = {};
    GetArg(szArg1, szLine, 1);
    if (!_strnicmp(szArg1, "off", 4))
    {
        MOVE->StopRoot();
    }
    else
    {
        EndPreviousCmd(true);
        CAMP->NewCamp(CURCAMP->On);
        pMU->Rooted    = true;
        MOVE->RootHead = pCharSpawn->Heading;
        sprintf_s(szTempOut, "\ay%s\aw:: You are now rooted in place.", PLUGIN_NAME);
        WriteLine(szTempOut, V_SILENCE);
    }
}

// End user command input handling
// ----------------------------------------
// settings - saved to INI

bool ToggleSetting(const char* pszToggleOutput, bool* pbEvalThis, bool* pbUsedToggle, bool* pbTurnOn)
{
    char szTheMsg[MAX_STRING] = {};

    // setting we are changing = did we use 'toggle' ? setting toggled ELSE set to 'on' or 'off'
    *pbEvalThis = *pbUsedToggle ? !*pbEvalThis : *pbTurnOn;
    sprintf_s(szTheMsg, "\ay%s\aw:: %s turned %s", PLUGIN_NAME, pszToggleOutput, *pbEvalThis ? szOn : szOff);
    WriteLine(szTheMsg, V_SETTINGS);
    return *pbEvalThis; // changesetting is only evaluating
}

void SaveConfigSetting(BYTE ucCmdUsed)
{
    const char* szSection = "";
    switch(ucCmdUsed)
    {
    case CMD_MAKECAMP:
        szSection = "MakeCamp";
        WritePrivateProfileValue(szSection, "CampRadius",             SET_CAMP->Radius,       INIFileName);
        WritePrivateProfileValue(szSection, "MinDelay",               SET_CAMP->Min,          INIFileName);
        WritePrivateProfileValue(szSection, "MaxDelay",               SET_CAMP->Max,          INIFileName);
        WritePrivateProfileValue(szSection, "RealtimePlayer",         SET_CAMP->Realtime,     INIFileName);
        WritePrivateProfileValue(szSection, "ReturnHaveTarget",       SET_CAMP->HaveTarget,   INIFileName);
        WritePrivateProfileValue(szSection, "ReturnNoAggro",          SET_CAMP->NoAggro,      INIFileName);
        WritePrivateProfileValue(szSection, "ReturnNotLooting",       SET_CAMP->NotLoot,      INIFileName);
        WritePrivateProfileValue(szSection, "UseLeash",               SET_CAMP->Leash,        INIFileName);
        WritePrivateProfileValue(szSection, "LeashLength",            SET_CAMP->Length,       INIFileName);
        WritePrivateProfileValue(szSection, "UseScatter",             SET_CAMP->Scatter,      INIFileName);
        WritePrivateProfileValue(szSection, "Bearing",                SET_CAMP->Bearing,      INIFileName);
        WritePrivateProfileValue(szSection, "ScatDist",               SET_CAMP->ScatDist,     INIFileName);
        WritePrivateProfileValue(szSection, "ScatSize",               SET_CAMP->ScatSize,     INIFileName);
        break;

    case CMD_STICK:
        szSection = "Stick";
        WritePrivateProfileValue(szSection, "AlwaysUW",               SET_S->UW,              INIFileName);
        WritePrivateProfileValue(szSection, "AwareNotAggro",          SET->Spin,              INIFileName);
        WritePrivateProfileValue(szSection, "ArcBehind",              SET_S->ArcBehind,       INIFileName);
        WritePrivateProfileValue(szSection, "ArcNotFront",            SET_S->ArcNotFront,     INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnGate",            SET_S->BreakGate,       INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnHit",             SET_S->BreakHit,        INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnTarget",          SET_S->BreakTarget,     INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnWarp",            SET_S->BreakWarp,       INIFileName);
        WritePrivateProfileValue(szSection, "PauseOnWarp",            SET_S->PauseWarp,       INIFileName);
        WritePrivateProfileValue(szSection, "DelayStrafe",            SET_S->DelayStrafe,     INIFileName);
        WritePrivateProfileValue(szSection, "DistBackup",             SET_S->DistBack,        INIFileName);
        WritePrivateProfileValue(szSection, "DistBreak",              SET_S->DistBreak,       INIFileName);
        WritePrivateProfileValue(szSection, "DistFlex",               SET_S->DistFlex,        INIFileName);
        WritePrivateProfileValue(szSection, "DistMod",                SET_S->DistMod,         INIFileName);
        WritePrivateProfileValue(szSection, "DistMod%",               SET_S->DistModP,        INIFileName);
        WritePrivateProfileValue(szSection, "DistSnaproll",           SET_S->DistSnap,        INIFileName);
        WritePrivateProfileValue(szSection, "RandomArc",              SET_S->Randomize,       INIFileName);
        WritePrivateProfileValue(szSection, "StrafeMinDelay",         SET_S->Min,             INIFileName);
        WritePrivateProfileValue(szSection, "StrafeMaxDelay",         SET_S->Max,             INIFileName);
        WritePrivateProfileValue(szSection, "UseBackward",            SET_S->UseBack,         INIFileName);
        WritePrivateProfileValue(szSection, "UseFleeing",             SET_S->UseFleeing,      INIFileName);
        WritePrivateProfileValue(szSection, "UseFlex",                SET_S->Flex,            INIFileName);
        WritePrivateProfileValue(szSection, "UseWalk",                SET_S->Walk,            INIFileName);
        break;

    case CMD_MOVETO:
        szSection = "MoveTo";
        WritePrivateProfileValue(szSection, "AlwaysUW",               SET_M->UW,              INIFileName);
        WritePrivateProfileValue(szSection, "ArrivalDist",            SET_M->Dist,            INIFileName);
        WritePrivateProfileValue(szSection, "ArrivalDistX",           SET_M->DistX,           INIFileName);
        WritePrivateProfileValue(szSection, "ArrivalDistY",           SET_M->DistY,           INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnAggro",           SET_M->BreakAggro,      INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnHit",             SET_M->BreakHit,        INIFileName);
        WritePrivateProfileValue(szSection, "DistBackup",             SET_M->DistBack,        INIFileName);
        WritePrivateProfileValue(szSection, "MoveToMod",              SET_M->Mod,             INIFileName);
        WritePrivateProfileValue(szSection, "UseBackward",            SET_M->UseBack,         INIFileName);
        WritePrivateProfileValue(szSection, "UseWalk",                SET_M->Walk,            INIFileName);
        break;

    case CMD_CIRCLE:
        szSection = "Circle";
        WritePrivateProfileValue(szSection, "Backward",               SET_C->Backward,        INIFileName);
        WritePrivateProfileValue(szSection, "CCW",                    SET_C->CCW,             INIFileName);
        WritePrivateProfileValue(szSection, "Drunken",                SET_C->Drunk,           INIFileName);
        WritePrivateProfileValue(szSection, "RadiusSize",             SET_C->Radius,          INIFileName);
        break;

    default:
        szSection = "Defaults";
        WritePrivateProfileValue(szSection, "AllowMove",              SET->AllowMove,         INIFileName);
        WritePrivateProfileValue(szSection, "AutoPause",              SET->AutoPause,         INIFileName);
        WritePrivateProfileValue(szSection, "AutoPauseMsg",           (uiVerbLevel & V_AUTOPAUSE) == V_AUTOPAUSE, INIFileName);
        WritePrivateProfileValue(szSection, "AutoSave",               SET->AutoSave,          INIFileName);
        WritePrivateProfileValue(szSection, "AutoUW",                 SET->AutoUW,            INIFileName);
        WritePrivateProfileValue(szSection, "BreakKeyboard",          SET->BreakKB,           INIFileName);
        WritePrivateProfileValue(szSection, "BreakMouse",             SET->BreakMouse,        INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnGM",              SET->BreakGM,           INIFileName);
        WritePrivateProfileValue(szSection, "BreakOnSummon",          SET->BreakSummon,       INIFileName);
        WritePrivateProfileValue(szSection, "DistSummon",             SET->DistSummon,        INIFileName);
        WritePrivateProfileValue(szSection, "FeignSupport",           SET->Feign,             INIFileName);
        WritePrivateProfileValue(szSection, "Heading",                SET->Head == H_TRUE ? "true" : SET->Head == H_LOOSE ? "loose" : "fast", INIFileName);
        WritePrivateProfileValue(szSection, "HideHelp",               (uiVerbLevel & V_HIDEHELP) == V_HIDEHELP, INIFileName);
        WritePrivateProfileValue(szSection, "KeyboardPause",          SET->PauseKB,           INIFileName);
        WritePrivateProfileValue(szSection, "MousePause",             SET->PauseMouse,        INIFileName);
        WritePrivateProfileValue(szSection, "LockPause",              SET->LockPause,         INIFileName);
        WritePrivateProfileValue(szSection, "PauseMinDelay",          PAUSE->Min,             INIFileName);
        WritePrivateProfileValue(szSection, "PauseMaxDelay",          PAUSE->Max,             INIFileName);
        WritePrivateProfileValue(szSection, "SaveByChar",             SET->SaveByChar,        INIFileName);
        WritePrivateProfileValue(szSection, "TurnRate",               SET->TurnRate,          INIFileName);
        WritePrivateProfileValue(szSection, "UseWindow",              SET->Window,            INIFileName);
        WritePrivateProfileValue(szSection, "Verbosity",              (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY, INIFileName);
        // FullVerbosity is more frequent, detailed output, and differs from Verbosity
        // Setting one does not include the text of the other.
        WritePrivateProfileValue(szSection, "FullVerbosity",          (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY,INIFileName);
        // Total Silence writes no output except critical or user-requested
        WritePrivateProfileValue(szSection, "TotalSilence",           uiVerbLevel == 0,       INIFileName);
        WritePrivateProfileValue(szSection, "VerbosityFlags",         uiVerbLevel,            INIFileName);
        WritePrivateProfileValue(szSection, "WinEQ",                  SET->WinEQ,             INIFileName);
        break;
    }
    return;

	//can do the rest later this function is incredibly expensive and slow, need to make it only save what has changed.
    // stucklogic
    szSection = "StuckLogic";
    WritePrivateProfileValue(szSection, "StuckLogic",             STUCK->On,              INIFileName);
    WritePrivateProfileValue(szSection, "DistStuck",              STUCK->Dist,            INIFileName);
    WritePrivateProfileValue(szSection, "PulseCheck",             (int)STUCK->Check,      INIFileName);
    WritePrivateProfileValue(szSection, "PulseUnstuck",           (int)STUCK->Unstuck,    INIFileName);
    WritePrivateProfileValue(szSection, "TryToJump",              STUCK->Jump,            INIFileName);
    WritePrivateProfileValue(szSection, "TurnHalf",               STUCK->TurnHalf,        INIFileName);

    // check if we want to explicitly ignore this chars custom section
    bool ignoreMe = GetPrivateProfileBool(szCharName, "DisregardMe", false, INIFileName);
    if (SET->SaveByChar && !ignoreMe)
    {
        // Character specific
        WritePrivateProfileValue(szCharName, "AllowMove",      SET->AllowMove,            INIFileName);
        WritePrivateProfileValue(szCharName, "ArcBehind",      SET_S->ArcBehind,          INIFileName);
        WritePrivateProfileValue(szCharName, "ArcNotFront",    SET_S->ArcNotFront,        INIFileName);
        WritePrivateProfileValue(szCharName, "AutoSave",       SET->AutoSave,             INIFileName);
        WritePrivateProfileValue(szCharName, "AutoUW",         SET->AutoUW,               INIFileName);
        WritePrivateProfileValue(szCharName, "DistBreak",      SET_S->DistBreak,          INIFileName);
        WritePrivateProfileValue(szCharName, "BreakOnGate",    SET_S->BreakGate,          INIFileName);
        WritePrivateProfileValue(szCharName, "BreakOnWarp",    SET_S->BreakWarp,          INIFileName);
        WritePrivateProfileValue(szCharName, "PauseOnWarp",    SET_S->PauseWarp,          INIFileName);
        WritePrivateProfileValue(szCharName, "LockPause",      SET->LockPause,            INIFileName);
        WritePrivateProfileValue(szCharName, "DistSnaproll",   SET_S->DistSnap,           INIFileName);
        WritePrivateProfileValue(szCharName, "FeignSupport",   SET->Feign,                INIFileName);
        WritePrivateProfileValue(szCharName, "Heading",        SET->Head == H_TRUE ? "true" : SET->Head == H_LOOSE ? "loose" : "fast", INIFileName);
        WritePrivateProfileValue(szCharName, "LeashLength",    SET_CAMP->Length,          INIFileName);
        WritePrivateProfileValue(szCharName, "UseLeash",       SET_CAMP->Leash,           INIFileName);
        WritePrivateProfileValue(szCharName, "UseWindow",      SET->Window,               INIFileName);
        WritePrivateProfileValue(szCharName, "Verbosity",      (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY, INIFileName);
        WritePrivateProfileValue(szCharName, "FullVerbosity",  (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY, INIFileName);
        WritePrivateProfileValue(szCharName, "VerbosityFlags", uiVerbLevel,               INIFileName);
        WritePrivateProfileValue(szCharName, "CampRadius",     SET_CAMP->Radius,          INIFileName);
        WritePrivateProfileValue(szCharName, "RealtimePlayer", SET_CAMP->Realtime,        INIFileName);
        // scatter values
        WritePrivateProfileValue(szCharName, "UseScatter",     SET_CAMP->Scatter,         INIFileName);
        WritePrivateProfileValue(szCharName, "Bearing",        SET_CAMP->Bearing,         INIFileName);
        WritePrivateProfileValue(szCharName, "ScatDist",       SET_CAMP->ScatDist,        INIFileName);
        WritePrivateProfileValue(szCharName, "ScatSize",       SET_CAMP->ScatSize,        INIFileName);
    }
}

void ChangeSetting(unsigned char ucCmdUsed, bool bToggle, char szSetting[MAX_STRING])
{
    char szCommand[MAX_STRING] = {};
    switch(ucCmdUsed)
    {
    case CMD_MAKECAMP:
        sprintf_s(szCommand, "/makecamp");
        break;
    case CMD_STICK:
        sprintf_s(szCommand, "/stick");
        break;
    case CMD_MOVETO:
        sprintf_s(szCommand, "/moveto");
        break;
    case CMD_CIRCLE:
        sprintf_s(szCommand, "/circle");
        break;
    default:
        return;
    }

    char szParameter[MAX_STRING] = {};
    char szSetState[MAX_STRING]  = {};
    char szSetDigit[MAX_STRING]  = {};
    char szSetError[MAX_STRING]  = {};
    bool bTurnOn                 = false;
    bool bSetDigit               = false;
    bool bCustomMsg              = false;
    unsigned int uiArgNum        = 1;

    GetArg(szParameter, szSetting, uiArgNum++);

    // check for valid parameter if "set" used (on, off, number)
    if (!bToggle)
    {
        GetArg(szSetState, szSetting, uiArgNum);
        if (isdigit(szSetState[0]) || szSetState[0] == '.' || szSetState[0] == '-')
        {
            bSetDigit = true;
            sprintf_s(szSetDigit, szSetState); // for naming clarification only, waste ram ftw.
        }
        else if (!_strnicmp(szSetState, "on", 3))
        {
            bTurnOn = true;
        }
        else if (!_strnicmp(szSetState, "off", 4))
        {
            bTurnOn = false; // serves no point other than to confirm valid input
        }
        else if (!_strnicmp(szSetting, "heading", 7))
        {
            if (!_strnicmp(szSetState, "true", 5))
            {
                pMU->Head = SET->Head = H_TRUE;
                sprintf_s(szMsg, "\ay%s\aw:: Heading adjustment type set to: \agtrue", PLUGIN_NAME);
            }
            else if (!_strnicmp(szSetState, "loose", 6))
            {
                pMU->Head = SET->Head = H_LOOSE;
                sprintf_s(szMsg, "\ay%s\aw:: Heading adjustment type set to: \ayloose", PLUGIN_NAME);
            }
            else if (!_strnicmp(szSetState, "fast", 5))
            {
                pMU->Head = SET->Head = H_FAST;
                sprintf_s(szMsg, "\ay%s\aw:: Heading adjustment type set to: \arfast", PLUGIN_NAME);
            }
            else
            {
                sprintf_s(szSetError, "\ay%s\aw:: \arERROR\ax: Invalid '%s set' syntax ( \ar%s\ax ) [true|loose|fast]", PLUGIN_NAME, szCommand, szParameter);
                WriteLine(szSetError, V_ERRORS);
                return;
            }
            WriteLine(szMsg, V_SETTINGS);
            if (SET->AutoSave) {
                SaveConfig();
            }
            return;
        }
        else
        {
            sprintf_s(szSetError, "\ay%s\aw:: \arERROR\ax: Invalid '%s set' syntax ( \ar%s\ax ) [on|off|number]", PLUGIN_NAME, szCommand, szParameter);
            WriteLine(szSetError, V_ERRORS);
            return;
        }
    }

    if (!bCustomMsg && (bToggle || !bSetDigit))
    {
        if (!_strnicmp(szParameter, "mpause", 7))
        {
            if (ToggleSetting("Pause from manual movement", &SET->PauseKB, &bToggle, &bTurnOn))
            {
                SET->BreakKB = false;
            }
        }
        else if (!_strnicmp(szParameter, "mousepause", 11))
        {
            if (ToggleSetting("Pause from mouse movement", &SET->PauseMouse, &bToggle, &bTurnOn))
            {
                SET->BreakMouse = false;
            }
        }
        else if (!_strnicmp(szParameter, "breakonkb", 10))
        {
            if (ToggleSetting("Break from keyboard movement", &SET->BreakKB, &bToggle, &bTurnOn))
            {
                SET->PauseKB = false;
            }
        }
        else if (!_strnicmp(szParameter, "breakonmouse", 13))
        {
            if (ToggleSetting("Break from mouse movement", &SET->BreakMouse, &bToggle, &bTurnOn))
            {
                SET->PauseMouse = false;
            }
        }
        else if (!_strnicmp(szParameter, "autosave", 9))
        {
            ToggleSetting("Auto-save settings to INI file", &SET->AutoSave, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "savebychar", 11))
        {
            ToggleSetting("Save INI file Character Name section", &SET->SaveByChar, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "feign", 6))
        {
            ToggleSetting("Remain feign support", &SET->Feign, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "lockpause", 10))
        {
            ToggleSetting("Prevent command usage breaking pause", &SET->LockPause, &bToggle, &bTurnOn);
            pMU->LockPause = SET->LockPause;
        }
        else if (!_strnicmp(szParameter, "autopause", 10))
        {
            ToggleSetting("AutoPause upon spell cast", &SET->AutoPause, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "autopauseoutput", 16))
        {
            if (bToggle)
            {
                uiVerbLevel ^= V_AUTOPAUSE;
            }
            else if (bTurnOn)
            {
                uiVerbLevel |= V_AUTOPAUSE;
            }
            else
            {
                uiVerbLevel &= ~V_AUTOPAUSE;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Display AutoPause output: %s", PLUGIN_NAME, (uiVerbLevel & V_AUTOPAUSE) == V_AUTOPAUSE ? szOn : szOff);
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "stucklogic", 13))
        {
            ToggleSetting("Stuck-checking logic", &STUCK->On, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "trytojump", 10))
        {
            ToggleSetting("Try to jump when stuck", &STUCK->Jump, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "turnhalf", 9))
        {
            ToggleSetting("Switch direction if turned halfway (stucklogic)", &STUCK->TurnHalf, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "verbosity", 10))
        {
            if (bToggle)
            {
                uiVerbLevel ^= V_VERBOSITY;
            }
            else if (bTurnOn)
            {
                uiVerbLevel |= V_VERBOSITY;
            }
            else
            {
                uiVerbLevel &= ~V_VERBOSITY;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Basic verbosity turned: %s", PLUGIN_NAME, (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY ? szOn : szOff);
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "fullverbosity", 14))
        {
            if (bToggle)
            {
                uiVerbLevel ^= V_FULLVERBOSITY;
            }
            else if (bTurnOn)
            {
                uiVerbLevel |= V_FULLVERBOSITY;
            }
            else
            {
                uiVerbLevel &= ~V_FULLVERBOSITY;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Enhanced verbosity turned: %s", PLUGIN_NAME, (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY ? szOn : szOff);
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "totalsilence", 13))
        {
            if (bToggle)
            {
                if (uiVerbLevel != V_SILENCE)
                {
                    uiRetainFlags = uiVerbLevel;
                    uiVerbLevel = V_SILENCE;
                }
                else
                {
                    uiVerbLevel = uiRetainFlags;
                }
            }
            else if (bTurnOn)
            {
                uiRetainFlags = uiVerbLevel;
                uiVerbLevel = V_SILENCE;
            }
            else
            {
                uiVerbLevel = uiRetainFlags;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Plugin silence turned: %s", PLUGIN_NAME, (uiVerbLevel == V_SILENCE) ? szOn : szOff);
            WriteLine(szMsg, V_SILENCE);
        }
        else if (!_strnicmp(szParameter, "totalverbosity", 15))
        {
            if (bToggle)
            {
                if (uiVerbLevel != V_EVERYTHING)
                {
                    uiRetainFlags = uiVerbLevel;
                    uiVerbLevel = V_EVERYTHING;
                }
                else
                {
                    uiVerbLevel = uiRetainFlags;
                }
            }
            else if (bTurnOn)
            {
                uiRetainFlags = uiVerbLevel;
                uiVerbLevel = V_EVERYTHING;
            }
            else
            {
                uiVerbLevel = uiRetainFlags;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Plugin total verbosity turned: %s", PLUGIN_NAME, (uiVerbLevel == V_EVERYTHING) ? szOn : szOff);
            WriteLine(szMsg, V_SILENCE);
        }
        else if (!_strnicmp(szParameter, "window", 7))
        {
            if (ToggleSetting("Dedicated UI Window", &SET->Window, &bToggle, &bTurnOn))
            {
                WINDOW->Create();
            }
            else
            {
                WINDOW->Destroy(true);
            }
        }
        else if (!_strnicmp(szParameter, "wineq", 6))
        {
            ToggleSetting("Use old-style movement", &SET->WinEQ, &bToggle, &bTurnOn);
            if (SET->WinEQ)
            {
                if (SET->Head == H_TRUE) SET->Head = H_LOOSE;
                if (pMU->Head == H_TRUE) pMU->Head = H_LOOSE;
            }
        }
        else if (!_strnicmp(szParameter, "hidehelp", 9))
        {
            if (bToggle)
            {
                uiVerbLevel ^= V_HIDEHELP;
            }
            else if (bTurnOn)
            {
                uiVerbLevel |= V_HIDEHELP;
            }
            else
            {
                uiVerbLevel &= ~V_HIDEHELP;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Hide automatic help output: %s", PLUGIN_NAME, (uiVerbLevel & V_HIDEHELP) == V_HIDEHELP ? szOn : szOff);
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "breakontarget", 14))
        {
            STICK->BreakTarget = ToggleSetting("Break stick on target change", &SET_S->BreakTarget, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "breakonsummon", 14))
        {
            ToggleSetting("Break command when summoned", &SET->BreakSummon, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "breakongm", 10))
        {
            ToggleSetting("Break command when visible GM enters", &SET->BreakGM, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "breakonwarp", 12))
        {
            if (ToggleSetting("Break command when NPC warps away", &SET_S->BreakWarp, &bToggle, &bTurnOn))
            {
                SET_S->PauseWarp = STICK->PauseWarp = false;
            }
            STICK->BreakWarp = SET_S->BreakWarp;
        }
        else if (!_strnicmp(szParameter, "pauseonwarp", 12))
        {
            if (ToggleSetting("Pause command when NPC warps away", &SET_S->PauseWarp, &bToggle, &bTurnOn))
            {
                SET_S->PauseWarp = STICK->BreakWarp = false;
            }
            STICK->PauseWarp = SET_S->PauseWarp;
        }
        else if (!_strnicmp(szParameter, "breakongate", 12))
        {
            SetupEvents(ToggleSetting("Break command when NPC gates", &SET_S->BreakGate, &bToggle, &bTurnOn));
            STICK->BreakGate = SET_S->BreakGate;
        }
        else if (!_strnicmp(szParameter, "breakonhit", 11))
        {
            if (ucCmdUsed == CMD_MOVETO)
            {
                SetupEvents(ToggleSetting("Break MoveTo if attacked", &SET_M->BreakHit, &bToggle, &bTurnOn));
                MOVETO->BreakHit = SET_M->BreakHit;
            }
            else if (ucCmdUsed == CMD_STICK)
            {
                SetupEvents(ToggleSetting("Break Stick if attacked", &SET_S->BreakHit, &bToggle, &bTurnOn));
                STICK->BreakHit = SET_S->BreakHit;
            }
            else
            {
                sprintf_s(szSetError, "\ay%s\aw:: \arERROR\ax: Not a valid %s %s ( \ar%s\ax ).", PLUGIN_NAME, szCommand, bToggle ? "toggle" : "set option", szParameter);
                WriteLine(szSetError, V_ERRORS);
                return;
            }
        }
        else if (!_strnicmp(szParameter, "breakonaggro", 13))
        {
            SetupEvents(ToggleSetting("Break MoveTo if aggro", &SET_M->BreakAggro, &bToggle, &bTurnOn));
            MOVETO->BreakAggro = SET_M->BreakAggro;
        }
        else if (!_strnicmp(szParameter, "alwaysdrunk", 12))
        {
            CIRCLE->Drunk = ToggleSetting("Circle always drunken", &SET_C->Drunk, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "alwaysbackwards", 16))
        {
            CIRCLE->Backward = ToggleSetting("Circle always backwards", &SET_C->Backward, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "alwaysccw", 10))
        {
            CIRCLE->CCW = ToggleSetting("Circle always counter-clockwise", &SET_C->CCW, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "alwaysuw", 9) && (ucCmdUsed == CMD_MOVETO || ucCmdUsed == CMD_STICK))
        {
            if (ucCmdUsed == CMD_MOVETO)
            {
                MOVETO->UW = ToggleSetting("Always use underwater parameter with /moveto", &SET_M->UW, &bToggle, &bTurnOn);
            }
            else
            {
                STICK->UW = ToggleSetting("Always use underwater parameter with /stick", &SET_S->UW, &bToggle, &bTurnOn);
            }
        }
        else if (!_strnicmp(szParameter, "loose", 6) && ucCmdUsed != CMD_MAKECAMP)
        {
            pMU->Head = bToggle ? (pMU->Head == H_LOOSE ? H_FAST : H_LOOSE) : (bTurnOn ? H_LOOSE : H_FAST);
            sprintf_s(szMsg, "\ay%s\aw:: Active \ay%s\ax loose heading turned %s (Cur: %s)", PLUGIN_NAME, szCommand, (pMU->Head == H_LOOSE) ? szOn : szOff, (pMU->Head == H_LOOSE) ? "\ayloose\ax" : "\arfast\ax");
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "truehead", 9) && ucCmdUsed != CMD_MAKECAMP)
        {
            pMU->Head = bToggle ? (pMU->Head == H_TRUE ? H_FAST : H_TRUE) : (bTurnOn ? H_TRUE : H_FAST);
            sprintf_s(szMsg, "\ay%s\aw:: Active \ay%s\ax true turning turned %s (Cur: %s)", PLUGIN_NAME, szCommand, (pMU->Head == H_TRUE) ? szOn : szOff, (pMU->Head == H_TRUE) ? "\ayloose\ax" : "\arfast\ax");
            WriteLine(szMsg, V_SETTINGS);
        }
        else if (!_strnicmp(szParameter, "nohottfront", 12))
        {
            ToggleSetting("Spin in circles when I lose aggro with no HoTT using '/stick front'", &SET->Spin, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "returnnoaggro", 14))
        {
            CURCAMP->NoAggro = ToggleSetting("Return to camp when not aggro", &SET_CAMP->NoAggro, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "returnnotlooting", 17))
        {
            CURCAMP->NotLoot = ToggleSetting("Return to camp when not looting", &SET_CAMP->NotLoot, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "returnhavetarget", 17))
        {
            CURCAMP->HaveTarget = ToggleSetting("Return to camp regardless of target", &SET_CAMP->HaveTarget, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "realtimeplayer", 15))
        {
            CURCAMP->Realtime = ToggleSetting("Return to realtime camp player location", &SET_CAMP->Realtime, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "leash", 6))
        {
            CURCAMP->Leash = ToggleSetting("Leash to camp", &SET_CAMP->Leash, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "usewalk", 8))
        {
            MOVETO->Walk = ToggleSetting("Walk when close to moveto/camp return destination", &SET_M->Walk, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "strafewalk", 11))
        {
            STICK->Walk = ToggleSetting("Walk when stick is close and strafing", &SET_S->Walk, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "randomize", 10))
        {
            STICK->Randomize = ToggleSetting("Randomize custom arcs", &SET_S->Randomize, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "delaystrafe", 12))
        {
            STICK->DelayStrafe = ToggleSetting("Enable custom stick angle delay", &SET_S->DelayStrafe, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "autouw", 7))
        {
            ToggleSetting("Automatically use 'uw' when underwater", &SET->AutoUW, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "useback", 8) && (ucCmdUsed == CMD_MOVETO || ucCmdUsed == CMD_STICK))
        {
            if (ucCmdUsed == CMD_MOVETO)
            {
                MOVETO->UseBack = ToggleSetting("Use backward movement when close to destination", &SET_M->UseBack, &bToggle, &bTurnOn);
            }
            else
            {
                STICK->UseBack = ToggleSetting("Use backwards movement when close to target", &SET_S->UseBack, &bToggle, &bTurnOn);
            }
        }
        else if (!_strnicmp(szParameter, "usefleeing", 11))
        {
            STICK->UseFleeing = ToggleSetting("Stick front fleeing mob detection", &SET_S->UseFleeing, &bToggle, &bTurnOn);
        }
        else if (!_strnicmp(szParameter, "usescatter", 11))
        {
            bCustomMsg = true;
            SET_CAMP->Scatter = bToggle ? !SET_CAMP->Scatter : bTurnOn;
            CURCAMP->Scatter = SET_CAMP->Scatter;
            sprintf_s(szMsg, "\ay%s\aw:: Scatter camp returns (%s) ScatDist(\ag%.2f\ax) Bearing(\ag%.2f\ax) ScatSize(\ag%.2f\ax)", PLUGIN_NAME, SET_CAMP->Scatter ? szOn : szOff, SET_CAMP->ScatDist, SET_CAMP->Bearing, SET_CAMP->ScatSize);
        }
        else if (!_strnicmp(szParameter, "flex", 5))
        {
            STICK->Flex = ToggleSetting("Flexible stick distance", &SET_S->Flex, &bToggle, &bTurnOn);
        }
        else
        {
            sprintf_s(szSetError, "\ay%s\aw:: \arERROR\ax: Not a valid %s %s ( \ar%s\ax ).", PLUGIN_NAME, szCommand, bToggle ? "toggle" : "set option", szParameter);
            WriteLine(szSetError, V_ERRORS);
            return;
        }

        if (!bCustomMsg)
        {
            if (SET->AutoSave) {
                if (ucCmdUsed == 1 || ucCmdUsed == 2 || ucCmdUsed == 3 || ucCmdUsed == 4)
                    SaveConfigSetting(ucCmdUsed);
                else
                    SaveConfig();
            }
            return;
        }
    }
    else if (!bCustomMsg && bSetDigit)
    {
        float fDigit = (float)atof(szSetDigit);
        if (!_strnicmp(szParameter, "pulsecheck", 11))
        {
            STUCK->Check = (unsigned int)fDigit > 1 ? (unsigned int)fDigit : STUCK->Check;
            sprintf_s(szMsg, "\ay%s\aw:: StuckLogic pulse check rate set to \ag%d\ax pulses.", PLUGIN_NAME, STUCK->Check);
        }
        else if (!_strnicmp(szParameter, "pulseunstuck", 13))
        {
            STUCK->Unstuck = (unsigned int)fDigit > 1 ? (unsigned int)fDigit : STUCK->Unstuck;
            sprintf_s(szMsg, "\ay%s\aw:: StuckLogic pulse unstuck value set to \ag%d\ax pulses.", PLUGIN_NAME, STUCK->Unstuck);
        }
        else if (!_strnicmp(szParameter, "diststuck", 10))
        {
            STUCK->Dist = fDigit > 0.0f ? fDigit : STUCK->Dist;
            sprintf_s(szMsg, "\ay%s\aw:: StuckLogic distance moved per pulse (else stuck) set to \ag%.3f", PLUGIN_NAME, STUCK->Dist);
        }
        else if (!_strnicmp(szParameter, "campmindelay", 13))
        {
            SET_CAMP->MinDelay((int)fDigit);
            CAMP->MinDelay(SET_CAMP->Min);
            sprintf_s(szMsg, "\ay%s\aw:: Mindelay for camp return set to \ag%d", PLUGIN_NAME, SET_CAMP->Min);
        }
        else if (!_strnicmp(szParameter, "campmaxdelay", 13))
        {
            SET_CAMP->MaxDelay((int)fDigit);
            CAMP->MaxDelay(SET_CAMP->Max);
            sprintf_s(szMsg, "\ay%s\aw:: Maxdelay for camp return set to \ag%d", PLUGIN_NAME, CAMP->Max);
        }
        else if (!_strnicmp(szParameter, "pausemindelay", 14))
        {
            PAUSE->MinDelay((int)fDigit);
            sprintf_s(szMsg, "\ay%s\aw:: Mindelay for mpause/mousepause set to \ag%d", PLUGIN_NAME, PAUSE->Min);
        }
        else if (!_strnicmp(szParameter, "pausemaxdelay", 14))
        {
            PAUSE->MaxDelay((int)fDigit);
            sprintf_s(szMsg, "\ay%s\aw:: Maxdelay for mpause/mousepause set to \ag%d", PLUGIN_NAME, PAUSE->Max);
        }
        else if (!_strnicmp(szParameter, "strafemindelay", 15))
        {
            SET_S->MinDelay((int)fDigit);
            STICK->MinDelay(SET_S->Min);
            sprintf_s(szMsg, "\ay%s\aw:: Mindelay for strafe resume set to \ag%d", PLUGIN_NAME, SET_S->Min);
        }
        else if (!_strnicmp(szParameter, "strafemaxdelay", 15))
        {
            SET_S->MaxDelay((int)fDigit);
            STICK->MaxDelay(SET_S->Max);
            sprintf_s(szMsg, "\ay%s\aw:: Maxdelay for strafe resume set to \ag%d", PLUGIN_NAME, SET_S->Max);
        }
        else if (!_strnicmp(szParameter, "ydist", 6))
        {
            SET_M->DistY = fDigit >= 1.0f ? fDigit : SET_M->DistY;
            MOVETO->DistY = SET_M->DistY;
            sprintf_s(szMsg, "\ay%s\aw:: MoveTo Y-Precision set to \ag%.2f", PLUGIN_NAME, SET_M->DistY);
        }
        else if (!_strnicmp(szParameter, "xdist", 6))
        {
            SET_M->DistX = fDigit >= 1.0f ? fDigit : SET_M->DistX;
            MOVETO->DistY = SET_M->DistY;
            sprintf_s(szMsg, "\ay%s\aw:: MoveTo X-Precision set to \ag%.2f", PLUGIN_NAME, SET_M->DistX);
        }
        else if (!_strnicmp(szParameter, "dist", 5) && ucCmdUsed == CMD_MOVETO)
        {
            SET_M->Dist = fDigit >= 1.0f ? fDigit : SET_M->Dist;
            MOVETO->Dist = SET_M->Dist;
            sprintf_s(szMsg, "\ay%s\aw:: MoveTo ArrivalDist set to \ag%.2f", PLUGIN_NAME, SET_M->Dist);
        }
        else if (!_strnicmp(szParameter, "snapdist", 9))
        {
            SET_S->DistSnap = fDigit >= 1.0f ? fDigit : SET_S->DistSnap;
            STICK->DistSnap = SET_S->DistSnap;
            sprintf_s(szMsg, "\ay%s\aw:: Snaproll Distance from target set to \ag%.2f", PLUGIN_NAME, SET_S->DistSnap);
        }
        else if (!_strnicmp(szParameter, "summondist", 11))
        {
            SET->DistSummon = fDigit >= 1.0f ? fDigit : SET->DistSummon;
            sprintf_s(szMsg, "\ay%s\aw:: BreakOnSummon distance set to \ag%.2f", PLUGIN_NAME, SET->DistSummon);
        }
        else if (!_strnicmp(szParameter, "turnrate", 9))
        {
            SET->TurnRate = fDigit >= 1.0f && fDigit <= 100.0f ? fDigit : SET->TurnRate;
            sprintf_s(szMsg, "\ay%s\aw:: Loose Turn Rate set to \ag%.2f", PLUGIN_NAME, SET->TurnRate);
        }
        else if (!_strnicmp(szParameter, "!frontarc", 10))
        {
            SET_S->ArcNotFront = fDigit <= 260.0f && fDigit > 1.0f ? fDigit : SET_S->ArcNotFront;
            STICK->ArcNotFront = SET_S->ArcNotFront;
            sprintf_s(szMsg, "\ay%s\aw:: !front arc set to \ag%.2f", PLUGIN_NAME, SET_S->ArcNotFront);
        }
        else if (!_strnicmp(szParameter, "behindarc", 10))
        {
            SET_S->ArcBehind = fDigit <= 260.0f && fDigit > 1.0f ? fDigit : SET_S->ArcBehind;
            STICK->ArcBehind = SET_S->ArcBehind;
            sprintf_s(szMsg, "\ay%s\aw:: behind arc set to \ag%.2f", PLUGIN_NAME, SET_S->ArcBehind);
        }
        else if (!_strnicmp(szParameter, "breakdist", 10))
        {
            SET_S->DistBreak = fDigit >= 1.0f ? fDigit : SET_S->DistBreak;
            STICK->DistBreak = SET_S->DistBreak;
            sprintf_s(szMsg, "\ay%s\aw:: BreakOnWarp dist set to \ag%.2f", PLUGIN_NAME, SET_S->DistBreak);
        }
        else if (!_strnicmp(szParameter, "campradius", 11))
        {
            SET_CAMP->SetRadius(fDigit);
            CURCAMP->SetRadius(SET_CAMP->Radius);
            sprintf_s(szMsg, "\ay%s\aw:: Camp radius set to \ag%.2f", PLUGIN_NAME, SET_CAMP->Radius);
        }
        else if (!_strnicmp(szParameter, "circleradius", 13))
        {
            SET_C->SetRadius(fDigit);
            CIRCLE->SetRadius(SET_C->Radius);
            sprintf_s(szMsg, "\ay%s\aw:: Circle radius set to \ag%.2f", PLUGIN_NAME, SET_C->Radius);
        }
        else if (!_strnicmp(szParameter, "leashlength", 12))
        {
            SET_CAMP->SetLeash(fDigit);
            CURCAMP->SetLeash(SET_CAMP->Length);
            sprintf_s(szMsg, "\ay%s\aw:: Leash length set to \ag%.2f", PLUGIN_NAME, SET_CAMP->Length);
        }
        else if (!_strnicmp(szParameter, "bearing", 8))
        {
            SET_CAMP->Bearing = fDigit;
            CURCAMP->Bearing = SET_CAMP->Bearing;
            sprintf_s(szMsg, "\ay%s\aw:: Camp return scatter bearing set to \ag%.2f", PLUGIN_NAME, SET_CAMP->Bearing);
        }
        else if (!_strnicmp(szParameter, "scatsize", 9))
        {
            SET_CAMP->ScatSize = fDigit >= 1.0f ? fDigit : SET_CAMP->ScatSize;
            CURCAMP->ScatSize = SET_CAMP->ScatSize;
            sprintf_s(szMsg, "\ay%s\aw:: Camp return scatter size set to \ag%.2f", PLUGIN_NAME, SET_CAMP->ScatSize);
        }
        else if (!_strnicmp(szParameter, "scatdist", 9))
        {
            SET_CAMP->ScatDist = fDigit >= 1.0f ? fDigit : SET_CAMP->ScatDist;
            CURCAMP->ScatDist = SET_CAMP->ScatDist;
            sprintf_s(szMsg, "\ay%s\aw:: Camp return scatter dist set to \ag%.2f", PLUGIN_NAME, SET_CAMP->ScatDist);
        }
        else if (!_strnicmp(szParameter, "backupdist", 11) && (ucCmdUsed == CMD_MOVETO || ucCmdUsed == CMD_STICK))
        {
            if (ucCmdUsed == CMD_MOVETO)
            {
                SET_M->DistBack = fDigit > 1.0f ? fDigit : SET_M->DistBack;
                MOVETO->DistBack = SET_M->DistBack;
            }
            else
            {
                SET_S->DistBack = fDigit > 1.0f ? fDigit : SET_S->DistBack;
                STICK->DistBack = SET_S->DistBack;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Range to use %s backwards positioning set to \ag%.2f", PLUGIN_NAME, szCommand, (ucCmdUsed == CMD_MOVETO) ? SET_M->DistBack : SET_S->DistBack);
        }
        else if (!_strnicmp(szParameter, "allowmove", 10))
        {
            SET->AllowMove = fDigit > 10.0f ? fDigit : SET->AllowMove;
            sprintf_s(szMsg, "\ay%s\aw:: Allow movement when turning at \ag%.2f", PLUGIN_NAME, SET->AllowMove);
        }
        else if (!_strnicmp(szParameter, "flexdist", 9))
        {
            SET_S->DistFlex = (fDigit >= 2.0f && fDigit <= 20.0f) ? fDigit : SET_S->DistFlex;
            STICK->DistFlex = SET_S->DistFlex;
            sprintf_s(szMsg, "\ay%s\aw:: Stick Flexibility distance set to \ag%.2f", PLUGIN_NAME, SET_S->DistFlex);
        }
        else if (!_strnicmp(szParameter, "font", 5))
        {
            int iValid = (int)fDigit > 0 && (int)fDigit < 11 ? (int)fDigit : 0;
            if (iValid == 0)
            {
                sprintf_s(szMsg, "\ay%s\aw:: \arError\ax - Font must be between 1 and 10.", PLUGIN_NAME);
                WriteLine(szMsg, V_ERRORS);
                return;
            }
            WINDOW->NewFont(iValid);
        }
        else if (!_strnicmp(szParameter, "verbflags", 10))
        {
            int iNewLevel = atoi(szSetDigit);
            if (iNewLevel > 0)
            {
                uiVerbLevel = (unsigned int)iNewLevel;
            }
            else
            {
                // set to silence for both 0 and negative numbers
                uiVerbLevel = V_SILENCE;
            }
            sprintf_s(szMsg, "\ay%s\aw:: Verbosity flags set to \ag%d", PLUGIN_NAME, uiVerbLevel);
        }
        else
        {
            sprintf_s(szSetError, "\ay%s\aw:: \arERROR\ax: Invalid '%s set' parameter ( \ar%s\ax )", PLUGIN_NAME, szCommand, szParameter);
            WriteLine(szSetError, V_ERRORS);
            return;
        }
    }

	if (SET->AutoSave) {
		if (ucCmdUsed==1 || ucCmdUsed ==2 || ucCmdUsed==3 || ucCmdUsed == 4)
			SaveConfigSetting(ucCmdUsed);
		else
			SaveConfig();
	}
    WriteLine(szMsg, V_SETTINGS);
}

// ----------------------------------------
// Begin Main function

void MainProcess(unsigned char ucCmdUsed)
{
   PcClient*	 pChData     = pLocalPC;
   PlayerClient* pChSpawn    = pCharSpawn;
   PlayerClient* pLPlayer    = pLocalPlayer;
   PlayerClient* psTarget    = (STICK->Hold ? GetSpawnByID(STICK->HoldID) : pTarget);
   PlayerClient* pCampPlayer = GetSpawnByID(CURCAMP->PcID);

    // ------------------------------------------
    // handle null stick pointers
    if (ucCmdUsed == CMD_STICK)
    {
        // prevent sticking to a target that does not exist or stick id to target that has changed types
        if (STICK->On && (!psTarget || (STICK->Hold && STICK->HoldType != GetSpawnType(psTarget))))
        {
            EndPreviousCmd(true);
            sprintf_s(szMsg, "\ay%s\aw:: You are no longer sticking to anything.", PLUGIN_NAME);
            WriteLine(szMsg, V_STICKV);
            return;
        }
    }
    // end null stick pointers

    // handle /makecamp player if the player no longer exists or has died
    if (CURCAMP->Pc && (!pCampPlayer || CURCAMP->PcType != GetSpawnType(pCampPlayer)))
    {
        sprintf_s(szMsg, "\ay%s\aw:: MakeCamp player ended due to player leaving/death.", PLUGIN_NAME);
        WriteLine(szMsg, V_MAKECAMPV);
        CAMP->ResetPlayer(false);
        return;
    }
    // end /makecamp player handling

    // handle null pointers for all commands
    if (!pChData || !pLPlayer || !pChSpawn->SpawnID || !GetPcProfile())
    {
        sprintf_s(szMsg, "\ay%s\aw:: Null pointer, turning off current command", PLUGIN_NAME);
        WriteLine(szMsg, V_SILENCE);
        EndPreviousCmd(false);
        return;
    }
    // end null pointers
    // ------------------------------------------

    // setup used only in MainProcess
    float fNewHeading     = 0.0f; // heading changes used by all commands
    bool bMoveToOOR       = true;  // used by moveto, set to false if in range
    static bool sbJumping = false; // true if we executed a jump in stucklogic

    // variables reflecting character state
    float fSpeedMulti  = pChSpawn->SpeedMultiplier;
    bool  bSwimming    = pChSpawn->FeetWet                                      ? true : false; // used by stucklogic & uw
    bool  bMounted     = pChSpawn->Mount                                           ? true : false; // used by stucklogic
    bool  bStunned     = pChData->Stunned                                          ? true : false; // used by stucklogic and autopause
    bool  bLevitated   = (pChSpawn->mPlayerPhysicsClient.Levitate == 2)                                 ? true : false; // used by stucklogic
    bool  bRooted      = (fSpeedMulti == -10000.0f  || pChSpawn->RunSpeed < -0.4f) ? true : false; // we return without moving when rooted
    bool  bSnared      = (fSpeedMulti < 0.0f        || pChSpawn->RunSpeed < 0.0f)  ? true : false; // used by stucklogic
    bool  bInJump      = (pChSpawn->Animation == 19 || pChSpawn->Animation == 20)  ? true : false; // used by trytojump

    if (!bInJump)                              sbJumping = false;
    if (bSwimming && SET->AutoUW) STICK->UW = MOVETO->UW = true;
    bool bUseStuck = (!bRooted && !bSnared) ? true : false; // we set false elsewhere if stucklogic is not desired this pulse
    // end MainProcess setup

    // handle breakonsummon
    if (SET->BreakSummon && ucCmdUsed != CMD_MAKECAMP)
    {
        if (SUMMON->CurDist == 0.0f && SUMMON->Y == 0.0f && SUMMON->X == 0.0f)
        {
            // set last loc to current loc if this is first evaluation for current command
            // so that comparison is near-zero and below 'if' will be false
            SUMMON->Y = pChSpawn->Y;
            SUMMON->X = pChSpawn->X;
        }

        // get the distance moved
        SUMMON->CurDist = fabs(GetDistance(SUMMON->Y, SUMMON->X, pChSpawn->Y, pChSpawn->X));
        // store current location for next pulse
        SUMMON->Y = pChSpawn->Y;
        SUMMON->X = pChSpawn->X;

        // if distance moved is larger than user value, halt commands & lock plugin
        if (SUMMON->CurDist > SET->DistSummon)
        {
            sprintf_s(szMsg, "\ay%s\aw:: \arWARNING\ax Command ended from character summoned \ag%.2f\ax distance in a pulse.", PLUGIN_NAME, SUMMON->CurDist);
            EndPreviousCmd(true);
            WriteLine(szMsg, V_BREAKONSUMMON);
            sprintf_s(szMsg, "\ay%s\aw:: \arWARNING\ax Verify you are not being monitored and type \ag/stick imsafe\ax to allow command usage.", PLUGIN_NAME);
            WriteLine(szMsg, V_BREAKONSUMMON);
            pMU->BrokeSummon = PAUSE->PausedMU = true;
            SpewDebug(DBG_MAIN, "Summon Detection fired. All Commands Paused. SET->DistSummon(%.2f) - Moved(%.2f)", SET->DistSummon, SUMMON->CurDist);
            return;
        }
    }
    //end breakonsummon

    // handle autopause
    if (SET->AutoPause && ucCmdUsed != CMD_MAKECAMP)
    {
        static bool sbAPOutput = false;
        // convert to long because spellid is defined as unsigned but data can be negative to represent not casting
        if (((long)(pChSpawn->CastingData.SpellID) >= 0 && !pMU->Bard) || (pChSpawn->StandState != STANDSTATE_STAND && pChSpawn->StandState != STANDSTATE_DUCK) || bStunned || bRooted || (ucCmdUsed == CMD_STICK && ME->IsMe(psTarget)))
        {
            if ((uiVerbLevel & V_AUTOPAUSE) == V_AUTOPAUSE && !sbAPOutput)
            {
                sprintf_s(szMsg, "\ay%s\aw:: AutoPause halting movement...", PLUGIN_NAME);
                WriteLine(szMsg, V_AUTOPAUSE);
                sbAPOutput = true;
                MOVE->StopHeading();
            }
            STICK->TimeStop();
            CAMP->TimeStop(); // reset delay times
            MOVE->StopMove(APPLY_TO_ALL);
            return; // return until above is false
        }
        sbAPOutput = false;
    }
    // end autopause

    // assign needed stick values
    if (ucCmdUsed == CMD_STICK)
    {
        if (!STICK->SetDist)
        {
            STICK->Dist = fStickDistance = (psTarget->StandState ? get_melee_range(pLocalPlayer, psTarget) : 15.0f) * STICK->DistModP + STICK->DistMod;
            STICK->SetDist = true;
        }

        // assign distance
        STICK->DifDist = STICK->CurDist;
        STICK->CurDist = fabs(GetDistance(pChSpawn, psTarget));

        static bool sbSelfOutput = false;
        // if we've changed targets mid-stick or this is the first pulse, dont trigger stucklogic or breakonwarp
        if (STICK->theLastID != psTarget->SpawnID)
        {
            if (STICK->theLastID && STICK->BreakTarget) // if we had set the ID
            {
                EndPreviousCmd(true);
                sprintf_s(szMsg, "\ay%s\aw:: \arStick broken from target change.", PLUGIN_NAME);
                WriteLine(szMsg, V_BREAKONWARP);
                return;
            }
            bUseStuck = false;
            STICK->DifDist = 0.0f;
            sbSelfOutput = false;
        }
        STICK->theLastID = psTarget->SpawnID;

        // make sure havent switched to self - if so, stop movement
        if (ME->IsMe(psTarget))
        {
            if (!sbSelfOutput)
            {
                MOVE->StopMove(APPLY_TO_ALL);
                sprintf_s(szMsg, "\ay%s\aw:: Movement pausing due to self target...", PLUGIN_NAME);
                WriteLine(szMsg, V_AUTOPAUSE);
                sbSelfOutput = true;
            }
            return;
        }
        sbSelfOutput = false;

        // if this is not the first pass and target has not changed
        // handle pauseonwarp or breakonwarp
        if ((STICK->PauseWarp || STICK->BreakWarp) && STICK->DifDist != 0.0f)
        {
            static bool sbBWOutput = false;
            // if the distance between us and the desired stick distance is > user break distance
            // (we compare prevdist so that user can initially stick from distances
            //       larger than breakdist as long as the distance keeps decreasing)
            if (sbBWOutput)
            {
                if (STICK->CurDist - STICK->Dist > STICK->DistBreak) return;
                // return until back in range
            }
            else if (STICK->CurDist - STICK->DifDist - STICK->Dist > STICK->DistBreak)
            {
                if (STICK->PauseWarp)
                {
                    if (!sbBWOutput)
                    {
                        MOVE->StopMove(APPLY_TO_ALL);
                        sprintf_s(szMsg, "\ay%s\aw: Stick pausing until target back in BreakDist range...", PLUGIN_NAME);
                        WriteLine(szMsg, V_BREAKONWARP);
                        sbBWOutput = true;
                        STICK->TimeStop();
                        CAMP->TimeStop(); // reset delay times
                    }
                    // return until above is false
                    return;
                }
                else
                {
                    EndPreviousCmd(true);
                    sprintf_s(szMsg, "\ay%s\aw:: Stick ending from target warping out of BreakDist range.", PLUGIN_NAME);
                    WriteLine(szMsg, V_BREAKONWARP);
                    return;
                }
            }
            sbBWOutput = false;
        }
        // end pauseonwarp/breakonwarp
    }
    // end stick values assignment

    // handle makecamp altreturn
    if (CAMP->DoAlt && !CAMP->Returning)
    {
        CAMP->TimeStop(); // reset delay time
        if (!CURCAMP->Scatter)
        {
            CampReturn(ALTCAMP->Y, ALTCAMP->X, ALTCAMP->Radius, &CAMP->Y, &CAMP->X);
        }
        else
        {
            PolarSpot(ALTCAMP->Y, ALTCAMP->X, 0.0f, CURCAMP->Bearing, CURCAMP->ScatDist, CURCAMP->ScatSize, &CAMP->Y, &CAMP->X);
        }
        MOVETO->On       = CAMP->Returning  = true;
        MOVETO->PreciseX = MOVETO->PreciseY = false;
        CAMP->DoReturn   = CAMP->Auto  = false;
        return;
    }
    // end altreturn

    // makecamp handling
    if (!CAMP->Returning && CURCAMP->On)
    {
        CAMP->CurDist = GetDistance(pChSpawn->Y, pChSpawn->X, CURCAMP->Pc ? pCampPlayer->Y : CURCAMP->Y, CURCAMP->Pc ? pCampPlayer->X : CURCAMP->X);
        CAMP->DifDist = GetDistance(STICK->On ? psTarget->Y : MOVETO->Y, STICK->On ? psTarget->X : MOVETO->X, CURCAMP->Pc ? pCampPlayer->Y : CURCAMP->Y, CURCAMP->Pc ? pCampPlayer->X : CURCAMP->X);

        // break from command if it would exceed active leash
        if (CURCAMP->Leash && (STICK->On || MOVETO->On))
        {
            if (CAMP->CurDist < CAMP->DifDist && CAMP->DifDist > CURCAMP->Length)
            {
                EndPreviousCmd(true);
                sprintf_s(szMsg, "\ay%s\aw:: Outside of leash length, breaking from current command", PLUGIN_NAME);
                WriteLine(szMsg, V_MAKECAMPV);
                return;
            }
        }
        // end leash new command check

        // if makecamp return issued, or if makecamp on check to see if we need to move back
        if (!PAUSE->UserKB && !PAUSE->UserMouse && !STICK->On && !MOVETO->On && !CIRCLE->On)
        {
            // normal return
            if (CAMP->CurDist > CURCAMP->Radius + 2.0f || CAMP->DoReturn) // give leeway to avoid teetering
            {
                bool bDoReturn = true;
                if (!CAMP->DoReturn)
                {
                    if (CURCAMP->NoAggro     && ME->InCombat()) bDoReturn = false;
                    if (!CURCAMP->HaveTarget && pTarget)        bDoReturn = false;
                    if (CURCAMP->NotLoot     && pActiveCorpse)  bDoReturn = false;
                }

                // check this logic - if the timer starts and then doreturn turns false, the conditions might not reset
                // and the return will happen instantly once doreturn is true again
                //
                // instead it SHOULD RESTART THE TIMER from the point the conditions are ready

                // processed conditions in which not to return, if none are met, begin returning
                if (bDoReturn)
                {
                    char cResult = CAMP->TimeStatus();
                    if (CAMP->DoReturn || cResult == T_READY)
                    {
                        CAMP->TimeStop();
                        if (!CURCAMP->Scatter)
                        {
                            CampReturn(CURCAMP->Pc ? pCampPlayer->Y : CURCAMP->Y, CURCAMP->Pc ? pCampPlayer->X : CURCAMP->X, CURCAMP->Radius, &CAMP->Y, &CAMP->X);
                        }
                        else
                        {
                            PolarSpot(CURCAMP->Pc ? pCampPlayer->Y : CURCAMP->Y, CURCAMP->Pc ? pCampPlayer->X : CURCAMP->X, 0.0f, CURCAMP->Bearing, CURCAMP->ScatDist, CURCAMP->ScatSize, &CAMP->Y, &CAMP->X);
                        }
                        MOVETO->On       = CAMP->Returning  = true;
                        MOVETO->PreciseX = MOVETO->PreciseY = false;
                        CAMP->DoAlt                         = false;
                        if (!CAMP->DoReturn) CAMP->Auto     = true; // so the output msg isnt displayed unless user/macro issued command and used in pause
                    }
                    else if (cResult == T_INACTIVE)
                    {
                        CAMP->Auto = false;
                        CAMP->TimeStart();
                        return; // return here to begin waiting for return time
                    }
                }
            }
            // end normal return
        }
        // end auto return checking

        // begin leash processing with active stick/circle
        if (!PAUSE->UserKB && !PAUSE->UserMouse && CURCAMP->Leash && (STICK->On || CIRCLE->On || CURCAMP->RedoStick || CURCAMP->RedoCircle))
        {
            CMULoc HeadBack;
            HeadBack.Y = CURCAMP->Pc ? pCampPlayer->Y : CURCAMP->Y;
            HeadBack.X = CURCAMP->Pc ? pCampPlayer->X : CURCAMP->X;

            if (CAMP->CurDist > CURCAMP->Length + 2.0f) // give leeway if teetering
            {
                if (STICK->On || CIRCLE->On)
                {
                    if (STICK->On)
                    {
                        EndPreviousCmd(true, CMD_STICK, true);
                        STICK->On         = bStickOn        = false; // disable stick but don't reset current cmd settings
                        STICK->BehindOnce = STICK->Snaproll = false; // disable these as well since locations will no longer be accurate
                        CURCAMP->RedoStick                = true;
                    }
                    else
                    {
                        EndPreviousCmd(true, CMD_CIRCLE, true);
                        CIRCLE->On            = false; // disable circling but don't reset current cmd settings
                        CURCAMP->RedoCircle = true;
                    }
                }
                if (psTarget && (CURCAMP->RedoStick || CURCAMP->RedoCircle))
                {
                    fNewHeading = MOVE->SaneHead((atan2(HeadBack.Y - pChSpawn->Y, HeadBack.X - pChSpawn->X) * HEADING_HALF / (float)PI));
                    MOVE->TryMove(GO_FORWARD, MU_WALKOFF, fNewHeading, HeadBack.Y, HeadBack.X);
                }
            }
            else if (CURCAMP->RedoStick || CURCAMP->RedoCircle)
            {
                EndPreviousCmd(false, (CURCAMP->RedoStick ? CMD_STICK : CMD_CIRCLE), true);
                if (CURCAMP->RedoStick)
                    STICK->TurnOn();
                else
                    CIRCLE->On = true;
                CURCAMP->RedoStick = CURCAMP->RedoCircle = false;
                return;
            }
        }
    }
    // end return to camp handling
    if (ucCmdUsed == CMD_MAKECAMP) return; // nothing below applies makecamp, return turns bMoveTo on which calls with CMD_MOVETO

    // reset use of /face command
    gFaceAngle = H_INACTIVE;

    // assign values for circle
    if (ucCmdUsed == CMD_CIRCLE)
    {
        float fUseCirYX[2] = { (pChSpawn->Y - CIRCLE->Y), (pChSpawn->X - CIRCLE->X) };
        CIRCLE->CurDist = sqrt(fUseCirYX[0] * fUseCirYX[0] + fUseCirYX[1] * fUseCirYX[1]);
        if (CIRCLE->CurDist < CIRCLE->Radius * (2.0f / 3.0f)) bUseStuck = false;
    }
    // end circle assignments

    if (ucCmdUsed == CMD_MOVETO)
    {
        if (MOVETO->PreciseY)
        {
            MOVETO->CurDist = fabs(GetDistance(pChSpawn->Y, 0.0f, MOVETO->Y, 0.0f));
            MOVETO->DifDist = MOVETO->DistY;
        }
        else if (MOVETO->PreciseX)
        {
            MOVETO->CurDist = fabs(GetDistance(0.0f, pChSpawn->X, 0.0f, MOVETO->X));
            MOVETO->DifDist = MOVETO->DistX;
        }
        else
        {
            if (CAMP->Returning)
            {
                // if camp player return is active, keep moveto location updated real time
                if (CURCAMP->Pc && CURCAMP->Realtime)
                {
                    if (!CURCAMP->Scatter)
                    {
                        CampReturn(pCampPlayer->Y, pCampPlayer->X, CURCAMP->Radius, &CAMP->Y, &CAMP->X);
                    }
                    else
                    {
                        PolarSpot(pCampPlayer->Y, pCampPlayer->X, 0.0f, CURCAMP->Bearing, CURCAMP->ScatDist, CURCAMP->ScatSize, &CAMP->Y, &CAMP->X);
                    }
                }
                MOVETO->CurDist = fabs(GetDistance(pChSpawn->Y, pChSpawn->X, CAMP->Y, CAMP->X));
            }
            else if (MOVETO->Z == 0.0f)
            {
                MOVETO->CurDist = fabs(GetDistance(pChSpawn->Y, pChSpawn->X, MOVETO->Y, MOVETO->X));
            }
            else
            {
                MOVETO->CurDist = fabs(GetDistance3D(pChSpawn->Y, pChSpawn->X, pChSpawn->Z, MOVETO->Y, MOVETO->X, MOVETO->Z));
            }
            MOVETO->DifDist = MOVETO->Dist;
        }

        // check for stucklogic and MoveToAggro
        if (MOVETO->CurDist <= MOVETO->DifDist)
        {
            bMoveToOOR = bUseStuck = false;
        }
        else
        {
            bMoveToOOR = true;
            if (MOVETO->BreakAggro && MOVETO->DidAggro()) return;
        }
    }

    // BEGIN STUCKLOGIC
    if (STUCK->On)
    {
        // use 3D to compare Z so stucklogic doesn't fire if we are moving more z than y/x (up/down slopes)
        // if bJumping then dont check z axis movement
        STUCK->DifDist = (bLevitated || (sbJumping && bInJump)) ? GetDistance(pChSpawn->Y, pChSpawn->X, STUCK->Y, STUCK->X) : GetDistance3D(pChSpawn->Y, pChSpawn->X, pChSpawn->Z, STUCK->Y, STUCK->X, STUCK->Z);

        // sanity check, if we've moved more than 5 (summon, knockback, user)
        // it will throw off our readjustment, so keep the last value instead
        if (STUCK->DifDist < 5.0f) STUCK->CurDist = MovingAvg(STUCK->DifDist, STUCK->Check);
        //SpewDebug(DBG_STUCK, "STUCK->CurDist = %.2f, fPulseMoved %.2f", STUCK->CurDist, fPulseMoved);

        STUCK->Y = pChSpawn->Y;
        STUCK->X = pChSpawn->X;
        STUCK->Z = pChSpawn->Z;

        //SpewDebug(DBG_DISABLE, "runspeed %.2f and walkspeed %.2f and speedrun %.2f and speedmultiplier %.2f", pChSpawn->RunSpeed, pChSpawn->WalkSpeed, pChSpawn->SpeedRun, pChSpawn->SpeedMultiplier);
        if (bSnared || bRooted)
        {
            // dont use stucklogic if snared
            bUseStuck = false;
            STUCK->StuckInc = 0;
            STUCK->StuckDec = 0;
        }
        else if (fSpeedMulti < 0.25f && *EQADDR_RUNWALKSTATE)
        {
            fSpeedMulti = 0.25f; // boost multiplier of those without runspeed if not walking
        }
        else if (!*EQADDR_RUNWALKSTATE)
        {
            fSpeedMulti = 0.1f; // boost multiplier of those walking
        }

        // if we were stuck last pulse and we moved more than stuckdist since (making progress to get unstuck)
        if (STUCK->StuckInc && STUCK->CurDist > STUCK->Dist)
        {
            STUCK->StuckInc--;
            //SpewDebug(DBG_STUCK, "Decremented STUCK->StuckInc to (%d) because we moved (%3.2f)", STUCK->StuckInc, STUCK->CurDist);

            //force unstuck after defined increments of not being stuck. we reset STUCK->StuckDec if we get stuck again
            STUCK->StuckDec++;
            if (STUCK->StuckDec > STUCK->Unstuck)
            {
                STUCK->StuckInc = 0;
                STUCK->StuckDec = 0;
                //SpewDebug(DBG_STUCK, "Zeroed STUCK->StuckInc and STUCK->StuckDec after consecutive decrements");
            }
        }

        if (bStunned || MOVE->ChangeHead != H_INACTIVE) bUseStuck = false; // dont analyze if stunned or plugin is adjusting heading
        //SpewDebug(DBG_STUCK, "if STUCK->CurDist(%.2f) < STUCK->Dist(%.2f) * fSpeedMulti(%.2f) && bUse(%s) && Fwd(%s) or Side(%s)", STUCK->CurDist, STUCK->Dist, fSpeedMulti, bUseStuck ? "true" : "false", pMU->CmdFwd ? "true" : "false", pMU->CmdStrafe ? "true" : "false");

        if ((pMU->CmdFwd || pMU->CmdStrafe) && bUseStuck &&
            // if moved forward or strafe (not backwards)
            // bUseStuck is set false if using stucklogic this pulse is not desired

            // main stucklogic formula here
            ( (STUCK->CurDist < STUCK->Dist * fSpeedMulti && !bSwimming && !bMounted) ||

            // maybe handle water and mounts on their own system?
            (bSwimming && (double)STUCK->CurDist < 0.0010) ||

            (bMounted && STUCK->CurDist < (STUCK->Dist + fSpeedMulti) / 3.0f) )

            )
        {
            //SpewDebug(DBG_STUCK, "Im Stuck (STUCK->StuckInc = %d) -- if fpulsemoved (%.2f) < STUCK->Dist(%.2f) * fSpeedMultiplier(%.2f) then increment", STUCK->StuckInc, fPulseMoved, STUCK->Dist, fSpeedMulti);

            // 'big if' is true, if we've moved less than user-set dist * current speed multiplier
            if (STUCK->DifDist < STUCK->Dist * fSpeedMulti) // check if our movement this single pulse is still bad
            {
                STUCK->StuckInc++;
                STUCK->StuckDec = 0;
                //SpewDebug(DBG_STUCK, "incremented STUCK->StuckInc %d, reset STUCK->StuckDec to zero", STUCK->StuckInc);

                // STUCK->Jump user set value in INI
                // try to jump early (but not 1-2pulse misfires) and again after a few seconds/turns
                if (STUCK->Jump && !sbJumping && !bLevitated && !bSwimming && ((STUCK->StuckInc % 5) == 0))
                {
                    ExecuteCmd(CMD_JUMP, 1, 0);
                    ExecuteCmd(CMD_JUMP, 0, 0);
                    sbJumping = true;
                }

                // calculate original heading for turnhalf before first turn attempt
                float fOrigHead, fHalfHead, fCompNegHead, fCompPosHead = 0.0f;
                if (STUCK->StuckInc == 4)
                {
                    switch (ucCmdUsed)
                    {
                    case CMD_STICK:
                        fOrigHead = MOVE->SaneHead((atan2(psTarget->X - pChSpawn->X, psTarget->Y - pChSpawn->Y) * HEADING_HALF) / (float)PI);
                        break;
                    case CMD_MOVETO:
                        if (CAMP->Returning)
                        {
                            fOrigHead = MOVE->SaneHead((atan2(CAMP->X - pChSpawn->X, CAMP->Y - pChSpawn->Y) * HEADING_HALF) / (float)PI);
                            break;
                        }
                        fOrigHead = MOVE->SaneHead((atan2(MOVETO->X - pChSpawn->X, MOVETO->Y - pChSpawn->Y) * HEADING_HALF) / (float)PI);
                        break;
                    case CMD_CIRCLE:
                        fOrigHead = (atan2(pChSpawn->Y - CIRCLE->Y, CIRCLE->X - pChSpawn->X) * CIRCLE_HALF) / (float)PI * CIRCLE_QUARTER;
                        fOrigHead += CIRCLE_QUARTER * (CIRCLE->Radius / CIRCLE->CurDist);
                        fOrigHead = MOVE->SaneHead(fOrigHead *= HEADING_MAX / CIRCLE_MAX);
                    }

                    fHalfHead = MOVE->SaneHead(fOrigHead + HEADING_HALF);
                    //SpewDebug(DBG_STUCK, "We've determined that halfway from destination is %.2f", fHalfHead);
                    fCompNegHead = MOVE->SaneHead(fHalfHead - fabs(STUCK->TurnSize / 2.0f));
                    fCompPosHead = MOVE->SaneHead(fHalfHead + fabs(STUCK->TurnSize / 2.0f));
                }

                // if STUCK->StuckInc == multiple of 4 (try to turn 1 increment, every 4 pulses of being stuck)
                if ((STUCK->StuckInc & 3) == 0)
                {
                    fNewHeading = MOVE->SaneHead(pChSpawn->Heading + STUCK->TurnSize);
                    //SpewDebug(DBG_STUCK, "Stucklogic turned, New heading is %.2f", fNewHeading);

                    // if enabled, check if we are halfway away from our destination, reset heading to original heading and start again
                    if (STUCK->TurnHalf)
                    {
                        //SpewDebug(DBG_STUCK, "TURNHALF: Comparing desired heading (%.2f) > %.2f and < %.2f", fNewHeading, fCompNegHead, fCompPosHead);
                        if (fNewHeading > fCompNegHead && fNewHeading < fCompPosHead)
                        {
                            fNewHeading = fOrigHead;
                            STUCK->TurnSize *= -1.0f;
                            //SpewDebug(DBG_STUCK, "TRUE, so flipped STUCK->TurnSize to other way (%.2f) and reset heading to %.2f", STUCK->TurnSize, fNewHeading);
                        }
                    }
                    MOVE->NewHead(fNewHeading);
                    //SpewDebug(DBG_STUCK, "Stucklogic heading change: %.2f", fNewHeading);
                }
            }
            // end fPulseMoved < STUCK->Dist * speedmulti
        }
        // end 'big if'
    }
    // end if STUCK->On (meaning use stucklogic or not)

    // if we are stuck or rooted dont process normal heading & movement
    if (STUCK->StuckInc || bRooted)
    {
        //SpewDebug(DBG_STUCK, "STUCK->StuckInc %d bRooted %s fired, returning without trying to move", STUCK->StuckInc, bRooted ? "true" : "false");
        return;
    }
    // END OF STUCKLOGIC


    // handle heading and movement
    switch (ucCmdUsed)
    {
    case CMD_STICK:
        if (!STICK->Snaproll) fNewHeading = MOVE->SaneHead(atan2(psTarget->X - pChSpawn->X, psTarget->Y - pChSpawn->Y) * HEADING_HALF / (float)PI);
        // jump ahead to stick handling
        break;
    case CMD_CIRCLE:
        fNewHeading = (!CIRCLE->CCW != CIRCLE->Backward) ? (atan2(pChSpawn->Y - CIRCLE->Y, CIRCLE->X - pChSpawn->X) * CIRCLE_HALF) / (float)PI : (atan2(CIRCLE->Y - pChSpawn->Y, pChSpawn->X - CIRCLE->X) * CIRCLE_HALF) / (float)PI;
        CIRCLE->CCW ?  fNewHeading -= CIRCLE_QUARTER + CIRCLE_QUARTER * (CIRCLE->Radius / CIRCLE->CurDist) : fNewHeading += CIRCLE_QUARTER + CIRCLE_QUARTER * (CIRCLE->Radius / CIRCLE->CurDist);
        MOVE->NewHead(MOVE->SaneHead(fNewHeading *= HEADING_MAX / CIRCLE_MAX));
        CIRCLE->Backward ? MOVE->DoMove(GO_BACKWARD) : MOVE->DoMove(GO_FORWARD);
        // end of all circle processing
        return;
    case CMD_MOVETO:
        if (bMoveToOOR)
        {
            if (psTarget && MOVETO->UW)
            {
                double dLookAngle = (double)atan2(psTarget->Z + psTarget->AvatarHeight * StateHeightMultiplier(psTarget->StandState) -
                    pChSpawn->Z - pChSpawn->AvatarHeight * StateHeightMultiplier(pChSpawn->StandState), fabs(GetDistance3D(pChSpawn->Y, pChSpawn->X, pChSpawn->Z, psTarget->Y, psTarget->X, psTarget->Z))) * HEADING_HALF / (double)PI;
                MOVE->NewFace(dLookAngle);
            }
            if (CAMP->Returning)
            {
                fNewHeading = MOVE->SaneHead(atan2(CAMP->X - pChSpawn->X, CAMP->Y - pChSpawn->Y) * HEADING_HALF / (float)PI);
                if (MOVETO->UseBack)
                {
                    if (MOVE->ChangeHead == H_INACTIVE && MOVETO->CurDist < MOVETO->DistBack)
                    {
                        float fHeadDiff = MOVE->SaneHead(pChSpawn->Heading - fNewHeading);
                        if (fHeadDiff >= 200.0f && fHeadDiff <= 300.0f)
                        {
                            MOVE->DoMove(GO_BACKWARD, true, MOVETO->CurDist < 20.0f ? MU_WALKON : MU_WALKOFF);
                            return;
                        }
                    }
                }
                MOVE->NewHead(fNewHeading);
                MOVE->TryMove(GO_FORWARD, (MOVETO->Walk && MOVETO->CurDist < 20.0f) ? MU_WALKON : MU_WALKOFF, fNewHeading, CAMP->Y, CAMP->X);
                return;
            }
            fNewHeading = MOVE->SaneHead(atan2(MOVETO->X - pChSpawn->X, MOVETO->Y - pChSpawn->Y) * HEADING_HALF / (float)PI);
            if (MOVETO->UseBack)
            {
                if (MOVE->ChangeHead == H_INACTIVE && MOVETO->CurDist < MOVETO->DistBack)
                {
                    float fHeadDiff = MOVE->SaneHead(pChSpawn->Heading - fNewHeading);
                    if (fHeadDiff >= 200.0f && fHeadDiff <= 300.0f)
                    {
                        MOVE->DoMove(GO_BACKWARD, true, MOVETO->CurDist < 20.0f ? MU_WALKON : MU_WALKOFF);
                        return;
                    }
                }
            }
            MOVE->NewHead(fNewHeading);
            MOVE->TryMove(GO_FORWARD, (MOVETO->Walk && MOVETO->CurDist < 20.0f) ? MU_WALKON : MU_WALKOFF, fNewHeading, MOVETO->Y, MOVETO->X);
            return;
        }
        if (!CAMP->Auto) // we do not want output for auto-returns
        {
            if (CAMP->DoReturn)
            {
                sprintf_s(szMsg, "\ay%s\aw:: Arrived at %s", PLUGIN_NAME, szArriveCamp);
            }
            else if (CAMP->DoAlt)
            {
                sprintf_s(szMsg, "\ay%s\aw:: Arrived at %s", PLUGIN_NAME, szArriveAlt);
            }
            else
            {
                sprintf_s(szMsg, "\ay%s\aw:: Arrived at %s", PLUGIN_NAME, szArriveMove);
                pMU->StoppedMoveto = true;
            }
            WriteLine(szMsg, V_MOVETOV);
        }
        CAMP->Auto = CAMP->DoAlt = CAMP->DoReturn = false;
        EndPreviousCmd(true);
        // end of all moveto processing
        return;
    default:
        return;
    }

    // -----------------------------------------------------
    // everything below this point relates to '/stick' ONLY
    // -----------------------------------------------------

    // diff between cur heading vs. desired heading
    float fHeadDiff = MOVE->SaneHead(pChSpawn->Heading - fNewHeading);

    // if we are close to the mob and our desired heading change is large, move backwards instead
    if (STICK->UseBack && !STICK->Snaproll && !STICK->Healer && MOVE->ChangeHead == H_INACTIVE && STICK->CurDist < STICK->Dist + STICK->DistBack)
    {
        // we check the MOVE->ChangeHead so not to affect turns already in motion
        if (fHeadDiff >= 200.0f && fHeadDiff <= 300.0f)
        {
            MOVE->DoMove(GO_BACKWARD);
            return;
        }
    }

    // what is clarity?
    bool bTurnHealer = (!STICK->Healer ? true : (STICK->CurDist > STICK->Dist + 10.0f ? true : false));
    if (bTurnHealer)
    {
        MOVE->NewHead(fNewHeading); // adjust heading for all cases except stick healer being oor

        if (STICK->UW) // adjust look angle if underwater param or autouw
        {
            double dLookAngle = (double)atan2(psTarget->Z + psTarget->AvatarHeight * StateHeightMultiplier(psTarget->StandState) -
                pChSpawn->Z - pChSpawn->AvatarHeight * StateHeightMultiplier(pChSpawn->StandState), STICK->CurDist) * HEADING_HALF / (double)PI;
            MOVE->NewFace(dLookAngle);
        }
    }

    // if stucklogic is on and we are ducking while sticking, un-duck
    if (STUCK->On && pChSpawn->StandState == STANDSTATE_DUCK) MOVE->DoStand();

    // move FORWARD ONLY until near stick range (except snaproll)
    if (STICK->CurDist > STICK->Dist + 10.0f && !STICK->Snaproll)
    {
        MOVE->StopMove(KILL_STRAFE);
        MOVE->TryMove(GO_FORWARD, (STICK->CurDist > STICK->Dist + 20.0f) ? MU_WALKOFF : (STICK->Walk ? MU_WALKON : MU_WALKIGNORE), fNewHeading, psTarget->Y, psTarget->X);
        return;
    }
    // everything below: only if near stick range

    // calculate our angular distance
    float fAngDist     = MOVE->AngDist(psTarget->Heading, pChSpawn->Heading);
    float fFabsAngDist = fabs(fAngDist);
    bool b3xRange   = (STICK->CurDist <  STICK->Dist * 3) ? true : false;
    bool bAnyStrafe = (STICK->Strafe  || STICK->Front)    ? true : false;

    // if too far away, don't do pin or behind or aggro checking (if awareness on)
    if (bAnyStrafe && b3xRange)
    {
        // handling for behind/pin/!front
        if (STICK->Strafe)
        {
            // halt strafing if we are on HoTT
            if (pMU->CmdStrafe && (int)pLPlayer->SpawnID == pChSpawn->TargetOfTarget)
            {
                MOVE->StopMove(KILL_STRAFE);
            }
            // we are not on HoTT
            else
            {
                // if randomize is enabled and using /stick behind or /stick !front, but not /stick pin
                if (STICK->Randomize && !STICK->BehindOnce)
                {
                    bool bPosFlag = (rand() % 100 > 50);
                    if (!STICK->Pin)
                    {
                        // our randomized "flag" is negative or positive sign - to determine which arc was modified
                        // we make this eval multiple times, so set it to a bool first
                        if (fAngDist < 0.0f && fFabsAngDist > (bPosFlag ? STICK->RandMax : STICK->RandMin))
                        {
                            MOVE->StickStrafe(GO_LEFT);
                        }
                        else if (fAngDist > 0.0f && fFabsAngDist > (bPosFlag ? STICK->RandMin : STICK->RandMax))
                        {
                            MOVE->StickStrafe(GO_RIGHT);
                        }
                        // else we are within the arc range, so stop strafing
                        else
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                    }
                    else
                    {
                        if (STICK->UseFleeing && IsMobFleeing(pChSpawn, psTarget) && (psTarget->HPCurrent * 100 / psTarget->HPMax) < 25)
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                        else if (fFabsAngDist > PIN_ARC_MAX || fFabsAngDist < PIN_ARC_MIN)
                        {
                            if ((fAngDist < 0.0f && fAngDist > (bPosFlag ? (PIN_ARC_MIN * -1.0f) : (STICK->RandMin * -1.0f))) || fAngDist > (bPosFlag ? STICK->RandMax : PIN_ARC_MAX))
                            {
                                MOVE->StickStrafe(GO_RIGHT);
                            }
                            if ((fAngDist > 0.0f && fAngDist < (bPosFlag ? PIN_ARC_MIN : STICK->RandMin)) || fAngDist < (bPosFlag ? (PIN_ARC_MAX * -1.0f) : (STICK->RandMax * -1.0f)))
                            {
                                MOVE->StickStrafe(GO_LEFT);
                            }
                        }
                        else
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                    }
                }
                // else randomize is not on, or /stick behindonce
                else
                {
                    // normal processing for 'behind' and 'behindonce'
                    if (STICK->Behind || STICK->BehindOnce)
                    {
                        if (fFabsAngDist > STICK->ArcBehind)
                        {
                            if (fAngDist < 0.0f)
                            {
                                MOVE->StickStrafe(GO_LEFT);
                            }
                            else
                            {
                                MOVE->StickStrafe(GO_RIGHT);
                            }
                        }
                        else
                        {
                            STICK->BehindOnce = false;
                            MOVE->StopMove(KILL_STRAFE);
                        }
                    }
                    // processing for '/stick pin'
                    else if (STICK->Pin)
                    {
                        if (STICK->UseFleeing && IsMobFleeing(pChSpawn, psTarget) && (psTarget->HPCurrent * 100 / psTarget->HPMax) < 25)
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                        else if (fFabsAngDist > PIN_ARC_MAX || fFabsAngDist < PIN_ARC_MIN)
                        {
                            if ((fAngDist < 0.0f && fAngDist > (PIN_ARC_MIN * -1.0f)) || fAngDist > PIN_ARC_MAX)
                            {
                                MOVE->StickStrafe(GO_RIGHT);
                            }
                            if ((fAngDist > 0.0f && fAngDist < PIN_ARC_MIN) || fAngDist < (PIN_ARC_MAX * -1.0f))
                            {
                                MOVE->StickStrafe(GO_LEFT);
                            }
                        }
                        else
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                    }
                    // else non-random processing for stick !front
                    else if (STICK->NotFront)
                    {
                        if (fFabsAngDist > STICK->ArcNotFront)
                        {
                            int iRandT = rand() % 100;
                            if (iRandT > 85)
                            {
                                STICK->BehindOnce = true;
                            }
                            else if (fAngDist < 0.0f)
                            {
                                MOVE->StickStrafe(GO_LEFT);
                            }
                            else
                            {
                                MOVE->StickStrafe(GO_RIGHT);
                            }
                        }
                        else
                        {
                            MOVE->StopMove(KILL_STRAFE);
                        }
                    }
                }
            }
        }
        else if (STICK->Front)
        {
            if (SET->Spin || (int)pLPlayer->SpawnID == pChSpawn->TargetOfTarget)
            {
                // if im the target of target or deliberately want to spin if lose aggro
                if (STICK->UseFleeing && IsMobFleeing(pChSpawn, psTarget) && (psTarget->HPCurrent * 100 / psTarget->HPMax) < 25)
                {
                    MOVE->StopMove(KILL_STRAFE);
                }
                else if (fFabsAngDist < FRONT_ARC)
                {
                    if (fAngDist > 0.0f)
                    {
                        MOVE->StickStrafe(GO_LEFT);
                    }
                    else
                    {
                        MOVE->StickStrafe(GO_RIGHT);
                    }
                }
                else
                {
                    MOVE->StopMove(KILL_STRAFE);
                }
            }
            else
            {
                MOVE->StopMove(KILL_STRAFE);
            }
        }
    }
    else
    {
        MOVE->StopMove(KILL_STRAFE);
    }
    //end pin/!front/behind/front

    // handling for stick snaproll
    if (STICK->Snaproll)
    {
        PolarSpot(psTarget->Y, psTarget->X, psTarget->Heading, STICK->Snap->Bearing, STICK->DistSnap, 0.0f, &STICK->Snap->Y, &STICK->Snap->X); // calculate location we want
        STICK->Snap->CurDist = fabs(GetDistance(pChSpawn->Y, pChSpawn->X, STICK->Snap->Y, STICK->Snap->X)); // get distance from that location

        // 3D problems when levitated
        //STICK->Snap->CurDist = fabs(GetDistance3D(pChSpawn->Y, pChSpawn->X, pChSpawn->Z, STICK->Snap->Y, STICK->Snap->X, psTarget->Z)); // get distance from that location

        if (STICK->Snap->CurDist <= 5.0f) // if distance is close enough on first pass, no need to snaproll, this also breaks in future pulses
        {
            MOVE->StopMove(APPLY_TO_ALL);
            STICK->Snaproll      = false;
            STICK->On = bStickOn = true;
            MOVE->NewHead(MOVE->SaneHead(atan2(psTarget->X - pChSpawn->X, psTarget->Y - pChSpawn->Y) * HEADING_HALF / (float)PI));
            STICK->NewSnaproll();
            return;
        }
        // determine heading to that location
        STICK->Snap->Head = MOVE->SaneHead((atan2(STICK->Snap->X - pChSpawn->X, STICK->Snap->Y - pChSpawn->Y) * HEADING_HALF) / (float)PI);
        MOVE->NewHead(STICK->Snap->Head);
        // start movement
        if (STICK->Snap->CurDist > 5.0f)
        {
            MOVE->TryMove(GO_FORWARD, MU_WALKOFF, STICK->Snap->Head, STICK->Snap->Y, STICK->Snap->X);
        }
        return;
    }
    // end snaproll handling

    // ******** main movement handling for stick *******
    // if we are outside stick distance
    if (STICK->CurDist > STICK->Dist)
    {
        bool bWithinRange = (STICK->CurDist < STICK->Dist + 20.0f) ? true : false;
        // flex option, return if we don't want to make small adjustment when close
        if (bWithinRange && STICK->Flex && STICK->CurDist < STICK->CurDist + STICK->DistFlex) return;
        // we are outside of flexible range, or flex is disabled
        MOVE->TryMove(GO_FORWARD, bWithinRange ? (STICK->Walk ? MU_WALKON : MU_WALKIGNORE) : MU_WALKIGNORE, fNewHeading, psTarget->Y, psTarget->X);
        return;
    }
    // else do moveback handling
    if (STICK->MoveBack)
    {
        // if not stick healer, stop movement if we are not within one turn of heading
        if (!STICK->Healer && fabs(pChSpawn->Heading - fNewHeading) > 14.0f)
        {
            MOVE->StopMove(KILL_FB);
            return;
        }
        // moveback not supported unless dist set to more than 5
        float fClose = STICK->Dist > 5.0f ? STICK->Dist - 5.0f : 0.0f;
        // if we are closer than 5.0 from our desired stick distance
        if (STICK->CurDist < fClose)
        {
            MOVE->Walk(GO_BACKWARD);
            return;
        }
        MOVE->StopMove(KILL_FB);
        return;
    }
    // else we are within desired distance, stop forward/backward movement
    MOVE->StopMove(KILL_FB);
}

// End Main function
// ----------------------------------------
// Begin Window Output

void WriteLine(const char* szOutput, VERBLEVEL V_COMPARE)
{
    // never write out if total silence is set
    if (uiVerbLevel == V_SILENCE) return;

    // if we passed totalsilence (0) as type (always output except when totalsilence on, former VERB_ENFORCE)  OR
    // set to display all output OR
    // flag for this msg output is on
    if ((V_COMPARE == V_SILENCE) || (uiVerbLevel & V_EVERYTHING) == V_EVERYTHING || (uiVerbLevel & V_COMPARE) == V_COMPARE)
    {
        if (SET->Window)
        {
            WINDOW->Write(szOutput);
            return;
        }
        WriteChatf("%s",szOutput);
    }
}

void OutputHelp(unsigned char ucCmdUsed, bool bOnlyCmdHelp)
{
    // to support hidehelp, compare uiVerbLevel before calling

    if (!ValidIngame(false)) return;

    char szCommand[20]    = {};
    bool bDisplaySettings = false;

    switch (ucCmdUsed)
    {
    case CMD_MAKECAMP:
        sprintf_s(szCommand, "/makecamp");
        break;
    case CMD_STICK:
        sprintf_s(szCommand, "/stick");
        break;
    case CMD_MOVETO:
        sprintf_s(szCommand, "/moveto");
        break;
    case CMD_CIRCLE:
        sprintf_s(szCommand, "/circle");
        break;
    case HELP_SETTINGS:
        bDisplaySettings = true;
        break;
    }

    char szTempOut[MAX_STRING] = {};
    sprintf_s(szTempOut, "\ay%s \agv%1.4f", PLUGIN_NAME, MQ2Version);
    WriteLine(szTempOut, V_SILENCE);

    if (bDisplaySettings)
    {
        WriteLine("\arThe following settings can be changed with any of the 4 commands. There are three options (using /stick as an example).", V_SILENCE);
        WriteLine("\ay/stick [set] [\agsetting\ax] [on | off]\ax - This will turn the setting on or off", V_SILENCE);
        WriteLine("\ay/stick [set] [\agsetting\ax] [#]\ax - This will change settings that use numerical values", V_SILENCE);
        WriteLine("\ay/stick [toggle] [\agsetting\ax]\ax - This will toggle the setting on and off.", V_SILENCE);
        WriteLine("\arThe following settings can be set on/off OR toggled:", V_SILENCE);
        WriteLine("\ag[autosave]\ax - Automatic saving of configuration file each time you change a setting.", V_SILENCE);
        WriteLine("\ag[feign]\ax - Stay-FD support, awaiting you to manually stand before resuming command.", V_SILENCE);
        WriteLine("\ag[breakonkb|breakonmouse]\ax - End command from manual keyboard/mouse movement.", V_SILENCE);
        WriteLine("\ag[mpause|mousepause]\ax - Pause from manual keyboard/mouse movement. Resumes after pausemindelay/pausemaxdelay values.", V_SILENCE);
        WriteLine("\ag[autopause]\ax - Automatic pause&resume command when casting(non-bard), ducking, stunned, targeting self.", V_SILENCE);
        WriteLine("\ag[verbosity|fullverbosity]\ax - MQ2ChatWnd command output and detailed ouput.", V_SILENCE);
        WriteLine("\ag[hidehelp]\ax - Hides help output from showing automatically on command failures.", V_SILENCE);
        WriteLine("\ag[totalsilence]\ax - Hide all MQ2ChatWnd output except critial warnings and requested information.", V_SILENCE);
        WriteLine("\ag[stucklogic]\ax - Automatic detection of stuck movement and attempted correction.", V_SILENCE);
        WriteLine("\ag[trytojump]\ax - Try to jump as part of stucklogic getting over obstacles.", V_SILENCE);
        WriteLine("\ag[turnhalf]\ax - Reset heading and try the other way if halfway from destination in stucklogic.", V_SILENCE);
        WriteLine("\ag[nohottfront]\ax - Awareness for stick front to not spin if loose aggro.", V_SILENCE);
        WriteLine("\ag[savebychar]\ax - Save character-specific settings to [CharName] section of configuration file.", V_SILENCE);
        WriteLine("\ag[breakonsummon]\ax - Halt plugin if character summoned beyond certain distance in a single pulse.", V_SILENCE);
        WriteLine("\ag[usewalk]\ax - Walk when closing in on moveto / camp return locations for precision.", V_SILENCE);
        WriteLine("\ag[alwaystruehed]\ax - Use legit heading adjustments with right/left keys.", V_SILENCE);
        WriteLine("\arThe following settings use a numerical value:", V_SILENCE);
        WriteLine("\ag[pulsecheck] [#]\ax - Set number of frames used to calculate moving average for stucklogic. (default 4)", V_SILENCE);
        WriteLine("\ag[pulseunstuck] [#]\ax - Set number of frames successfully moved forward to consider unstuck (default 5)", V_SILENCE);
        WriteLine("\ag[diststuck] [#.###]\ax - Set distance needed to move per pulse to not be considered stuck. (default 0.5)", V_SILENCE);
        WriteLine("\ag[campmindelay|campmaxdelay|pausemindelay|pausemaxdelay] [#]\ax - Set camp return or mpause/mousepause delays.", V_SILENCE);
        WriteLine("\ag[turnrate] [#.##]\ax (\ay1.0\ax to \ay100.0\ax) - Set increment used for loose heading turns.", V_SILENCE);
        return; // dont re-spam the output below
    }

    if (!bOnlyCmdHelp)
    {
        WriteLine("\arThe following options work for all commands (\ay/makecamp\ax, \ay/stick\ax, \ay/moveto\ax, \ay/circle\ax)", V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [help]\ax - Displays help output for the command.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [debug]\ax - Outputs debugging information to file.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [load|save]\ax - Load settings from or save settings to the configuration file. (MQ2MoveUtils.ini)", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [pause|unpause]\ax - Pauses/Unpauses all aspects of this plugin.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\arFor detailed information on plugin settings, use \ay%s [help] [settings]\ax for more information.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\arThe remaining options apply to \ay%s\ax only.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
    }

    sprintf_s(szTempOut, "\ay%s [off]\ax - Ends the command.", szCommand);
    WriteLine(szTempOut, V_SILENCE);

    switch (ucCmdUsed)
    {
    case CMD_STICK:
        sprintf_s(szTempOut, "\ay%s\ax - Sticks to current target using default values.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [id #]\ax - Sticks to a target by its SpawnID or your target if the SpawnID is invalid.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [<#>]\ax - Sticks to target using the distance you supply, i.e. /stick 10 starts 10 range away.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [<#>]\ax - Sticks to given %% of max melee range from target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [moveback]\ax - Backs you up to current distance value if you get closer to your target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [uw/underwater]\ax - Look Up/Down to face your target (underwater or not).", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [truehead|loose]\ax - Adjusts your heading legit/close to legit instead of instantly.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [snaproll]\ax - Moves you behind the target in a straight line, then proceeds with a basic stick.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [behind]\ax - Sticks you behind your target. \ar*Will spin in circles if you get aggro and no HoTT", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [behindonce]\ax - Sticks you behind target immediately, then proceeds with a basic stick.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [hold]\ax - Sticks you to current target even if your target changes.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [!front]\ax - Sticks you anywhere but front melee arc of target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [front]\ax - Sticks you to the front melee arc of the target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [pin]\ax - Sticks you to the sides of target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [toggle] [breakongate|breakonwarp]\ax - Toggle command ending on mob gate/warp.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [breakdist] [#]\ax - Set distance for breakonwarp to trigger.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s <your_normal_options> [always]\ax - \arMust be at the end\ax. Will continue to stick to all new NPC targets supplied. Turns off hold/id.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        break;
    case CMD_MOVETO:
        sprintf_s(szTempOut, "\ay%s [loc Y X [Z]|off]\ax - Move to location | stop moving", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s id [SpawnID]\ax - Move to spawnid. If not numeric value then move to current target", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [loose]\ax - Toggles more realistic movement", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [dist <#|-#>]\ax - Furthest distance to be considered 'at location', negative subtracts from value", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [ydist|xdist] [#.##]\ax - Set distance used for precisey/precisex distance checking.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s mdist <#> [id | loc] \ax - Set distance inline with a moveto command.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        break;
    case CMD_MAKECAMP:
        sprintf_s(szTempOut, "\ay%s [on|off]\ax - Drops anchor at current location | removes anchor", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [loc <y> <x>]\ax - Makes camp at supplied anchor location", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [leash]\ax - Toggles leashing to current camp", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [leash [<dist>]]\ax - Set distance to exceed camp before leashing, always turns leash on", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [radius <dist>]\ax - Sets radius/size of camp", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [maxdelay <#>|mindelay <#>]\ax - Sets the max/minimum amount of time before returning to camp.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [return]\ax - Immediately return to camp", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [altreturn]\ax - Immediately return to alternate camp (if established)", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [player [<name>]]\ax - Set dynamic camp around player name in zone, or current target", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [leashlength] [#.##]\ax - Set leash length size.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [returnnoaggro] [on|off]\ax - Return to camp if not aggro (regardless of target).", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [returnnotlooting] [on|off]\ax - Do not return to camp if looting.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [returnhavetarget] [on|off]\ax - Return to camp even if have target.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [campradius] [#]\ax - Change radius of existing camp.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [scatdist] [#.##]\ax - Set scatter distance from center.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [scatsize] [#.##]\ax - Set scatter radius size.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [bearing] [#.##]\ax - Set scatter bearing from center.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [toggle] [usescatter]\ax - Use user-defined scatter values for camp returns.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        break;
    case CMD_CIRCLE:
        sprintf_s(szTempOut, "\ay%s [on <radius>]\ax - Turn circle on at current loc. If radius supplied sets size of circle.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [loc <y> <x>]\ax - Turn on circle at given anchor location. (Y X are the order /loc prints them).", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [radius <#>]\ax - Change the circle radius without resetting circle.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [drunken]\ax - Toggles random turns.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [clockwise|cw]\ax - Toggles circling clockwise.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [counterclockwise|ccw|reverse]\ax - Toggles circling counter-clockwise.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [forwards|backwards]\ax - Set movement direction forwards or backwards.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [toggle] [alwaysccw]\ax - Always use counter-clockwise (default is clockwise).", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [toggle] [alwaysbackwards]\ax - Always use backwards (default is forwards).", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [toggle] [alwaysdrunk]\ax - Always use drunken movement.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        sprintf_s(szTempOut, "\ay%s [set] [circleradius] [#]\ax - Change the circle radius without resetting circle.", szCommand);
        WriteLine(szTempOut, V_SILENCE);
        break;
    }
}

void DebugToWnd(unsigned char ucCmdUsed)
{
    char szTemp[MAX_STRING]   = {};
    char szTempID[MAX_STRING] = "NULL";
    char szYes[8]             = "\agyes\ax";
    char szNo[7]              = "\arno\ax";
    char szDir[25]            = "\agNormal\ax";
    char szLongLine[48]       = "\ay---------------------------------------------";

    sprintf_s(szTemp, "\ay%s v%1.4f - Current Status", PLUGIN_NAME, MQ2Version);
    WriteLine(szTemp, V_SILENCE);
    if (ucCmdUsed == CMD_STICK || ucCmdUsed == APPLY_TO_ALL)
    {
        if (STICK->Behind)
        {
            sprintf_s(szDir, "\agBehind\ax");
        }
        else if (STICK->Pin)
        {
            sprintf_s(szDir, "\agSide\ax");
        }
        else if (STICK->NotFront)
        {
            sprintf_s(szDir, "\agNot Front\ax");
        }
        else if (STICK->Front)
        {
            sprintf_s(szDir, "\agFront\ax");
        }
        WriteLine(szLongLine, V_SILENCE);
        sprintf_s(szTemp, "\ayStick\ax: Status(%s) Dir(%s) Dist(\ag%.2f\ax) Mod(\ag%.2f\ax) Mod%%%%(\ag%.2f%%%%\ax) Head(%s) Water(%s) MoveBack(%s) Hold(%s) Always(%s)", STICK->On ? szOn : szOff, szDir, STICK->Dist, STICK->DistMod, STICK->DistModP, (pMU->Head == H_TRUE) ? "\agTrue\ax" : ((pMU->Head == H_LOOSE) ? "\ayLoose\ax" : "\arFast\ax"), STICK->UW ? szYes : szNo, STICK->MoveBack ? szYes : szNo, STICK->Hold ? szYes : szNo, STICK->Always ? szOn : szOff);
        WriteLine(szTemp, V_SILENCE);
        if (STICK->Hold)
        {
            PlayerClient* pStickThis = GetSpawnByID(STICK->HoldID);
            if (pStickThis)
            {
                sprintf_s(szTempID, "%s", pStickThis->DisplayedName);
            }
            sprintf_s(szTemp, "\ayStick\ax: Holding to ID(\ag%d\ax) Name(\ag%s\ax)", STICK->HoldID, szTempID);
            WriteLine(szTemp, V_SILENCE);
        }
        sprintf_s(szTemp, "\ayStick Options\ax: BreakOnWarp(%s) BreakDist(\ag%.2f\ax) BreakOnGate(%s) ", STICK->BreakWarp ? szOn : szOff, STICK->DistBreak, STICK->BreakGate ? szOn : szOff);
        WriteLine(szTemp, V_SILENCE);
    }
    if (ucCmdUsed == CMD_MOVETO || ucCmdUsed == APPLY_TO_ALL)
    {
        WriteLine(szLongLine, V_SILENCE);
        sprintf_s(szTemp, "\ayMoveto\ax: Status(%s) Y(\ag%.2f\ax) X(\ay%.2f\ax) Dist(\ag%.2f\ax) Mod(\ag%.2f\ax) YPrecise(%s) XPrecise(%s)", MOVETO->On ? szOn : szOff, MOVETO->Y, MOVETO->X, MOVETO->Dist, MOVETO->Mod, MOVETO->PreciseY ? szYes : szNo, MOVETO->PreciseX ? szYes : szNo);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayMoveto Options\ax: Y-Dist(\ag%.2f\ax) X-Dist(\ag%.2f\ax)", MOVETO->DistY, MOVETO->DistX);
        WriteLine(szTemp, V_SILENCE);
    }
    if (ucCmdUsed == CMD_CIRCLE || ucCmdUsed == APPLY_TO_ALL)
    {
        WriteLine(szLongLine, V_SILENCE);
        sprintf_s(szTemp, "\ayCircle\ax: Status(%s) Y(\ag%.2f\ax) X(\ag%.2f\ax) Radius(\ag%.2f\ax) Drunken(%s) Backwards(%s) Clockwise(%s)", CIRCLE->On ? szOn : szOff, CIRCLE->Y, CIRCLE->X, CIRCLE->Radius, CIRCLE->Drunk ? szYes : szNo, CIRCLE->Backward ? szYes : szNo, CIRCLE->CCW ? szNo : szYes);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayCircle Options\ax: AlwaysDrunk(%s) AlwaysBackwards(%s) AlwaysCCW(%s)", SET_C->Drunk ? szYes : szNo, SET_C->Backward ? szYes : szNo, SET_C->CCW ? szYes : szNo);
        WriteLine(szTemp, V_SILENCE);
    }
    if (ucCmdUsed == CMD_MAKECAMP || ucCmdUsed == APPLY_TO_ALL)
    {
        WriteLine(szLongLine, V_SILENCE);
        sprintf_s(szTemp, "\ayMakeCamp\ax: Status(%s) Player(%s) Y(\ag%.2f\ax) X(\ag%.2f\ax) Radius(\ag%.2f\ax) Leash(%s) LeashLen(\ag%.2f\ax) Returning(%s) ", CURCAMP->On ? szOn : szOff, CURCAMP->Pc ? szYes : szNo, CURCAMP->Y, CURCAMP->X, CURCAMP->Radius, CURCAMP->Leash ? szOn : szOff, CURCAMP->Length, CAMP->Auto ? szYes : szNo);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayMakeCamp\ax: Scatter(%s) Bearing(\ag%.2f\ax) ScatDist(\ag%.2f\ax) ScatSize(\ag%.2f\ax)", CURCAMP->Scatter ? szOn : szOff, CURCAMP->Bearing, CURCAMP->ScatDist, CURCAMP->ScatSize);
        WriteLine(szTemp, V_SILENCE);
        if (CURCAMP->Pc)
        {
            PlayerClient* pCampThis = GetSpawnByID(CURCAMP->PcID);
            if (pCampThis)
            {
                sprintf_s(szTempID, "%s", pCampThis->DisplayedName);
            }
            sprintf_s(szTemp, "\ayMakeCamp\ax: Player Name(\ag%s\ax) ID(\ag%d\ax)", szTempID, CURCAMP->PcID);
            WriteLine(szTemp, V_SILENCE);
        }
        if (ALTCAMP->On)
        {
            sprintf_s(szTemp, "\ayMakeCamp\ax: AlternateCamp(\agon\ax) AltAnchorY(\ag%.2f\ax) AltAnchorX(\ag%.2f\ax) AltRadius(\ag%.2f\ax)", ALTCAMP->Y, ALTCAMP->X, ALTCAMP->Radius);
            WriteLine(szTemp, V_SILENCE);
        }
        sprintf_s(szTemp, "\ayMakeCamp Options\ax: ReturnNoAggro(%s) MinDelay(\ag%d\ax) MaxDelay(\ag%d\ax)", CURCAMP->NoAggro ? szOn : szOff, CAMP->Min, CAMP->Max);
        WriteLine(szTemp, V_SILENCE);
    }

    if (ucCmdUsed == APPLY_TO_ALL)
    {
        WriteLine(szLongLine, V_SILENCE);
        sprintf_s(szTemp, "\ayPlugin Status\ax: Paused(%s) MinPause(\ag%d\ax) MaxPause(\ag%d\ax)", PAUSE->PausedMU ? szYes : szNo, PAUSE->Min, PAUSE->Max);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayStuckLogic\ax: Enabled(%s) DistStuck(\ag%.3f\ax) PulseCheck(\ag%d\ax) PulseUnstuck(\ag%d\ax)", STUCK->On ? szYes : szNo, STUCK->Dist, STUCK->Check, STUCK->Unstuck);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayPause Options\ax: AutoPause(%s) MPause(%s) BreakOnKB(%s) MousePause(%s) BreakOnMouse(%s)", SET->AutoPause ? szOn : szOff, SET->PauseKB ? szOn : szOff, SET->PauseMouse ? szOn : szOff, SET->BreakKB ? szOn : szOff, SET->BreakMouse ? szOn : szOff);
        WriteLine(szTemp, V_SILENCE);
        sprintf_s(szTemp, "\ayPlugin Options\ax: AutoSave(%s) FeignSupport(%s) BreakOnSummon(%s) BreakSummonDist(\ag%.2f\ax) AwareNotAggro(%s)", SET->AutoSave ? szOn : szOff, SET->Feign ? szOn : szOff, SET->BreakSummon ? szOn : szOff, SET->DistSummon, SET->Spin ? szOn : szOff);
        WriteLine(szTemp, V_SILENCE);
    }
}

void SpewMUError(unsigned char ucErrorNum)
{
    // this function exists to avoid the duplicate code for error messages that are used multiple places
    // any one-time error will still be in its own logic for clarity
    char szErrorMsg[MAX_STRING] = {};

    switch(ucErrorNum)
    {
    case ERR_STICKSELF:
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) You cannot stick to yourself!", PLUGIN_NAME);
        break;
    case ERR_STICKNONE:
        sprintf_s(szErrorMsg, "\ay%s\aw:: You must specify something to stick to!", PLUGIN_NAME);
        break;
    case ERR_BADMOVETO:
        EndPreviousCmd(true);
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) \ay/moveto loc [ <Y> <X> [z] ]\ax was supplied incorrectly.", PLUGIN_NAME);
        break;
    case ERR_BADMAKECAMP:
        CAMP->NewCamp(false);
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) \ay/makecamp loc [ <Y> <X> ]\ax was supplied incorrectly.", PLUGIN_NAME);
        break;
    case ERR_BADCIRCLE:
        EndPreviousCmd(true);
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) Usage \ay/circle loc [ <y> <x> ] [other options]\ax", PLUGIN_NAME);
        break;
    case ERR_BADSPAWN:
        EndPreviousCmd(true);
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) Invalid SpawnID and do not have a valid target.", PLUGIN_NAME);
        break;
    case ERR_BADDELAY:
        sprintf_s(szErrorMsg, "\ay%s\aw:: (\arERROR\ax) \ay[mindelay|maxdelay] [#]\ax was supplied incorrectly.", PLUGIN_NAME);
        break;
    default:
        // return if we didnt pass a valid msg number
        return;
    }
    WriteLine(szErrorMsg, V_ERRORS);
}

// End window output
// ----------------------------------------
// Debugger

void DebugToDebugger(const char* szFormat, ...)
{
    char szDbgOutput[MAX_STRING] = {};
    va_list vaList;
    va_start(vaList, szFormat);
    vsprintf_s(szDbgOutput, szFormat, vaList);
    OutputDebugString("[MUTILS] ");
    OutputDebugString(szDbgOutput);
    OutputDebugString("\n");
}

void SpewDebug(unsigned char ucDbgType, const char* szOutput, ...)
{
    int i = 1;
    switch (ucDbgType)
    {
    case DBG_MAIN:
#ifdef DEBUGMAIN
        DebugToDebugger("%s", szOutput);
#else
        i++;
#endif
        break;
    case DBG_STUCK:
#ifdef DEBUGSTUCK
        DebugToDebugger("%s", szOutput);
#else
        i++;
#endif
        break;
    case DBG_MISC:
#ifdef DEBUGMISC
        DebugToDebugger("%s", szOutput);
#else
        i++;
#endif
        break;
    case DBG_DISABLE:
        // do nothing with output
        i++;
        break;
    default:
        i++;
        break;
    }
}

// ----------------------------------------
// Begin file input/output

void DebugToINI(unsigned char ucCmdUsed)
{
    if (!ValidIngame(false)) return;

    char szTemp[MAX_STRING] = {};
    char szCommand[15]      = {};

    switch (ucCmdUsed)
    {
    case CMD_MAKECAMP:
        sprintf_s(szCommand, "/makecamp");
        break;
    case CMD_STICK:
        sprintf_s(szCommand, "/stick");
        break;
    case CMD_MOVETO:
        sprintf_s(szCommand, "/moveto");
        break;
    case CMD_CIRCLE:
        sprintf_s(szCommand, "/circle");
        break;
    }

    sprintf_s(szTemp, "%s v%1.4f", PLUGIN_NAME, MQ2Version);
    WritePrivateProfileString("Version",      "Number",                szTemp,                 szDebugName);
    WritePrivateProfileString("Commands",     "CommandUsed",           szCommand,              szDebugName);
    WritePrivateProfileValue("GenericBOOL",   "pMU->Keybinds",         pMU->Keybinds,          szDebugName);
    WritePrivateProfileValue("GenericBOOL",   "PAUSE->PausedMU",       PAUSE->PausedMU,        szDebugName);
    WritePrivateProfileValue("GenericBOOL",   "pMU->BrokeSummon",      pMU->BrokeSummon,       szDebugName);
    WritePrivateProfileString("Strings",      "szMsg",                 szMsg,                  szDebugName);
    WritePrivateProfileValue("AggroChecking", "SET->Spin",             SET->Spin,              szDebugName);
    WritePrivateProfileValue("AggroChecking", "pMU->Aggro",            pMU->Aggro,             szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->AutoPause",        SET->AutoPause,         szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->PauseKB",          SET->PauseKB,           szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->PauseMouse",       SET->PauseMouse,        szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->BreakKB",          SET->BreakKB,           szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->BreakMouse",       SET->BreakMouse,        szDebugName);
    WritePrivateProfileValue("INISettings",   "STUCK->On",             STUCK->On,              szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->AutoSave",         SET->AutoSave,          szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->Feign",            SET->Feign,             szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->SaveByChar",       SET->SaveByChar,        szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->BreakSummon",      SET->BreakSummon,       szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->DistSummon",       SET->DistSummon,        szDebugName);
    WritePrivateProfileValue("INISettings",   "SET->TurnRate",         SET->TurnRate,          szDebugName);
    WritePrivateProfileValue("INISettings",   "PAUSE->Min",            PAUSE->Min,             szDebugName);
    WritePrivateProfileValue("INISettings",   "PAUSE->Max",            PAUSE->Max,             szDebugName);
    WritePrivateProfileValue("INISettings",   "CAMP->Min",             CAMP->Min,              szDebugName);
    WritePrivateProfileValue("INISettings",   "CAMP->Max",             CAMP->Max,              szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "MOVE->ChangeHead",      MOVE->ChangeHead,       szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "PAUSE->UserKB",         PAUSE->UserKB,          szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "CAMP->Returning",       CAMP->Returning,        szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "CAMP->Auto",            CAMP->Auto,             szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "STICK->HaveTarget",     STICK->HaveTarget,      szDebugName);
    WritePrivateProfileValue("SetOnPulse",    "STICK->CurDist",        STICK->CurDist,         szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->BreakGate",      STICK->BreakGate,       szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->BreakWarp",      STICK->BreakWarp,       szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->DistBreak",      STICK->DistBreak,       szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->On",             STICK->On,              szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->SetDist",        STICK->SetDist,         szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Dist",           STICK->Dist,            szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->DistMod",        STICK->DistMod,         szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->DistModP",       STICK->DistModP,        szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->MoveBack",       STICK->MoveBack,        szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Behind",         STICK->Behind,          szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->BehindOnce",     STICK->BehindOnce,      szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Front",          STICK->Front,           szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->NotFront",       STICK->NotFront,        szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Pin",            STICK->Pin,             szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Snaproll",       STICK->Snaproll,        szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Hold",           STICK->Hold,            szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->HoldID",         (int)STICK->HoldID,     szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->UW",             STICK->UW,              szDebugName);
    WritePrivateProfileValue("Stick",         "STICK->Always",         STICK->Always,          szDebugName);
    WritePrivateProfileValue("StickSnaproll", "STICK->Snap->Y",        STICK->Snap->Y,         szDebugName);
    WritePrivateProfileValue("StickSnaproll", "STICK->Snap->X",        STICK->Snap->X,         szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->On",           CURCAMP->On,            szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Y",            CURCAMP->Y,             szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->X",            CURCAMP->X,             szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Radius",       CURCAMP->Radius,        szDebugName);
    WritePrivateProfileValue("MakeCamp",      "ALTCAMP->On",           ALTCAMP->On,            szDebugName);
    WritePrivateProfileValue("MakeCamp",      "ALTCAMP->Y",            ALTCAMP->Y,             szDebugName);
    WritePrivateProfileValue("MakeCamp",      "ALTCAMP->X",            ALTCAMP->X,             szDebugName);
    WritePrivateProfileValue("MakeCamp",      "ALTCAMP->Radius",       ALTCAMP->Radius,        szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Pc",           CURCAMP->Pc,            szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->PcID",         (int)CURCAMP->PcID,     szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CAMP->DoReturn",        CAMP->DoReturn,         szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CAMP->DoAlt",           CAMP->DoAlt,            szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Leash",        CURCAMP->Leash,         szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Length",       CURCAMP->Length,        szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Scatter",      CURCAMP->Scatter,       szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->Bearing",      CURCAMP->Bearing,       szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->ScatSize",     CURCAMP->ScatSize,      szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->ScatDist",     CURCAMP->ScatDist,      szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->NoAggro",      CURCAMP->NoAggro,       szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->HaveTarget",   CURCAMP->HaveTarget,    szDebugName);
    WritePrivateProfileValue("MakeCamp",      "CURCAMP->NotLoot",      CURCAMP->NotLoot,       szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->On",            MOVETO->On,             szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->Y",             MOVETO->Y,              szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->X",             MOVETO->X,              szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->Dist",          MOVETO->Dist,           szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->PreciseY",      MOVETO->PreciseY,       szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->DistY",         MOVETO->DistY,          szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->PreciseX",      MOVETO->PreciseX,       szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->DistX",         MOVETO->DistX,          szDebugName);
    WritePrivateProfileValue("MoveTo",        "MOVETO->Walk",          MOVETO->Walk,           szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->On",            CIRCLE->On,             szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->Y",             CIRCLE->Y,              szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->X",             CIRCLE->X,              szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->Radius",        CIRCLE->Radius,         szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->Backward",      CIRCLE->Backward,       szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->CCW",           CIRCLE->CCW,            szDebugName);
    WritePrivateProfileValue("Circle",        "CIRCLE->Drunk",         CIRCLE->Drunk,          szDebugName);
    WritePrivateProfileValue("StuckLogic",    "pMU->CmdFwd",           pMU->CmdFwd,            szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Jump",           STUCK->Jump,            szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->TurnHalf",       STUCK->TurnHalf,        szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Check",          (int)STUCK->Check,      szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Unstuck",        (int)STUCK->Unstuck,    szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->StuckInc",       (int)STUCK->StuckInc,   szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->StuckDec",       (int)STUCK->StuckDec,   szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Dist",           STUCK->Dist,            szDebugName);
    WritePrivateProfileValue("StuckLogic",    "PulseAvg",              STUCK->CurDist,         szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->TurnSize",       STUCK->TurnSize,        szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Y",              STUCK->Y,               szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->X",              STUCK->X,               szDebugName);
    WritePrivateProfileValue("StuckLogic",    "STUCK->Z",              STUCK->Z,               szDebugName);
}

void SaveConfig()
{
    char szTemp[MAX_STRING]     = {};

    // default settings
    WritePrivateProfileValue("Defaults",   "AllowMove",              SET->AllowMove,           INIFileName);
    WritePrivateProfileValue("Defaults",   "AutoPause",              SET->AutoPause,           INIFileName);
    WritePrivateProfileValue("Defaults",   "AutoPauseMsg",           (uiVerbLevel & V_AUTOPAUSE) == V_AUTOPAUSE, INIFileName);
    WritePrivateProfileValue("Defaults",   "AutoSave",               SET->AutoSave,            INIFileName);
    WritePrivateProfileValue("Defaults",   "AutoUW",                 SET->AutoUW,              INIFileName);
    WritePrivateProfileValue("Defaults",   "BreakKeyboard",          SET->BreakKB,             INIFileName);
    WritePrivateProfileValue("Defaults",   "BreakMouse",             SET->BreakMouse,          INIFileName);
    WritePrivateProfileValue("Defaults",   "BreakOnGM",              SET->BreakGM,             INIFileName);
    WritePrivateProfileValue("Defaults",   "BreakOnSummon",          SET->BreakSummon,         INIFileName);
    WritePrivateProfileValue("Defaults",   "DistSummon",             SET->DistSummon,          INIFileName);
    WritePrivateProfileValue("Defaults",   "FeignSupport",           SET->Feign,               INIFileName);
    WritePrivateProfileValue("Defaults",   "Heading",                (SET->Head == H_TRUE) ? "true" : (SET->Head == H_LOOSE ? "loose" : "fast"), INIFileName);
    WritePrivateProfileValue("Defaults",   "HideHelp",               (uiVerbLevel & V_HIDEHELP) == V_HIDEHELP, INIFileName);
    WritePrivateProfileValue("Defaults",   "KeyboardPause",          SET->PauseKB,             INIFileName);
    WritePrivateProfileValue("Defaults",   "MousePause",             SET->PauseMouse,          INIFileName);
    WritePrivateProfileValue("Defaults",   "LockPause",              SET->LockPause,           INIFileName);
    WritePrivateProfileValue("Defaults",   "PauseMinDelay",          PAUSE->Min,               INIFileName);
    WritePrivateProfileValue("Defaults",   "PauseMaxDelay",          PAUSE->Max,               INIFileName);
    WritePrivateProfileValue("Defaults",   "SaveByChar",             SET->SaveByChar,          INIFileName);
    WritePrivateProfileValue("Defaults",   "TurnRate",               SET->TurnRate,            INIFileName);
    WritePrivateProfileValue("Defaults",   "UseWindow",              SET->Window,              INIFileName);
    WritePrivateProfileValue("Defaults",   "Verbosity",              (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY, INIFileName);
    // FullVerbosity is more frequent, detailed output, and differs from Verbosity
    // Setting one does not include the text of the other.
    WritePrivateProfileValue("Defaults",   "FullVerbosity",          (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY, INIFileName);
    // Total Silence writes no output except critical or user-requested
    WritePrivateProfileValue("Defaults",   "TotalSilence",           uiVerbLevel == 0,         INIFileName);
    WritePrivateProfileValue("Defaults",   "VerbosityFlags",         uiVerbLevel,              INIFileName);
    WritePrivateProfileValue("Defaults",   "WinEQ",                  SET->WinEQ,               INIFileName);
    // stick settings
    WritePrivateProfileValue("Stick",      "AlwaysUW",               SET_S->UW,                INIFileName);
    WritePrivateProfileValue("Stick",      "AwareNotAggro",          SET->Spin,                INIFileName);
    WritePrivateProfileValue("Stick",      "ArcBehind",              SET_S->ArcBehind,         INIFileName);
    WritePrivateProfileValue("Stick",      "ArcNotFront",            SET_S->ArcNotFront,       INIFileName);
    WritePrivateProfileValue("Stick",      "BreakOnGate",            SET_S->BreakGate,         INIFileName);
    WritePrivateProfileValue("Stick",      "BreakOnHit",             SET_S->BreakHit,          INIFileName);
    WritePrivateProfileValue("Stick",      "BreakOnTarget",          SET_S->BreakTarget,       INIFileName);
    WritePrivateProfileValue("Stick",      "BreakOnWarp",            SET_S->BreakWarp,         INIFileName);
    WritePrivateProfileValue("Stick",      "PauseOnWarp",            SET_S->PauseWarp,         INIFileName);
    WritePrivateProfileValue("Stick",      "DelayStrafe",            SET_S->DelayStrafe,       INIFileName);
    WritePrivateProfileValue("Stick",      "DistBackup",             SET_S->DistBack,          INIFileName);
    WritePrivateProfileValue("Stick",      "DistBreak",              SET_S->DistBreak,         INIFileName);
    WritePrivateProfileValue("Stick",      "DistFlex",               SET_S->DistFlex,          INIFileName);
    WritePrivateProfileValue("Stick",      "DistMod",                SET_S->DistMod,           INIFileName);
    WritePrivateProfileValue("Stick",      "DistMod%",               SET_S->DistModP,          INIFileName);
    WritePrivateProfileValue("Stick",      "DistSnaproll",           SET_S->DistSnap,          INIFileName);
    WritePrivateProfileValue("Stick",      "RandomArc",              SET_S->Randomize,         INIFileName);
    WritePrivateProfileValue("Stick",      "StrafeMinDelay",         SET_S->Min,               INIFileName);
    WritePrivateProfileValue("Stick",      "StrafeMaxDelay",         SET_S->Max,               INIFileName);
    WritePrivateProfileValue("Stick",      "UseBackward",            SET_S->UseBack,           INIFileName);
    WritePrivateProfileValue("Stick",      "UseFleeing",             SET_S->UseFleeing,        INIFileName);
    WritePrivateProfileValue("Stick",      "UseFlex",                SET_S->Flex,              INIFileName);
    WritePrivateProfileValue("Stick",      "UseWalk",                SET_S->Walk,              INIFileName);
    // makecamp settings
    WritePrivateProfileValue("MakeCamp",   "CampRadius",             SET_CAMP->Radius,         INIFileName);
    WritePrivateProfileValue("MakeCamp",   "MinDelay",               SET_CAMP->Min,            INIFileName);
    WritePrivateProfileValue("MakeCamp",   "MaxDelay",               SET_CAMP->Max,            INIFileName);
    WritePrivateProfileValue("MakeCamp",   "RealtimePlayer",         SET_CAMP->Realtime,       INIFileName);
    WritePrivateProfileValue("MakeCamp",   "ReturnHaveTarget",       SET_CAMP->HaveTarget,     INIFileName);
    WritePrivateProfileValue("MakeCamp",   "ReturnNoAggro",          SET_CAMP->NoAggro,        INIFileName);
    WritePrivateProfileValue("MakeCamp",   "ReturnNotLooting",       SET_CAMP->NotLoot,        INIFileName);
    WritePrivateProfileValue("MakeCamp",   "UseLeash",               SET_CAMP->Leash,          INIFileName);
    WritePrivateProfileValue("MakeCamp",   "LeashLength",            SET_CAMP->Length,         INIFileName);
    // camp scattering
    WritePrivateProfileValue("MakeCamp",   "UseScatter",             SET_CAMP->Scatter,        INIFileName);
    WritePrivateProfileValue("MakeCamp",   "Bearing",                SET_CAMP->Bearing,        INIFileName);
    WritePrivateProfileValue("MakeCamp",   "ScatDist",               SET_CAMP->ScatDist,       INIFileName);
    WritePrivateProfileValue("MakeCamp",   "ScatSize",               SET_CAMP->ScatSize,       INIFileName);
    // moveto
    WritePrivateProfileValue("MoveTo",     "AlwaysUW",               SET_M->UW,                INIFileName);
    WritePrivateProfileValue("MoveTo",     "ArrivalDist",            SET_M->Dist,              INIFileName);
    WritePrivateProfileValue("MoveTo",     "ArrivalDistX",           SET_M->DistX,             INIFileName);
    WritePrivateProfileValue("MoveTo",     "ArrivalDistY",           SET_M->DistY,             INIFileName);
    WritePrivateProfileValue("MoveTo",     "BreakOnAggro",           SET_M->BreakAggro,        INIFileName);
    WritePrivateProfileValue("MoveTo",     "BreakOnHit",             SET_M->BreakHit,          INIFileName);
    WritePrivateProfileValue("MoveTo",     "DistBackup",             SET_M->DistBack,          INIFileName);
    WritePrivateProfileValue("MoveTo",     "MoveToMod",              SET_M->Mod,               INIFileName);
    WritePrivateProfileValue("MoveTo",     "UseBackward",            SET_M->UseBack,           INIFileName);
    WritePrivateProfileValue("MoveTo",     "UseWalk",                SET_M->Walk,              INIFileName);
    // circle
    WritePrivateProfileValue("Circle",     "Backward",               SET_C->Backward,          INIFileName);
    WritePrivateProfileValue("Circle",     "CCW",                    SET_C->CCW,               INIFileName);
    WritePrivateProfileValue("Circle",     "Drunken",                SET_C->Drunk,             INIFileName);
    WritePrivateProfileValue("Circle",     "RadiusSize",             SET_C->Radius,            INIFileName);
    // stucklogic
    WritePrivateProfileValue("StuckLogic", "StuckLogic",             STUCK->On,                INIFileName);
    WritePrivateProfileValue("StuckLogic", "DistStuck",              STUCK->Dist,              INIFileName);
    WritePrivateProfileValue("StuckLogic", "PulseCheck",             (int)STUCK->Check,        INIFileName);
    WritePrivateProfileValue("StuckLogic", "PulseUnstuck",           (int)STUCK->Unstuck,      INIFileName);
    WritePrivateProfileValue("StuckLogic", "TryToJump",              STUCK->Jump,              INIFileName);
    WritePrivateProfileValue("StuckLogic", "TurnHalf",               STUCK->TurnHalf,          INIFileName);

    // check if we want to explicitly ignore this chars custom section
    bool ignoreMe = GetPrivateProfileBool(szCharName, "DisregardMe", false, INIFileName);
    if (SET->SaveByChar && !ignoreMe)
    {
        // Character specific
        WritePrivateProfileValue(szCharName, "AllowMove",      SET->AllowMove,                 INIFileName);
        WritePrivateProfileValue(szCharName, "ArcBehind",      SET_S->ArcBehind,               INIFileName);
        WritePrivateProfileValue(szCharName, "ArcNotFront",    SET_S->ArcNotFront,             INIFileName);
        WritePrivateProfileValue(szCharName, "AutoSave",       SET->AutoSave,                  INIFileName);
        WritePrivateProfileValue(szCharName, "AutoUW",         SET->AutoUW,                    INIFileName);
        WritePrivateProfileValue(szCharName, "DistBreak",      SET_S->DistBreak,               INIFileName);
        WritePrivateProfileValue(szCharName, "BreakOnGate",    SET_S->BreakGate,               INIFileName);
        WritePrivateProfileValue(szCharName, "BreakOnWarp",    SET_S->BreakWarp,               INIFileName);
        WritePrivateProfileValue(szCharName, "PauseOnWarp",    SET_S->PauseWarp,               INIFileName);
        WritePrivateProfileValue(szCharName, "LockPause",      SET->LockPause,                 INIFileName);
        WritePrivateProfileValue(szCharName, "DistSnaproll",   SET_S->DistSnap,                INIFileName);
        WritePrivateProfileValue(szCharName, "FeignSupport",   SET->Feign,                     INIFileName);
        WritePrivateProfileValue(szCharName, "Heading",        SET->Head == H_TRUE ? "true" : SET->Head == H_LOOSE  ? "loose" : "fast", INIFileName);
        WritePrivateProfileValue(szCharName, "LeashLength",    SET_CAMP->Length,               INIFileName);
        WritePrivateProfileValue(szCharName, "UseLeash",       SET_CAMP->Leash,                INIFileName);
        WritePrivateProfileValue(szCharName, "UseWindow",      SET->Window,                    INIFileName);
        WritePrivateProfileValue(szCharName, "Verbosity",      (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY , INIFileName);
        WritePrivateProfileValue(szCharName, "FullVerbosity",  (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY, INIFileName);
        WritePrivateProfileValue(szCharName, "VerbosityFlags", uiVerbLevel,                    INIFileName);
        WritePrivateProfileValue(szCharName, "CampRadius",     SET_CAMP->Radius,               INIFileName);
        WritePrivateProfileValue(szCharName, "RealtimePlayer", SET_CAMP->Realtime,             INIFileName);
        // scatter values
        WritePrivateProfileValue(szCharName, "UseScatter",     SET_CAMP->Scatter,              INIFileName);
        WritePrivateProfileValue(szCharName, "Bearing",        SET_CAMP->Bearing,              INIFileName);
        WritePrivateProfileValue(szCharName, "ScatDist",       SET_CAMP->ScatDist,             INIFileName);
        WritePrivateProfileValue(szCharName, "ScatSize",       SET_CAMP->ScatSize,             INIFileName);
    }

    WINDOW->Save();
}

void LoadConfig()
{
    bool bRewriteIni            = false; // re-save if bad values were read

    // default settings
    float allowMove = GetPrivateProfileFloat("Defaults",   "AllowMove",        SET->AllowMove,     INIFileName);
    if (allowMove >= 10.0f)
    {
        SET->AllowMove = allowMove;
    }
    else
    {
        bRewriteIni = true;
    }
    SET->AutoPause   = GetPrivateProfileBool("Defaults",   "AutoPause",        SET->AutoPause,     INIFileName);
    SET->AutoSave    = GetPrivateProfileBool("Defaults",   "AutoSave",         SET->AutoSave,      INIFileName);
    SET->AutoUW      = GetPrivateProfileBool("Defaults",   "AutoUW",           SET->AutoUW,        INIFileName);
    SET->BreakKB     = GetPrivateProfileBool("Defaults",   "BreakKeyboard",    SET->BreakKB,       INIFileName);
    SET->BreakMouse  = GetPrivateProfileBool("Defaults",   "BreakMouse",       SET->BreakMouse,    INIFileName);
    SET->BreakGM     = GetPrivateProfileBool("Defaults",   "BreakOnGM",        SET->BreakGM,       INIFileName);
    SET->BreakSummon = GetPrivateProfileBool("Defaults",   "BreakOnSummon",    SET->BreakSummon,   INIFileName);

    float distSummon = GetPrivateProfileFloat("Defaults",  "DistSummon",       SET->DistSummon,    INIFileName);
    if (distSummon >= 2.0f)
    {
        SET->DistSummon = distSummon;
    }
    else
    {
        bRewriteIni = true;
    }

    SET->Feign       = GetPrivateProfileBool("Defaults",   "FeignSupport",     SET->Feign,         INIFileName);
    std::string heading = GetPrivateProfileString("Defaults", "Heading", SET->Head == H_TRUE ? "true" : (SET->Head == H_LOOSE ? "loose" : "fast"), INIFileName);
    SET->Head = H_FAST;
    if (ci_equals(heading, "true"))
    {
        SET->Head = H_TRUE;
    }
    else if (!ci_equals(heading, "loose"))
    {
        SET->Head = H_LOOSE;
    }

    SET->PauseKB    = GetPrivateProfileBool("Defaults",    "KeyboardPause",    SET->PauseKB,       INIFileName);
    if (SET->PauseKB) SET->BreakKB = false;
    SET->PauseMouse = GetPrivateProfileBool("Defaults",    "MousePause",       SET->PauseMouse,    INIFileName);
    if (SET->PauseMouse) SET->BreakMouse = false;
    SET->LockPause  = GetPrivateProfileBool("Defaults",    "LockPause",        SET->LockPause,     INIFileName);
    PAUSE->MaxDelay(GetPrivateProfileInt("Defaults",       "PauseMaxDelay",    PAUSE->Max,         INIFileName));
    PAUSE->MinDelay(GetPrivateProfileInt("Defaults",       "PauseMinDelay",    PAUSE->Min,         INIFileName));
    SET->SaveByChar = GetPrivateProfileBool("Defaults",    "SaveByChar",       SET->SaveByChar,    INIFileName);

    float turnRate = GetPrivateProfileFloat("Defaults",    "TurnRate",         SET->TurnRate,      INIFileName);
    if (turnRate >= 1.0f && turnRate <= 100.0f)
    {
        SET->TurnRate = turnRate;
    }
    else
    {
        bRewriteIni = true;
    }

    SET->Window = GetPrivateProfileBool("Defaults",        "UseWindow",        SET->Window,        INIFileName);

    // verbosity flag handling
    bool verbosity = GetPrivateProfileBool("Defaults",     "Verbosity",        (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY, INIFileName);
    if (verbosity)
    {
        uiVerbLevel |= V_VERBOSITY;
    }
    else
    {
        uiVerbLevel &= ~V_VERBOSITY;
    }

    // FullVerbosity is more frequent, detailed output, and differs from Verbosity
    // Setting one does not include the text of the other.
    bool fullVerbosity = GetPrivateProfileBool("Defaults", "FullVerbosity",    (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY, INIFileName);
    if (fullVerbosity)
    {
        uiVerbLevel |= V_FULLVERBOSITY;
    }
    else
    {
        uiVerbLevel &= ~V_FULLVERBOSITY;
    }

    // Total Silence disables all output except extreme error messages and BreakOnSummon
    bool totalSilence = GetPrivateProfileBool("Defaults",  "TotalSilence",     uiVerbLevel == V_SILENCE, INIFileName);
    if (totalSilence)
    {
        uiVerbLevel = V_SILENCE;
    }

    // do not need an else, just leaves flags alone if we aren't silencing
    // if totalsilence is not set to 'on', set flags directly. if ini entry doesn't exist, default will have been set by verb/full reads above
    if (uiVerbLevel != V_SILENCE)
    {
        uiVerbLevel = GetPrivateProfileInt("Defaults", "VerbosityFlags", uiVerbLevel, INIFileName);
        bool autoPauseMsg = GetPrivateProfileBool("Defaults", "AutoPauseMsg",  (uiVerbLevel & V_AUTOPAUSE) == V_AUTOPAUSE, INIFileName);
        if (autoPauseMsg)
        {
            uiVerbLevel |= V_AUTOPAUSE;
        }
        else
        {
            uiVerbLevel &= ~V_AUTOPAUSE;
        }

        bool hideHelp = GetPrivateProfileBool("Defaults",  "HideHelp",         (uiVerbLevel & V_HIDEHELP) == V_HIDEHELP, INIFileName);
        if (hideHelp)
        {
            uiVerbLevel |= V_HIDEHELP;
        }
        else
        {
            uiVerbLevel &= ~V_HIDEHELP;
        }
    }

    SET->WinEQ = GetPrivateProfileBool("Defaults",         "WinEQ",            SET->WinEQ,         INIFileName);

    // stick settings
    SET_S->UW = GetPrivateProfileBool("Stick",             "AlwaysUW",         SET_S->UW,          INIFileName);

    float arcBehind = GetPrivateProfileFloat("Stick",      "ArcBehind",        SET_S->ArcBehind,   INIFileName);
    if (arcBehind > 5.0f && arcBehind < 260.0f)
    {
        SET_S->ArcBehind = arcBehind;
    }
    else
    {
        bRewriteIni = true;
    }

    float arcNotFront = GetPrivateProfileFloat("Stick",    "ArcNotFront",      SET_S->ArcNotFront, INIFileName);
    if (arcNotFront > 5.0f && arcNotFront < 260.0f)
    {
        SET_S->ArcNotFront = arcNotFront;
    }
    else
    {
        bRewriteIni = true;
    }

    SET->Spin = GetPrivateProfileBool("Stick",             "AwareNotAggro",    SET->Spin,          INIFileName);

    SET_S->BreakGate   = GetPrivateProfileBool("Stick",    "BreakOnGate",      SET_S->BreakGate,   INIFileName);
    SET_S->BreakHit    = GetPrivateProfileBool("Stick",    "BreakOnHit",       SET_S->BreakHit,    INIFileName);
    SET_S->BreakTarget = GetPrivateProfileBool("Stick",    "BreakOnTarget",    SET_S->BreakTarget, INIFileName);
    SET_S->BreakWarp   = GetPrivateProfileBool("Stick",    "BreakOnWarp",      SET_S->BreakWarp,   INIFileName);
    if (SET_S->BreakWarp) SET_S->PauseWarp = false;
    SET_S->PauseWarp   = GetPrivateProfileBool("Stick",    "PauseOnWarp",      SET_S->PauseWarp,   INIFileName);
    if (SET_S->PauseWarp) SET_S->BreakWarp = false;
    SET_S->DelayStrafe = GetPrivateProfileBool("Stick",    "DelayStrafe",      SET_S->DelayStrafe, INIFileName);

    float distBackup = GetPrivateProfileFloat("Stick",     "DistBackup",       SET_S->DistBack,    INIFileName);
    if (distBackup >= 1.0f)
    {
        SET_S->DistBack = distBackup;
    }
    else
    {
        bRewriteIni = true;
    }

    float distBreak = GetPrivateProfileFloat("Stick",      "DistBreak",        SET_S->DistBreak,   INIFileName);
    if (distBreak >= 1.0f)
    {
        SET_S->DistBreak = distBreak;
    }
    else
    {
        bRewriteIni = true;
    }

    float distFlex = GetPrivateProfileFloat("Stick",       "DistFlex",         SET_S->DistFlex,    INIFileName);
    if (distFlex >= 2.0f && distFlex <= 20.0f)
    {
        SET_S->DistFlex = distFlex;
    }
    else
    {
        bRewriteIni = true;
    }

    float distMod = GetPrivateProfileFloat("Stick",        "DistMod",          SET_S->DistMod,     INIFileName);
    if (distMod >= 0.0f)
    {
        SET_S->DistMod = distMod;
    }
    else
    {
        bRewriteIni = true;
    }

    float distModP = GetPrivateProfileFloat("Stick",       "DistMod%",         SET_S->DistModP,    INIFileName);
    if (distModP >= 0.0f)
    {
        SET_S->DistModP = distModP;
    }
    else
    {
        bRewriteIni = true;
    }
    float distSnapRoll = GetPrivateProfileFloat("Stick",   "DistSnaproll",     SET_S->DistSnap,    INIFileName);
    if (distSnapRoll >= 1.0f)
    {
        SET_S->DistSnap = distSnapRoll;
    }
    else
    {
        bRewriteIni = true;
    }

    SET_S->MaxDelay(GetPrivateProfileInt("Stick",          "StrafeMaxDelay",  SET_S->Max,          INIFileName));
    SET_S->MinDelay(GetPrivateProfileInt("Stick",          "StrafeMinDelay",  SET_S->Min,          INIFileName));

    // end stick delays
    SET_S->Randomize  = GetPrivateProfileBool("Stick",     "RandomArc",       SET_S->Randomize,    INIFileName);
    SET_S->UseBack    = GetPrivateProfileBool("Stick",     "UseBackward",     SET_S->UseBack,      INIFileName);
    SET_S->UseFleeing = GetPrivateProfileBool("Stick",     "UseFleeing",      SET_S->UseFleeing,   INIFileName);
    SET_S->Flex       = GetPrivateProfileBool("Stick",     "UseFlex",         SET_S->Flex,         INIFileName);
    SET_S->Walk       = GetPrivateProfileBool("Stick",     "UseWalk",         SET_S->Walk,         INIFileName);

    // makecamp settings
    float campRadius = GetPrivateProfileFloat("MakeCamp",  "CampRadius",      SET_CAMP->Radius,    INIFileName);
    if (campRadius >= 5.0f)
    {
        SET_CAMP->SetRadius(campRadius);
    }
    else
    {
        bRewriteIni = true;
    }

    SET_CAMP->MaxDelay(GetPrivateProfileInt("MakeCamp",    "MaxDelay",         SET_CAMP->Max,      INIFileName));
    SET_CAMP->MinDelay(GetPrivateProfileInt("MakeCamp",    "MinDelay",         SET_CAMP->Min,      INIFileName));
    SET_CAMP->Realtime = GetPrivateProfileBool("MakeCamp", "RealtimePlayer",   SET_CAMP->Realtime, INIFileName);
    SET_CAMP->HaveTarget = GetPrivateProfileBool("MakeCamp", "ReturnHaveTarget", SET_CAMP->HaveTarget, INIFileName);
    SET_CAMP->NoAggro = GetPrivateProfileBool("MakeCamp",  "ReturnNoAggro",    SET_CAMP->NoAggro,  INIFileName);
    SET_CAMP->NotLoot = GetPrivateProfileBool("MakeCamp",  "ReturnNotLooting", SET_CAMP->NotLoot,  INIFileName);
    SET_CAMP->Leash = GetPrivateProfileBool("MakeCamp",    "UseLeash",         SET_CAMP->Leash,    INIFileName);

    float leashLen = GetPrivateProfileFloat("MakeCamp",    "LeashLength",      SET_CAMP->Length,   INIFileName);
    if (leashLen >= SET_CAMP->Radius)
    {
        SET_CAMP->SetLeash(leashLen);
    }
    else
    {
        bRewriteIni = true;
    }

    // scatter configuration (makecamp)
    SET_CAMP->Scatter = GetPrivateProfileBool("MakeCamp",  "UseScatter",       SET_CAMP->Scatter,  INIFileName);
    SET_CAMP->Bearing = GetPrivateProfileFloat("MakeCamp", "Bearing",          SET_CAMP->Bearing,  INIFileName);

    float scatDist =    GetPrivateProfileFloat("MakeCamp", "ScatDist",         SET_CAMP->ScatDist, INIFileName);
    if (scatDist >= 1.0f)
    {
        SET_CAMP->ScatDist = scatDist;
    }
    else
    {
        bRewriteIni = true;
    }

    float scatSize = GetPrivateProfileFloat("MakeCamp",    "ScatSize",         SET_CAMP->ScatSize, INIFileName);
    if (scatSize >= 1.0f)
    {
        SET_CAMP->ScatSize = scatSize;
    }
    else
    {
        bRewriteIni = true;
    }

    // moveto settings
    SET_M->UW = GetPrivateProfileBool("MoveTo",            "AlwaysUW",         SET_M->UW,          INIFileName);
    float arrivalDist = GetPrivateProfileFloat("MoveTo",   "ArrivalDist",      SET_M->Dist,        INIFileName);
    if (arrivalDist >= 1.0f)
    {
        SET_M->Dist = arrivalDist;
    }
    else
    {
        bRewriteIni = true;
    }

    float arrivalDistX = GetPrivateProfileFloat("MoveTo",  "ArrivalDistX",     SET_M->DistX,       INIFileName);
    if (arrivalDistX >= 1.0f)
    {
        SET_M->DistX = arrivalDistX;
    }
    else
    {
        bRewriteIni = true;
    }

    float arrivalDistY = GetPrivateProfileFloat("MoveTo",  "ArrivalDistY",     MOVETO->DistY,      INIFileName);
    if (arrivalDistY >= 1.0f)
    {
        SET_M->DistY = arrivalDistY;
    }
    else
    {
        bRewriteIni = true;
    }

    SET_M->BreakAggro = GetPrivateProfileBool("MoveTo",    "BreakOnAggro",     SET_M->BreakAggro,  INIFileName);
    SET_M->BreakHit   = GetPrivateProfileBool("MoveTo",    "BreakOnHit",       SET_M->BreakHit,    INIFileName);

    distBackup        = GetPrivateProfileFloat("MoveTo",   "DistBackup",       SET_M->DistBack,    INIFileName);
    if (distBackup >= 1.0f)
    {
        SET_M->DistBack = distBackup;
    }
    else
    {
        bRewriteIni = true;
    }

    float moveToMod = GetPrivateProfileFloat("MoveTo",     "MoveToMod",        SET_M->Mod,         INIFileName);
    if (moveToMod >= 0.0f)
    {
        SET_M->Mod = moveToMod;
    }
    else
    {
        bRewriteIni = true;
    }

    SET_M->UseBack = GetPrivateProfileBool("MoveTo",       "UseBackward",      SET_M->UseBack,     INIFileName);
    SET_M->Walk    = GetPrivateProfileBool("MoveTo",       "UseWalk",          SET_M->Walk,        INIFileName);

    // circle settings
    SET_C->Backward = GetPrivateProfileBool("Circle",      "Backward",         SET_C->Backward,    INIFileName);
    SET_C->CCW      = GetPrivateProfileBool("Circle",      "CCW",              SET_C->CCW,         INIFileName);
    SET_C->Drunk    = GetPrivateProfileBool("Circle",      "Drunken",          SET_C->Drunk,       INIFileName);

    float radius = GetPrivateProfileFloat("Circle",        "RadiusSize",       SET_C->Radius,      INIFileName);
    if (radius >= 5.0f)
    {
        SET_C->SetRadius(radius);
    }
    else
    {
        bRewriteIni = true;
    }

    // stuck logic related
    STUCK->On       = GetPrivateProfileBool("StuckLogic",  "StuckLogic",       STUCK->On,          INIFileName);
    float distStuck = GetPrivateProfileFloat("StuckLogic", "DistStuck",        STUCK->Dist,        INIFileName);
    if (distStuck > 0.0f)
    {
        STUCK->Dist = distStuck;
    }
    else
    {
        bRewriteIni = true;
    }

    int pulseCheck = GetPrivateProfileInt("StuckLogic",    "PulseCheck",       STUCK->Check,       INIFileName);
    if (pulseCheck > 1)
    {
        STUCK->Check = pulseCheck;
    }
    else
    {
        bRewriteIni = true;
    }

    int pulseUnstuck = GetPrivateProfileInt("StuckLogic",  "PulseUnstuck",     STUCK->Unstuck,     INIFileName);
    if (pulseUnstuck > 1)
    {
        STUCK->Unstuck = pulseUnstuck;
    }
    else
    {
        bRewriteIni = true;
    }

    STUCK->Jump     = GetPrivateProfileBool("StuckLogic",  "TryToJump",        STUCK->Jump,        INIFileName);
    STUCK->TurnHalf = GetPrivateProfileBool("StuckLogic",  "TurnHalf",         STUCK->TurnHalf,    INIFileName);

    // check if we want to explicitly ignore this char's custom ini
    bool ignoreMe = GetPrivateProfileBool(szCharName,      "DisregardMe",      false,              INIFileName);
    if (SET->SaveByChar && !ignoreMe)
    {
        // Character specific
        float allowMove = GetPrivateProfileFloat(szCharName, "AllowMove",      SET->AllowMove,     INIFileName);
        if (allowMove > 10.0f)
        {
            SET->AllowMove = allowMove;
        }
        else
        {
            bRewriteIni = true;
        }

        float arcBehind = GetPrivateProfileFloat(szCharName, "ArcBehind",      SET_S->ArcBehind,   INIFileName);
        if (arcBehind > 5.0f && arcBehind < 260.0f)
        {
            SET_S->ArcBehind = arcBehind;
        }
        else
        {
            bRewriteIni = true;
        }

        float arcNotFront = GetPrivateProfileFloat(szCharName, "ArcNotFront",  SET_S->ArcNotFront, INIFileName);
        if (arcNotFront > 5.0f && arcNotFront < 260.0f)
        {
            SET_S->ArcNotFront = arcNotFront;
        }
        else
        {
            bRewriteIni = true;
        }

        SET->AutoSave    = GetPrivateProfileBool(szCharName, "AutoSave",       SET->AutoSave,      INIFileName);
        SET->AutoUW      = GetPrivateProfileBool(szCharName, "AutoUW",         SET->AutoUW,        INIFileName);
        SET_S->BreakGate = GetPrivateProfileBool(szCharName, "BreakOnGate",    SET_S->BreakGate,   INIFileName);
        SET_S->BreakWarp = GetPrivateProfileBool(szCharName, "BreakOnWarp",    SET_S->BreakWarp,   INIFileName);
        if (SET_S->BreakWarp) SET_S->PauseWarp = false;
        SET_S->PauseWarp = GetPrivateProfileBool(szCharName, "PauseOnWarp",    SET_S->PauseWarp,   INIFileName);
        if (SET_S->PauseWarp) SET_S->BreakWarp = false;
        SET->LockPause   = GetPrivateProfileBool(szCharName, "LockPause",      SET->LockPause,     INIFileName);

        float distBreak  = GetPrivateProfileFloat(szCharName, "DistBreak",     STICK->DistBreak,   INIFileName);
        if (distBreak >= 1.0f)
        {
            STICK->DistBreak = distBreak;
        }
        else
        {
            bRewriteIni = true;
        }

        float distSnapRoll = GetPrivateProfileFloat(szCharName, "DistSnaproll", SET_S->DistSnap,   INIFileName);
        if (distSnapRoll >= 1.0f)
        {
            SET_S->DistSnap = distSnapRoll;
        }
        else
        {
            bRewriteIni = true;
        }

        SET->Feign = GetPrivateProfileBool(szCharName, "FeignSupport",         SET->Feign,         INIFileName);
        std::string heading = GetPrivateProfileString(szCharName, "Heading",   SET->Head == H_TRUE ? "true" : SET->Head == H_LOOSE ? "loose" : "fast", INIFileName);
        SET->Head = H_FAST;
        if (ci_equals(heading, "true"))
        {
            SET->Head = H_TRUE;
        }
        else if (ci_equals(heading, "loose"))
        {
            SET->Head = H_LOOSE;
        }

        float leashLen = GetPrivateProfileFloat(szCharName, "LeashLength",     SET_CAMP->Length,   INIFileName);
        if (leashLen >= SET_CAMP->Radius)
        {
            SET_CAMP->SetLeash(leashLen);
        }
        else
        {
            bRewriteIni = true;
        }

        SET_CAMP->Leash = GetPrivateProfileBool(szCharName, "UseLeash",        SET_CAMP->Leash,    INIFileName);
        SET->Window = GetPrivateProfileBool(szCharName,     "UseWindow",       SET->Window,        INIFileName);

        // verbosity flag handling
        bool verbosity = GetPrivateProfileBool(szCharName, "Verbosity",        (uiVerbLevel & V_VERBOSITY) == V_VERBOSITY, INIFileName);
        if (verbosity)
        {
            uiVerbLevel |= V_VERBOSITY;
        }
        else
        {
            uiVerbLevel &= ~V_VERBOSITY;
        }

        // FullVerbosity is more frequent, detailed output, and differs from Verbosity
        // Setting one does not include the text of the other.
        bool fullVerbosity = GetPrivateProfileBool(szCharName, "FullVerbosity", (uiVerbLevel & V_FULLVERBOSITY) == V_FULLVERBOSITY, INIFileName);
        if (fullVerbosity)
        {
            uiVerbLevel |= V_FULLVERBOSITY;
        }
        else
        {
            uiVerbLevel &= ~V_FULLVERBOSITY;
        }

        // set flags directly, if no entry present use results of above reads
        uiVerbLevel = GetPrivateProfileInt(szCharName, "VerbosityFlags", uiVerbLevel, INIFileName);

        float campRadius = GetPrivateProfileFloat(szCharName, "CampRadius",    SET_CAMP->Radius,   INIFileName);
        if (campRadius >= 5.0f)
        {
            SET_CAMP->SetRadius(campRadius);
        }
        else
        {
            bRewriteIni = true;
        }

        SET_CAMP->Realtime = GetPrivateProfileBool(szCharName, "RealtimePlayer", SET_CAMP->Realtime, INIFileName);
        // scatter configuration (makecamp)
        SET_CAMP->Scatter = GetPrivateProfileBool(szCharName, "UseScatter",    SET_CAMP->Scatter,  INIFileName);
        SET_CAMP->Bearing = GetPrivateProfileFloat(szCharName, "Bearing",      SET_CAMP->Bearing,  INIFileName);
        float scatDist    = GetPrivateProfileFloat(szCharName, "ScatDist",     SET_CAMP->ScatDist, INIFileName);
        if (scatDist >= 1.0f)
        {
            SET_CAMP->ScatDist = scatDist;
        }
        else
        {
            bRewriteIni = true;
        }

        float scatSize = GetPrivateProfileFloat(szCharName, "ScatSize",        SET_CAMP->ScatSize, INIFileName);
        if (scatSize >= 1.0f)
        {
            SET_CAMP->ScatSize = scatSize;
        }
        else
        {
            bRewriteIni = true;
        }
    }

    if (bRewriteIni)
    {
        SaveConfig();
    }

    // copy new setting defaults to active
    pMU->NewDefaults();

    // create window if needed
    if (SET->Window)
    {
        WINDOW->Create();
    }
    else
    {
        WINDOW->Destroy(true);
    }
}

// End file input / output
// ----------------------------------------
// Keybind handling

void KeyKiller(int iKeyPressed)
{
    if (iKeyPressed == CMD_FORWARD)
    {
        MOVE->DoMove(KILL_STRAFE, false, MU_WALKOFF);
        MOVE->DoMove(GO_BACKWARD, false, MU_WALKOFF);
    }
    else if (iKeyPressed == CMD_BACK)
    {
        MOVE->DoMove(KILL_STRAFE, false, MU_WALKOFF);
        MOVE->DoMove(GO_FORWARD,  false, MU_WALKOFF);
    }
    else if (iKeyPressed == CMD_STRAFE_LEFT)
    {
        MOVE->DoMove(KILL_FB,     false, MU_WALKOFF);
        MOVE->DoMove(GO_RIGHT,    false, MU_WALKOFF);
    }
    else if (iKeyPressed == CMD_STRAFE_RIGHT)
    {
        MOVE->DoMove(KILL_FB,     false, MU_WALKOFF);
        MOVE->DoMove(GO_LEFT,     false, MU_WALKOFF);
    }
    else if (iKeyPressed == CMD_AUTORUN)
    {
        MOVE->DoMove(KILL_STRAFE, false, MU_WALKOFF);
    }
    else if (iKeyPressed == CMD_RIGHT)
    {
        MOVE->DoMove(APPLY_TO_ALL, false, MU_WALKOFF);
        MOVE->ChangeHead = H_INACTIVE;
        if (pMU->Head == H_TRUE)
        {
            pEverQuestInfo->keyDown[CMD_LEFT] = 0;
            pKeypressHandler->CommandState[CMD_LEFT] = 0;
        }
        return; // return so that we do not auto heading adjust via StopHeading()
    }
    else if (iKeyPressed == CMD_LEFT)
    {
        MOVE->DoMove(APPLY_TO_ALL, false, MU_WALKOFF);
        MOVE->ChangeHead = H_INACTIVE;
        if (pMU->Head == H_TRUE)
        {
            pEverQuestInfo->keyDown[CMD_RIGHT] = 0;
            pKeypressHandler->CommandState[CMD_RIGHT] = 0;
        }
        return; // return so that we do not auto heading adjust via StopHeading()
    }
    MOVE->StopHeading();
}

void KeybindPressed(int iKeyPressed, int iKeyDown)
{
    if (!ValidIngame(false))
        return;

    if (pMU->Rooted)
    {
        MOVE->DoRoot();
        return;
    }

    //MOVE->ChangeHead = H_INACTIVE; // true/loose fights for heading if not reset
    bool bCmdActive = (STICK->Ready() || MOVETO->On || CIRCLE->On || CAMP->Auto) ? true : false;

    if (iKeyDown)
    {
        PAUSE->UserKB = true; // so makecamp wont auto return
        PAUSE->TimeStop(); // stop return time from kicking in if you let go of the key and repress again.
        if (!bCmdActive || PAUSE->PausedCmd) return; // do nothing else if plugin not currently moving or 'pause' from user already active

        if (SET->PauseKB)
        {
            if (MOVETO->On && CURCAMP->On && CURCAMP->Leash && !CAMP->Auto)
            {
                sprintf_s(szMsg, "\ay%s\aw:: Ended '/moveto' or '/makecamp return' because leash is on.", PLUGIN_NAME);
                WriteLine(szMsg, V_MAKECAMPFV);
                EndPreviousCmd(false);
            }
            PAUSE->PausedMU = true;
            if (!pMU->KeyKilled && !SET->WinEQ)
            {
                KeyKiller(iKeyPressed);
                pMU->KeyKilled = true;
            }
        }
        else if (SET->BreakKB)
        {
            if (!CAMP->Auto)
            {
                sprintf_s(szMsg, "\ay%s\aw:: Current command ended from manual movement.", PLUGIN_NAME);
                WriteLine(szMsg, V_MOVEPAUSE);
            }
            EndPreviousCmd(false); //EndPreviousCmd(true); // this stops kb input
            if (!pMU->KeyKilled && !SET->WinEQ)
            {
                KeyKiller(iKeyPressed);
                pMU->KeyKilled = true;
            }
        }
    }
    else
    {
        if (!SET->WinEQ)
        {
            if (pEverQuestInfo->keyDown[CMD_FORWARD]
                || pEverQuestInfo->keyDown[CMD_BACK]
                || pEverQuestInfo->keyDown[CMD_LEFT]
                || pEverQuestInfo->keyDown[CMD_RIGHT]
                || pEverQuestInfo->keyDown[CMD_STRAFE_LEFT]
                || pEverQuestInfo->keyDown[CMD_STRAFE_RIGHT]
                || pEverQuestInfo->AutoRun)
            {
                // return until all keys let go
                return;
            }
        }

        PAUSE->UserKB = pMU->KeyKilled = false;
        if (!SET->PauseKB || !bCmdActive || PAUSE->PausedCmd) return; // return if no reason to resume
        PAUSE->TimeStart(); // start resume timer, picked back up OnPulse()
    }
}

void FwdWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_FORWARD, iKeyDown);
}

void BckWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_BACK, iKeyDown);
}

void LftWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_LEFT, iKeyDown);
}

void RgtWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_RIGHT, iKeyDown);
}

void StrafeLftWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_STRAFE_LEFT, iKeyDown);
}

void StrafeRgtWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_STRAFE_RIGHT, iKeyDown);
}

void AutoRunWrapper(const char* szName, bool iKeyDown)
{
    KeybindPressed(CMD_AUTORUN, iKeyDown);
}

void DoKeybinds()
{
    if (!pMU || pMU->Keybinds) return;
    AddMQ2KeyBind("MUTILS_FWD",        FwdWrapper);
    AddMQ2KeyBind("MUTILS_BCK",        BckWrapper);
    AddMQ2KeyBind("MUTILS_LFT",        LftWrapper);
    AddMQ2KeyBind("MUTILS_RGT",        RgtWrapper);
    AddMQ2KeyBind("MUTILS_STRAFE_LFT", StrafeLftWrapper);
    AddMQ2KeyBind("MUTILS_STRAFE_RGT", StrafeRgtWrapper);
    AddMQ2KeyBind("MUTILS_AUTORUN",    AutoRunWrapper);
    SetMQ2KeyBind("MUTILS_FWD",        false, pKeypressHandler->NormalKey[CMD_FORWARD]);
    SetMQ2KeyBind("MUTILS_BCK",        false, pKeypressHandler->NormalKey[CMD_BACK]);
    SetMQ2KeyBind("MUTILS_LFT",        false, pKeypressHandler->NormalKey[CMD_LEFT]);
    SetMQ2KeyBind("MUTILS_RGT",        false, pKeypressHandler->NormalKey[CMD_RIGHT]);
    SetMQ2KeyBind("MUTILS_STRAFE_LFT", false, pKeypressHandler->NormalKey[CMD_STRAFE_LEFT]);
    SetMQ2KeyBind("MUTILS_STRAFE_RGT", false, pKeypressHandler->NormalKey[CMD_STRAFE_RIGHT]);
    SetMQ2KeyBind("MUTILS_AUTORUN",    false, pKeypressHandler->NormalKey[CMD_AUTORUN]);
    SetMQ2KeyBind("MUTILS_FWD",        true,  pKeypressHandler->AltKey[CMD_FORWARD]);
    SetMQ2KeyBind("MUTILS_BCK",        true,  pKeypressHandler->AltKey[CMD_BACK]);
    SetMQ2KeyBind("MUTILS_LFT",        true,  pKeypressHandler->AltKey[CMD_LEFT]);
    SetMQ2KeyBind("MUTILS_RGT",        true,  pKeypressHandler->AltKey[CMD_RIGHT]);
    SetMQ2KeyBind("MUTILS_STRAFE_LFT", true,  pKeypressHandler->AltKey[CMD_STRAFE_LEFT]);
    SetMQ2KeyBind("MUTILS_STRAFE_RGT", true,  pKeypressHandler->AltKey[CMD_STRAFE_RIGHT]);
    SetMQ2KeyBind("MUTILS_AUTORUN",    true,  pKeypressHandler->AltKey[CMD_AUTORUN]);
    pMU->Keybinds = true;
}

void UndoKeybinds()
{
    if (!pMU || !pMU->Keybinds) return;
    RemoveMQ2KeyBind("MUTILS_FWD");
    RemoveMQ2KeyBind("MUTILS_BCK");
    RemoveMQ2KeyBind("MUTILS_LFT");
    RemoveMQ2KeyBind("MUTILS_RGT");
    RemoveMQ2KeyBind("MUTILS_STRAFE_LFT");
    RemoveMQ2KeyBind("MUTILS_STRAFE_RGT");
    RemoveMQ2KeyBind("MUTILS_AUTORUN");
    pMU->Keybinds = false;
}

// End keybind handling

// ---------------------------------------------------------------------------
// MQ2 Exported functions
unsigned int __stdcall MQ2DataVariableLookup(char * VarName, char * Value,size_t ValueLen)
{
	strcpy_s(Value, ValueLen, VarName);
	if (pLocalPC) {
		return (unsigned int)strlen(ParseMacroParameter(Value, ValueLen));
	}
	return (unsigned int)strlen(Value);
}
PLUGIN_API void InitializePlugin()
{
    pMoveEvent = new Blech('#','|',MQ2DataVariableLookup);

    // commands
    AddCommand("/makecamp",  MakeCampWrapper, FALSE, TRUE, TRUE);
    AddCommand("/moveto",    MoveToWrapper,   FALSE, TRUE, TRUE);
    AddCommand("/stick",     StickWrapper,    FALSE, TRUE, TRUE);
    AddCommand("/circle",    CircleWrapper,   FALSE, TRUE, TRUE);
    AddCommand("/calcangle", CalcOurAngle,    FALSE, TRUE, TRUE);
    AddCommand("/rootme",    RootCmd,         FALSE, TRUE, TRUE);

    srand((unsigned int)time(0));

    // setup global vars
    sprintf_s(szDebugName, "%s\\MQ2MoveUtils-debug.ini", gPathConfig);

    // instance classes
    ME     = new CMUCharacter();
    SET    = new CMUSettings();
    MOVE   = new CMUMovement();
    WINDOW = new CMUWndHandler();
    pMU    = new CMUActive();

    // TLO
    AddMQ2Data("Stick",     dataStick);
    AddMQ2Data("MakeCamp",  dataMakeCamp);
    AddMQ2Data("MoveTo",    dataMoveTo);
    AddMQ2Data("Circle",    dataCircling);
    AddMQ2Data("MoveUtils", dataMoveUtils);

    pStickType     = new MQ2StickType;
    pMakeCampType  = new MQ2MakeCampType;
    pMoveToType    = new MQ2MoveToType;
    pCircleType    = new MQ2CircleType;
    pMoveUtilsType = new MQ2MoveUtilsType;

    // setup mq2melee pointers
	if (pbMULoaded = (bool*)GetPluginProc("MQ2Melee", "bMULoaded"))
		*pbMULoaded = true;
}

PLUGIN_API void ShutdownPlugin()
{
    // remove commands
    RemoveCommand("/makecamp");
    RemoveCommand("/moveto");
    RemoveCommand("/stick");
    RemoveCommand("/circle");
    RemoveCommand("/calcangle");
    RemoveCommand("/rootme");

    // remove TLOs
    RemoveMQ2Data("Stick");
    RemoveMQ2Data("MakeCamp");
    RemoveMQ2Data("MoveTo");
    RemoveMQ2Data("Circle");
    RemoveMQ2Data("MoveUtils");

    delete pStickType;
    delete pMoveToType;
    delete pMakeCampType;
    delete pCircleType;
    delete pMoveUtilsType;

    // destroy mq2 linkage
    UndoKeybinds();
    SetupEvents(false, true);
	if (pMoveEvent) {
		pMoveEvent->Reset();
		delete pMoveEvent;
		pMoveEvent = 0;
	}
    // destroy mq2 melee linkage
	if (pbMULoaded = (bool*)GetPluginProc("MQ2Melee", "bMULoaded"))
		*pbMULoaded = false;

    // destroy UI window
    WINDOW->Destroy(ValidIngame());

    // do not leave character walking
    MOVE->SetWalk(false);

    // destroy classes
    delete ME;
    delete WINDOW;
    delete MOVE;
    delete pMU;
    delete SET;
}

PLUGIN_API void SetGameState(unsigned long ulGameState)
{
    //DebugToDebugger("SetGameState %d", ulGameState);
    if (ulGameState == GAMESTATE_INGAME)
    {
        sprintf_s(szCharName, "%s.%s", GetServerShortName(), pLocalPC->Name);

        if (!pMU->Keybinds)
        {
            DoKeybinds();
        }
        if (!pMU->Loaded)
        {
            LoadConfig();
            pMU->Loaded = true;
        }
        SetupEvents(true);

        // randomly flip arc flag
        if (rand() % 100 > 50) STICK->RandFlag = !STICK->RandFlag;
        // draw window if enabled
        WINDOW->Create();
        // turn walking off
        MOVE->SetWalk(false);
        // keybind bug fix
        pMU->KeyKilled = false;
    }
    else
    {
        if (pMU->Active() || CURCAMP->On)
        {
            sprintf_s(szMsg, "\ay%s\aw:: GameState change ended previous command.", PLUGIN_NAME);
            WriteLine(szMsg, V_SILENCE);
        }
        EndPreviousCmd(false); // using true here causes ctd
        CAMP->ResetBoth();
        pMU->MovetoBroke = pMU->StoppedMoveto = pMU->StickBroke = false;
        if (ulGameState == GAMESTATE_CHARSELECT)
        {
            UndoKeybinds();
            pMU->Loaded = false;
            SetupEvents(false, true);
            WINDOW->Destroy(true);
            pMU->BrokeGM = pMU->BrokeSummon = false;
        }
    }
}

PLUGIN_API void OnReloadUI()
{
    WINDOW->Create();
}

PLUGIN_API void OnCleanUI()
{
    WINDOW->Destroy(true);
}

PLUGIN_API void OnZoned()
{
    EndPreviousCmd(false); // no need to use true here
    CAMP->ResetBoth();
    // keybind bug fix
    pMU->KeyKilled = false;
}

PLUGIN_API void OnAddSpawn(PlayerClient* pNewSpawn)
{
    if (SET->BreakGM || !pNewSpawn->SpawnID)
    {
        return;
    }

    if (pNewSpawn->GM)
    {
        __time64_t tCurrentTime;
        char szTime[30] = {};
        struct tm THE_TIME = {};
#ifdef OLD_COMPILER_USER
        time(&tCurrentTime);
        THE_TIME = localtime(&tCurrentTime);
#else
        _time64(&tCurrentTime);
        errno_t err = _localtime64_s(&THE_TIME,&tCurrentTime);
#endif
        strftime(szTime, 20, " [%H:%M:%S]", &THE_TIME);
        sprintf_s(szMsg, "\ay%s\aw:: \arWARNING\ax Plugin halted from\ag [GM] %s\ax in zone. -- \aw%s", PLUGIN_NAME, pNewSpawn->DisplayedName, szTime);
        WriteLine(szMsg, V_BREAKONGM);
        pMU->BrokeGM = true;
        EndPreviousCmd(ValidIngame());
    }
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pOldSpawn)
{
    if (!pOldSpawn->SpawnID) return;

    if (pOldSpawn->SpawnID == STICK->HoldID)
    {
        STICK->StopHold();
    }

    if (pOldSpawn->SpawnID == CURCAMP->PcID)
    {
        CAMP->ResetPlayer(true);
    }

    if (pOldSpawn->GM && SET->BreakGM && pMU && pMU->BrokeGM)
    {
        __time64_t tCurrentTime;
        char szTime[30] = {};
        struct tm THE_TIME = {};
#ifdef OLD_COMPILER_USER
        time(&tCurrentTime);
        THE_TIME = localtime(&tCurrentTime);
#else
        _time64(&tCurrentTime);
        errno_t err = _localtime64_s(&THE_TIME,&tCurrentTime);
#endif
        strftime(szTime, 20, " [%H:%M:%S]", &THE_TIME);
        sprintf_s(szMsg, "\ay%s\aw::\ag [GM] %s\ax has left the zone or turned invisible. Use \ag/stick imsafe\ax to allow command usage. -- \aw%s", PLUGIN_NAME, pOldSpawn->DisplayedName, szTime);
        WriteLine(szMsg, V_BREAKONGM);
    }
}

PLUGIN_API void OnPulse()
{
    if (!ValidIngame(false)) return;

    if (InHoverState())
    {
        // keep window up when hovering
        WINDOW->Hover();
        // end root command if active
        MOVE->StopRoot();
        // end movement commands (not camp) when hovering
        if (pMU->Active())
        {
            EndPreviousCmd(false);
            sprintf_s(szMsg, "\ay%s\aw:: \arYour death has ended the previous command.", PLUGIN_NAME);
            WriteLine(szMsg, V_SILENCE);
        }
        pMU->FixWalk = true;
        return;
    }
    // if we died, turn walking off when we rez
    if (pMU->FixWalk)
    {
        MOVE->SetWalk(false);
        pMU->FixWalk = false;
    }

    // heading adjustment if needed
    MOVE->AutoHead();
    // MoveUtils.Aggro TLO
    pMU->AggroTLO();
    // check BreakOnSummon and BreakOnGM
    if (pMU->Broken())
		return;
    // rootme command
    MOVE->DoRoot();
    // handle mousepause & movepause timers
    PAUSE->HandlePause();
    // if plugin is paused, do not process commands
    if (PAUSE->Waiting()) return;

    // no initial checks preventing main process call
    // check if individual commands are active & ready
    if (STICK->On)
    {
        if (!STICK->Ready()) return;
        MainProcess(CMD_STICK);
    }
    else if (MOVETO->On)
    {
        MainProcess(CMD_MOVETO);
    }
    else if (CIRCLE->On)
    {
        if (CIRCLE->Drunk && CIRCLE->Wait()) return;
        MainProcess(CMD_CIRCLE);
    }
    else if (!PAUSE->UserMouse && (CURCAMP->On || CAMP->DoAlt))
    {
        // this will process camp returns when no other commands active
        MainProcess(CMD_MAKECAMP);
    }
}

// ---------------------------------------------------------------------------
// old debug info
//
    //PlayerClient* pChSpawn = pCharSpawn;
    //PlayerClient* psTarget = pTarget;
    //DebugSpew("For - %x - Back - %x - Left - %x - Right - %x", pKeypressHandler->CommandState[iForward], pKeypressHandler->CommandState[iBackward], pKeypressHandler->CommandState[iStrafeLeft], pKeypressHandler->CommandState[iStrafeRight]);
    /*char szHead[100] = {};
    sprintf_s(szHead, "%.4f", pChSpawn->Heading);
    DebugSpew("Heading %s", szHead);
    return;*/
    /*if (!psTarget) return;
    float fTempCalc = CalcAngularDist(psTarget->Heading, pChSpawn->Heading);
    DebugSpew("fAngDist: %.2f", fTempCalc);
    //return;*/
    // 19 = jump while moving, 20 = downwards fall from jump only
    // 24 = duck and START of jump, 71/32 = normal stand, 34 = turning left/right
    // 33 = on the way to sit, 38 = has sat down
    // 72 = shake the head shit after 5-6 seconds of standing still
    // 18 = run forward, 17=slow run (backwards, or end of run forward as decelerating, or strafe sideways)
    // 21 = falling through air
    //DebugSpew("Anim: %d, StandState: %d, pspawn anim: %d", pChSpawn->Animation, pChSpawn->StandState, pChSpawn->pSpawn->Animation);
    // values for  ->Animation and ->pSpawn->Animation are always the same
    // uncomment to spew real turn values using right/left keys
    // they do gradually increase if any want to try to solve the real scaling formula
    // otherwise 16.0 seems to be the max and a safe turn value
    /*static float fHeadChangeTime = pChSpawn->Heading;
    char szAnotherTemp[MAX_STRING] = {};
    sprintf_s(szAnotherTemp, "%.2f", fHeadChangeTime - pChSpawn->Heading);
    DebugSpew("Heading Change %s Speed %.2f", szAnotherTemp, pChSpawn->SpeedHeading);
    fHeadChangeTime = pChSpawn->Heading;
    //return;*/
// ---------------------------------------------------------------------------
PLUGIN_API unsigned long OnIncomingChat(char* Line, unsigned long Color)
{
    if (gbInZone && pMoveEvent)
    {
        char szLine[MAX_STRING];
        strcpy_s(szLine, Line);
        pMoveEvent->Feed(szLine);
    }
    return 0;
}
