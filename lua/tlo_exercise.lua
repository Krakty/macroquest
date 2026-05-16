local mq = require('mq')
local logfile = io.open("C:\\MacroQuest\\Logs\\tlo_exercise.log", "w")
local pass, fail, crash = 0, 0, 0

local function log(msg) logfile:write(msg .. "\n"); logfile:flush() end
local function test(label, fn, validator)
    local ok, val = pcall(fn)
    if not ok then log("  CRASH " .. label .. " => " .. tostring(val)); crash = crash + 1; return end
    if validator then
        local vok, reason = validator(val)
        if vok then log("  PASS  " .. label .. " = " .. tostring(val)); pass = pass + 1
        else log("  FAIL  " .. label .. " = " .. tostring(val) .. " (" .. (reason or "failed") .. ")"); fail = fail + 1 end
    else
        if val ~= nil then log("  PASS  " .. label .. " = " .. tostring(val)); pass = pass + 1
        else log("  FAIL  " .. label .. " = nil"); fail = fail + 1 end
    end
end

local function isNum(v) return tonumber(v) ~= nil, "not number" end
local function isPos(v) local n = tonumber(v); return n ~= nil and n > 0, "not positive" end
local function isBool(v) return v == true or v == false, "not bool" end
local function isStr(v) return type(v) == "string" and #v > 0, "empty" end

log("=== TLO EXERCISE === " .. os.date())

log("\n--- Character ---")
for _, p in ipairs({"Name","Level","Class","Race","CurrentHPs","MaxHPs","PctHPs","CurrentMana","MaxMana","Endurance","MaxEndurance","STR","STA","DEX","AGI","INT","WIS","CHA","svMagic","svFire","svCold","svPoison","svDisease","AAPoints","PctExp","Platinum","Gold","Silver","Copper","Combat","Moving","Grouped","MaxBuffSlots","FreeInventory","Deity","Guild","Hunger","Thirst"}) do
    test("Me." .. p, function() return mq.TLO.Me[p]() end)
end

log("\n--- Spawn ---")
for _, p in ipairs({"ID","Name","Surname","Level","X","Y","Z","Heading","Speed","Height","Animation","Standing","Sitting","AFK","LFG","Linkdead","GM","Sneaking","Levitating","Type","Body","Gender","State","MaxRange","Pet","Light"}) do
    test("Spawn." .. p, function() return mq.TLO.Me[p]() end)
end

log("\n--- Inventory ---")
for slot = 0, 22 do
    local ok, val = pcall(function() return mq.TLO.Me.Inventory(slot)() end)
    if ok and val ~= nil then
        for _, f in ipairs({"Name","ID","Value","Weight","Icon","Stack","NoDrop","Lore"}) do
            test("Inv(" .. slot .. ")." .. f, function() return mq.TLO.Me.Inventory(slot)[f]() end)
        end
        break
    end
end

log("\n--- Buffs ---")
for i = 1, 5 do
    local ok, val = pcall(function() return mq.TLO.Me.Buff(i)() end)
    if ok and val ~= nil and tostring(val) ~= "" then
        for _, f in ipairs({"Name","ID","Duration","Level"}) do
            test("Buff(" .. i .. ")." .. f, function() return mq.TLO.Me.Buff(i)[f]() end)
        end
        break
    end
end

log("\n--- Target ---")
if pcall(function() return mq.TLO.Target.ID() end) and mq.TLO.Target.ID() then
    for _, f in ipairs({"ID","Name","Level","Class","Type","Distance","PctHPs"}) do
        test("Target." .. f, function() return mq.TLO.Target[f]() end)
    end
else log("  SKIP  No target") end

log("\n--- Spawns ---")
test("SpawnCount", function() return mq.TLO.SpawnCount() end, isPos)
for i = 1, math.min(10, tonumber(mq.TLO.SpawnCount()) or 0) do
    local s = mq.TLO.NearestSpawn(i)
    test("Spawn(" .. i .. ").Name", function() return s.Name() end)
    test("Spawn(" .. i .. ").ID", function() return s.ID() end, isPos)
end

log("\n--- Windows ---")
for _, w in ipairs({"PlayerWindow","TargetWindow","BuffWindow","GroupWindow","HotButtonWnd","InventoryWindow"}) do
    for _, f in ipairs({"Open","Enabled","Style","X","Y","Width","Height","Tooltip","Name"}) do
        test(w .. "." .. f, function() return mq.TLO.Window(w)[f]() end)
    end
end

log("\n--- Zone/EQ ---")
test("Zone.Name", function() return mq.TLO.Zone.Name() end, isStr)
test("Zone.ShortName", function() return mq.TLO.Zone.ShortName() end, isStr)
test("Zone.ID", function() return mq.TLO.Zone.ID() end, isPos)
test("EQ.GameState", function() return mq.TLO.EverQuest.GameState() end, isStr)
test("EQ.Server", function() return mq.TLO.EverQuest.Server() end, isStr)
test("EQ.Ping", function() return mq.TLO.EverQuest.Ping() end, isNum)

log(string.format("\n=== SUMMARY: %d pass, %d fail, %d crash ===", pass, fail, crash))
if crash > 0 then log("*** CRASHES -- struct offsets wrong ***")
elseif fail > 0 then log("*** FAILURES -- some values unexpected ***")
else log("ALL TESTS PASSED") end
logfile:close()
print(string.format("TLO Exercise: %d pass, %d fail, %d crash. See Logs/tlo_exercise.log", pass, fail, crash))
