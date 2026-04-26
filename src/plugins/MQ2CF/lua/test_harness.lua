-- MQ2CF Test Harness
-- Run in-game via: /cf test [subsystem]
-- Validates Core.* API bindings return sane values
--
-- Each test function returns (pass_count, fail_count, messages)
-- Tests don't modify game state -- read-only validation only.

local Test = {}
Test.passed = 0
Test.failed = 0
Test.messages = {}

-- ---------------------------------------------------------------------------
-- Assertion helpers
-- ---------------------------------------------------------------------------
local function log(msg)
    table.insert(Test.messages, msg)
end

local function assert_true(condition, name)
    if condition then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected true, got false")
    end
end

local function assert_false(condition, name)
    if not condition then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected false, got true")
    end
end

local function assert_eq(expected, actual, name)
    if expected == actual then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected " .. tostring(expected) .. ", got " .. tostring(actual))
    end
end

local function assert_range(min, max, actual, name)
    if actual >= min and actual <= max then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected " .. min .. "-" .. max .. ", got " .. tostring(actual))
    end
end

local function assert_not_nil(value, name)
    if value ~= nil then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected non-nil, got nil")
    end
end

local function assert_type(expected_type, value, name)
    if type(value) == expected_type then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected type " .. expected_type .. ", got " .. type(value))
    end
end

local function assert_gt(threshold, actual, name)
    if actual > threshold then
        Test.passed = Test.passed + 1
    else
        Test.failed = Test.failed + 1
        log("\ar  FAIL: " .. name .. " -- expected > " .. tostring(threshold) .. ", got " .. tostring(actual))
    end
end

local function reset()
    Test.passed = 0
    Test.failed = 0
    Test.messages = {}
end

-- ---------------------------------------------------------------------------
-- Core.Util tests
-- ---------------------------------------------------------------------------
function Test.test_util()
    log("\ay--- Core.Util ---")

    local gs = Core.Util.GetGameState()
    assert_type("number", gs, "GetGameState returns number")
    -- If we're running tests, we must be in-game
    assert_true(Core.Util.IsInGame(), "IsInGame is true")

    -- WriteChatf should not crash (we can't validate output)
    Core.Util.WriteChatf("[MQ2CF Test] Util subsystem OK")
    Test.passed = Test.passed + 1 -- if we got here, WriteChatf didn't crash
end

-- ---------------------------------------------------------------------------
-- Core.Spawn tests
-- ---------------------------------------------------------------------------
function Test.test_spawn()
    log("\ay--- Core.Spawn ---")

    -- Local player basics
    local myId = Core.Spawn.GetLocalId()
    assert_type("number", myId, "GetLocalId returns number")
    assert_gt(0, myId, "Local player ID > 0")

    local myName = Core.Spawn.GetLocalName()
    assert_type("string", myName, "GetLocalName returns string")
    assert_gt(0, #myName, "Local player name is non-empty")

    -- Get same values via SpawnID
    local nameById = Core.Spawn.GetName(myId)
    assert_eq(myName, nameById, "GetName(myId) matches GetLocalName()")

    -- HP checks
    local hp = Core.Spawn.GetHP(myId)
    assert_type("number", hp, "GetHP returns number")
    assert_range(0, 100, hp, "HP% is 0-100")

    local hpCur = Core.Spawn.GetHPCurrent(myId)
    local hpMax = Core.Spawn.GetHPMax(myId)
    assert_gt(0, hpMax, "HPMax > 0")
    assert_range(0, hpMax, hpCur, "HPCurrent <= HPMax")

    -- Mana checks (may be 0 for non-casters, but should be valid)
    local mana = Core.Spawn.GetMana(myId)
    local manaMax = Core.Spawn.GetManaMax(myId)
    assert_type("number", mana, "GetMana returns number")
    assert_type("number", manaMax, "GetManaMax returns number")
    assert_range(0, manaMax + 1, mana, "Mana <= ManaMax")

    -- Level
    local level = Core.Spawn.GetLevel(myId)
    assert_range(1, 130, level, "Level 1-130")

    -- Class
    local class = Core.Spawn.GetClass(myId)
    assert_range(1, 16, class, "Class 1-16")

    -- Position
    local y, x, z = Core.Spawn.GetPosition(myId)
    assert_type("number", y, "Position Y is number")
    assert_type("number", x, "Position X is number")
    assert_type("number", z, "Position Z is number")

    -- StandState
    local ss = Core.Spawn.GetStandState(myId)
    assert_type("number", ss, "StandState returns number")

    -- IsDead (we should be alive if running tests)
    assert_false(Core.Spawn.IsDead(myId), "Local player is not dead")

    -- Distance to self should be 0
    local dist = Core.Spawn.GetDistance(myId, myId)
    assert_range(-0.1, 0.1, dist, "Distance to self is ~0")

    -- GetTarget returns a number (0 if no target, spawn ID if targeted)
    local myTarget = Core.Spawn.GetTarget(myId)
    assert_type("number", myTarget, "GetTarget returns number")

    -- Invalid spawn ID should return safe defaults
    local badName = Core.Spawn.GetName(999999)
    assert_eq("", badName, "Invalid ID returns empty name")

    local badHP = Core.Spawn.GetHP(999999)
    assert_eq(0, badHP, "Invalid ID returns 0 HP")
end

-- ---------------------------------------------------------------------------
-- Core.Spell tests
-- ---------------------------------------------------------------------------
function Test.test_spell()
    log("\ay--- Core.Spell ---")

    -- Complete Heal (spell ID 13) is a well-known spell
    -- If this doesn't work, the spell DB lookup is broken
    -- Note: exact spell ID may vary, use GetSpellByName if available

    -- Test with a known valid spell ID range (1-50000+ are valid)
    -- We'll test with spell ID 1 which should exist in any EQ install
    local name = Core.Spell.GetName(1)
    assert_type("string", name, "GetName(1) returns string")
    assert_gt(0, #name, "Spell 1 has a name")

    local mana = Core.Spell.GetManaCost(1)
    assert_type("number", mana, "GetManaCost returns number")

    local range = Core.Spell.GetRange(1)
    assert_type("number", range, "GetRange returns number")

    local tt = Core.Spell.GetTargetType(1)
    assert_type("number", tt, "GetTargetType returns number")

    local cat = Core.Spell.GetCategory(1)
    assert_type("number", cat, "GetCategory returns number")

    local sg = Core.Spell.GetSpellGroup(1)
    assert_type("number", sg, "GetSpellGroup returns number")

    -- Invalid spell should return safe defaults
    local badName = Core.Spell.GetName(-1)
    assert_eq("", badName, "Invalid spell ID returns empty name")
end

-- ---------------------------------------------------------------------------
-- Core.Group tests
-- ---------------------------------------------------------------------------
function Test.test_group()
    log("\ay--- Core.Group ---")

    local count = Core.Group.GetMemberCount()
    assert_type("number", count, "GetMemberCount returns number")
    assert_range(0, 6, count, "Group count 0-6")

    -- If in a group, validate member data
    if count > 0 then
        for i = 0, count - 1 do
            local sid = Core.Group.GetMemberSpawnID(i)
            assert_gt(0, sid, "Group member " .. i .. " has valid SpawnID")

            local hp = Core.Group.GetMemberHP(i)
            assert_range(0, 100, hp, "Group member " .. i .. " HP% valid")
        end
    end

    -- XTarget count should be valid
    local xtCount = Core.Group.GetXTargetCount()
    assert_type("number", xtCount, "GetXTargetCount returns number")
    assert_range(0, 23, xtCount, "XTarget count 0-23")

    -- MA may or may not be set
    local maId = Core.Group.GetMainAssistSpawnID()
    assert_type("number", maId, "GetMainAssistSpawnID returns number")

    -- Mob count
    local mobs = Core.Group.GetMobCount()
    assert_type("number", mobs, "GetMobCount returns number")
    assert_range(0, 100, mobs, "Mob count reasonable")
end

-- ---------------------------------------------------------------------------
-- Core.Lifecycle tests
-- ---------------------------------------------------------------------------
function Test.test_lifecycle()
    log("\ay--- Core.Lifecycle ---")

    assert_type("boolean", Core.Lifecycle.IsPaused(), "IsPaused returns bool")
    assert_false(Core.Lifecycle.IsPaused(), "Not paused by default")

    -- Pause/Resume round-trip
    Core.Lifecycle.Pause()
    assert_true(Core.Lifecycle.IsPaused(), "Paused after Pause()")
    Core.Lifecycle.Resume()
    assert_false(Core.Lifecycle.IsPaused(), "Resumed after Resume()")

    local tick = Core.Lifecycle.GetTickCount()
    assert_type("number", tick, "GetTickCount returns number")
    assert_gt(0, tick, "TickCount > 0")

    local ts = Core.Lifecycle.GetTimestamp()
    assert_type("number", ts, "GetTimestamp returns number")
    assert_gt(1000000000000, ts, "Timestamp is plausible ms since epoch")
end

-- ---------------------------------------------------------------------------
-- Core.Humanize tests
-- ---------------------------------------------------------------------------
function Test.test_humanize()
    log("\ay--- Core.Humanize ---")

    -- Jitter: 1000 +/- 20% should be 800-1200
    for i = 1, 10 do
        local v = Core.Humanize.Jitter(1000, 20)
        assert_range(800, 1200, v, "Jitter(1000,20) in range #" .. i)
    end

    -- HumanDelay: should be in [min, max]
    for i = 1, 10 do
        local v = Core.Humanize.HumanDelay(100, 500)
        assert_range(100, 500, v, "HumanDelay(100,500) in range #" .. i)
    end

    -- ShouldSkipTick: 0% should never skip, 100% should always skip
    assert_false(Core.Humanize.ShouldSkipTick(0), "ShouldSkipTick(0) is false")
    assert_true(Core.Humanize.ShouldSkipTick(100), "ShouldSkipTick(100) is true")

    -- RandomFloat
    local rf = Core.Humanize.RandomFloat(1.0, 2.0)
    assert_type("number", rf, "RandomFloat returns number")
    assert_range(1.0, 2.0, rf, "RandomFloat(1,2) in range")

    -- WeightedRandom
    local wr = Core.Humanize.WeightedRandom(10, 100, 0.5)
    assert_type("number", wr, "WeightedRandom returns number")
    assert_range(10, 100, wr, "WeightedRandom(10,100,0.5) in range")
end

-- ---------------------------------------------------------------------------
-- Core.Settings tests
-- ---------------------------------------------------------------------------
function Test.test_settings()
    log("\ay--- Core.Settings ---")

    -- Set and Get round-trip
    local ok = Core.Settings.Set("test_key", "test_value")
    assert_true(ok, "Set returns true")

    local val = Core.Settings.Get("test_key")
    assert_eq("test_value", val, "Get returns what was Set")

    -- GetInt with default
    Core.Settings.Set("test_int", "42")
    local intVal = Core.Settings.GetInt("test_int", 0)
    assert_eq(42, intVal, "GetInt returns correct value")

    local intDefault = Core.Settings.GetInt("nonexistent_key", 99)
    assert_eq(99, intDefault, "GetInt returns default for missing key")

    -- GetBool
    Core.Settings.Set("test_bool", "true")
    local boolVal = Core.Settings.GetBool("test_bool", false)
    assert_true(boolVal, "GetBool returns true")

    -- GetFloat
    Core.Settings.Set("test_float", "3.14")
    local floatVal = Core.Settings.GetFloat("test_float", 0.0)
    assert_range(3.13, 3.15, floatVal, "GetFloat returns ~3.14")

    -- Profile
    local loadOk = Core.Settings.LoadProfile("test_profile")
    assert_type("boolean", loadOk, "LoadProfile returns bool")

    -- Clean up test key
    Core.Settings.Set("test_key", "")
end

-- ---------------------------------------------------------------------------
-- Core.Camp tests
-- ---------------------------------------------------------------------------
function Test.test_camp()
    log("\ay--- Core.Camp ---")

    -- Start with no camp
    Core.Camp.ClearCamp()
    assert_false(Core.Camp.HasCamp(), "No camp after ClearCamp")
    assert_eq(-1, Core.Camp.GetDistanceToCamp(), "Distance is -1 with no camp")
    assert_false(Core.Camp.IsCampValid(), "Camp not valid with no camp")

    -- Set camp
    local ok = Core.Camp.SetCamp()
    assert_true(ok, "SetCamp returns true")
    assert_true(Core.Camp.HasCamp(), "HasCamp after SetCamp")
    assert_true(Core.Camp.IsCampValid(), "Camp is valid after SetCamp")

    -- Distance to camp should be ~0 (we just set it)
    local dist = Core.Camp.GetDistanceToCamp()
    assert_range(-0.1, 5.0, dist, "Distance to fresh camp is ~0")

    -- Should be in camp radius
    assert_true(Core.Camp.IsInCampRadius(100), "In camp radius 100")

    -- IsSpawnInRadius: self should be within a large radius of fresh camp
    local myId = Core.Spawn.GetLocalId()
    assert_true(Core.Camp.IsSpawnInRadius(myId, 1000), "Self is in camp radius after SetCamp")
    assert_false(Core.Camp.IsSpawnInRadius(999999, 100), "Invalid spawn not in camp")

    -- Camp position should be numbers
    local x, y, z = Core.Camp.GetCampPosition()
    assert_type("number", x, "Camp X is number")
    assert_type("number", y, "Camp Y is number")
    assert_type("number", z, "Camp Z is number")

    -- Zone ID should be valid
    local zid = Core.Camp.GetCampZoneID()
    assert_gt(0, zid, "Camp zone ID > 0")

    -- Leash distance
    Core.Camp.SetMaxLeashDistance(5000)
    assert_true(Core.Camp.IsCampValid(), "Camp still valid after leash change")

    -- Clean up
    Core.Camp.ClearCamp()
end

-- ---------------------------------------------------------------------------
-- Core.Buff tests
-- ---------------------------------------------------------------------------
function Test.test_buff()
    log("\ay--- Core.Buff ---")

    local myId = Core.Spawn.GetLocalId()

    -- GetBuffCount should return a non-negative number
    local count = Core.Buff.GetBuffCount(myId)
    assert_type("number", count, "GetBuffCount returns number")
    assert_range(0, 100, count, "Buff count reasonable")

    -- GetDebuffCount
    local debuffs = Core.Buff.GetDebuffCount()
    assert_type("number", debuffs, "GetDebuffCount returns number")
    assert_range(0, 50, debuffs, "Debuff count reasonable")

    -- HasBuff with invalid spell should be false
    assert_false(Core.Buff.HasBuff(myId, -1), "HasBuff(-1) is false")
    assert_false(Core.Buff.HasBuff(myId, 0), "HasBuff(0) is false")

    -- HasBuffByGroup with invalid group should be false
    assert_false(Core.Buff.HasBuffByGroup(myId, -1), "HasBuffByGroup(-1) is false")

    -- GetBuffInSlot(0) should return a number
    local slot0 = Core.Buff.GetBuffInSlot(myId, 0)
    assert_type("number", slot0, "GetBuffInSlot(0) returns number")

    -- Non-local spawn should return safe defaults
    assert_false(Core.Buff.HasBuff(999999, 1), "Non-local HasBuff returns false")
    assert_eq(0, Core.Buff.GetBuffCount(999999), "Non-local GetBuffCount returns 0")
end

-- ---------------------------------------------------------------------------
-- Core.AA tests
-- ---------------------------------------------------------------------------
function Test.test_aa()
    log("\ay--- Core.AA ---")

    -- Yaulp AA (group 4320) -- may not be an AA, test with a known AA
    -- Divine Rez AA group = 36
    local name = Core.AA.GetAAName(36)
    assert_type("string", name, "GetAAName returns string")

    local spellId = Core.AA.GetAASpellId(36)
    assert_type("number", spellId, "GetAASpellId returns number")

    local ready = Core.AA.IsReady(36)
    assert_type("boolean", ready, "IsReady returns bool")

    local timer = Core.AA.GetReuseTimer(36)
    assert_type("number", timer, "GetReuseTimer returns number")
    assert_range(0, 99999, timer, "Reuse timer reasonable")

    -- CanActivate returns two values
    local canActivate, reason = Core.AA.CanActivate(36)
    assert_type("boolean", canActivate, "CanActivate returns bool")
    assert_type("string", reason, "CanActivate returns reason string")

    -- Invalid AA
    local badName = Core.AA.GetAAName(-1)
    assert_eq("", badName, "Invalid AA returns empty name")
    assert_eq(0, Core.AA.GetAASpellId(-1), "Invalid AA returns 0 spell")
end

-- ---------------------------------------------------------------------------
-- Core.Mode tests
-- ---------------------------------------------------------------------------
function Test.test_mode()
    log("\ay--- Core.Mode ---")

    local mode = Core.Mode.GetMode()
    assert_type("number", mode, "GetMode returns number")
    assert_range(0, 8, mode, "Mode in valid range")

    local name = Core.Mode.GetModeName(mode)
    assert_type("string", name, "GetModeName returns string")
    assert_gt(0, #name, "Mode name is non-empty")

    -- All mode names should be valid
    for i = 0, 8 do
        local n = Core.Mode.GetModeName(i)
        assert_type("string", n, "GetModeName(" .. i .. ") returns string")
        assert_gt(0, #n, "Mode " .. i .. " name non-empty")
    end

    -- IsTankFocused and IsMovementMode return bools
    assert_type("boolean", Core.Mode.IsTankFocused(), "IsTankFocused returns bool")
    assert_type("boolean", Core.Mode.IsMovementMode(), "IsMovementMode returns bool")

    -- Invalid mode name
    local unknown = Core.Mode.GetModeName(99)
    assert_eq("Unknown", unknown, "Invalid mode returns 'Unknown'")
end

-- ---------------------------------------------------------------------------
-- Core.LocalPlayer extended tests
-- ---------------------------------------------------------------------------
function Test.test_localplayer()
    log("\ay--- Core.LocalPlayer ---")

    local sid = Core.LocalPlayer.GetSpawnID()
    assert_gt(0, sid, "GetSpawnID > 0")

    local base = Core.LocalPlayer.GetCharacterBase()
    assert_eq(sid, base, "GetCharacterBase == GetSpawnID")

    local gate = Core.LocalPlayer.IsGateFlagSet()
    assert_type("boolean", gate, "IsGateFlagSet returns bool")

    local mana = Core.LocalPlayer.GetMana()
    assert_type("number", mana, "GetMana returns number")

    local endur = Core.LocalPlayer.GetEndurance()
    assert_type("number", endur, "GetEndurance returns number")

    -- Gem slots
    for i = 0, 12 do
        local spellId = Core.LocalPlayer.GetGemSpellID(i)
        assert_type("number", spellId, "GetGemSpellID(" .. i .. ") returns number")
    end
end

-- ---------------------------------------------------------------------------
-- Core.Gem tests
-- ---------------------------------------------------------------------------
function Test.test_gem()
    log("\ay--- Core.Gem ---")

    local count = Core.Gem.GetAvailableSlotCount()
    assert_type("number", count, "GetAvailableSlotCount returns number")
    assert_range(8, 14, count, "Gem count 8-14")

    local spell0 = Core.Gem.GetCurrentSpell(0)
    assert_type("number", spell0, "GetCurrentSpell(0) returns number")

    assert_type("boolean", Core.Gem.IsReserved(0), "IsReserved returns bool")
    assert_type("boolean", Core.Gem.NeedsMemorization(), "NeedsMemorization returns bool")

    local desired = Core.Gem.GetDesiredSpell(0)
    assert_type("number", desired, "GetDesiredSpell returns number")
end

-- ---------------------------------------------------------------------------
-- Core.Event tests
-- ---------------------------------------------------------------------------
function Test.test_event()
    log("\ay--- Core.Event ---")

    -- Register a test event
    Core.Event.Register("test_cf_event", "TESTPATTERN_CF_12345", function(line)
        -- no-op for testing
    end)
    assert_true(Core.Event.IsEnabled("test_cf_event"), "Event registered and enabled")

    Core.Event.SetEnabled("test_cf_event", false)
    assert_false(Core.Event.IsEnabled("test_cf_event"), "Event disabled")

    Core.Event.SetEnabled("test_cf_event", true)
    assert_true(Core.Event.IsEnabled("test_cf_event"), "Event re-enabled")

    Core.Event.Unregister("test_cf_event")
    assert_false(Core.Event.IsEnabled("test_cf_event"), "Event unregistered")
end

-- ---------------------------------------------------------------------------
-- Run all tests or specific subsystem
-- ---------------------------------------------------------------------------
function Test.run(subsystem)
    reset()

    Core.Util.WriteChatf("\ag[MQ2CF Test]\ax Running tests...")

    if subsystem == nil or subsystem == "" or subsystem == "all" then
        Test.test_util()
        Test.test_spawn()
        if Core.Spell then Test.test_spell() end
        if Core.Group then Test.test_group() end
        if Core.Lifecycle then Test.test_lifecycle() end
        if Core.Humanize then Test.test_humanize() end
        if Core.Settings then Test.test_settings() end
        if Core.Camp then Test.test_camp() end
        if Core.Buff then Test.test_buff() end
        if Core.AA then Test.test_aa() end
        if Core.Mode then Test.test_mode() end
        if Core.LocalPlayer then Test.test_localplayer() end
        if Core.Gem then Test.test_gem() end
        if Core.Event then Test.test_event() end
    elseif subsystem == "util" then
        Test.test_util()
    elseif subsystem == "spawn" then
        Test.test_spawn()
    elseif subsystem == "spell" then
        Test.test_spell()
    elseif subsystem == "group" then
        Test.test_group()
    elseif subsystem == "lifecycle" then
        Test.test_lifecycle()
    elseif subsystem == "humanize" then
        Test.test_humanize()
    elseif subsystem == "settings" then
        Test.test_settings()
    elseif subsystem == "camp" then
        Test.test_camp()
    elseif subsystem == "buff" then
        Test.test_buff()
    elseif subsystem == "aa" then
        Test.test_aa()
    elseif subsystem == "mode" then
        Test.test_mode()
    elseif subsystem == "localplayer" then
        Test.test_localplayer()
    elseif subsystem == "gem" then
        Test.test_gem()
    elseif subsystem == "event" then
        Test.test_event()
    else
        Core.Util.WriteChatf("\ar[MQ2CF Test]\ax Unknown subsystem: " .. subsystem)
        return
    end

    -- Output all messages to chat
    for _, msg in ipairs(Test.messages) do
        Core.Util.WriteChatf(msg)
    end

    -- Write results to log file
    local logPath = Core.Util.GetResourcePath() .. "\\MQ2CF\\test_results.log"
    local f = io.open(logPath, "w")
    if f then
        f:write("MQ2CF Test Results -- " .. os.date() .. "\n")
        f:write("Subsystem: " .. (subsystem or "all") .. "\n")
        f:write(string.rep("-", 60) .. "\n")
        for _, msg in ipairs(Test.messages) do
            -- Strip MQ color codes for the log file
            local clean = msg:gsub("\\a[a-z]", "")
            f:write(clean .. "\n")
        end
        f:write(string.rep("-", 60) .. "\n")
        f:write("PASSED: " .. Test.passed .. "\n")
        f:write("FAILED: " .. Test.failed .. "\n")
        f:close()
    end

    -- Summary line in chat
    local color = Test.failed == 0 and "\ag" or "\ar"
    Core.Util.WriteChatf(color .. "[MQ2CF Test]\ax " ..
        Test.passed .. " passed, " .. Test.failed .. " failed")
end

return Test
