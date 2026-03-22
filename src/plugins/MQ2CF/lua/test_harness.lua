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
    elseif subsystem == "util" then
        Test.test_util()
    elseif subsystem == "spawn" then
        Test.test_spawn()
    elseif subsystem == "spell" then
        Test.test_spell()
    elseif subsystem == "group" then
        Test.test_group()
    else
        Core.Util.WriteChatf("\ar[MQ2CF Test]\ax Unknown subsystem: " .. subsystem)
        return
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

    -- Summary line in chat only
    local color = Test.failed == 0 and "\ag" or "\ar"
    Core.Util.WriteChatf(color .. "[MQ2CF Test]\ax " ..
        Test.passed .. " passed, " .. Test.failed .. " failed" ..
        " (details: " .. logPath .. ")")
end

return Test
