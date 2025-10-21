-- Minimal OSC Receiver Test
-- Testing Lua bindings with maximum debug output

backlog_post("==========================================")
backlog_post("  MINIMAL OSC TEST - Starting...")
backlog_post("==========================================")

backlog_post("[TEST 1] Creating OSCReceiver...")
local receiver = OSCReceiver()
backlog_post("[TEST 2] OSCReceiver object created")

backlog_post("[TEST 3] Calling Initialize(7000)...")
if not receiver:Initialize(7000) then
    backlog_post("[ERROR] Failed to initialize!")
    return
end
backlog_post("[TEST 4] Initialize returned successfully")

backlog_post("[TEST 5] Setting callback for /ch/1...")
receiver:SetCallback("/ch/1", function(msg)
    backlog_post("[LUA CALLBACK] === CALLBACK TRIGGERED ===")
    local addr = msg:GetAddress()
    backlog_post("[LUA CALLBACK] Address: " .. addr)

    local floatCount = msg:GetFloatCount()
    backlog_post("[LUA CALLBACK] Float count: " .. floatCount)

    if floatCount > 0 then
        local value = msg:GetFloat(0)
        backlog_post(string.format("[LUA CALLBACK] /ch/1 = %.3f", value))
    end
    backlog_post("[LUA CALLBACK] === CALLBACK FINISHED ===")
end)
backlog_post("[TEST 6] Callback registered")

backlog_post("==========================================")
backlog_post("[READY] Listening on 127.0.0.1:7000")
backlog_post("[INFO] Call receiver:Update() to poll messages")
backlog_post("==========================================")

-- Simple update loop
local frame = 0
runProcess(function()
    while true do
        frame = frame + 1

        if frame == 1 then
            backlog_post("[TEST 7] First Update() call...")
        end

        receiver:Update()

        if frame == 1 then
            backlog_post("[TEST 8] First Update() returned")
        end

        if frame % 120 == 0 then
            backlog_post("[HEARTBEAT] Frame " .. frame .. " - loop alive")
        end

        update()
    end
end)

backlog_post("[TEST 9] Script finished, background process running")
