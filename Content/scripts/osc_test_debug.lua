-- OSC Receiver Test Script with Debug Messages
-- This script tests OSC message reception on port 7000

backlog_post("===========================================")
backlog_post("  OSC Receiver Test - DEBUG VERSION")
backlog_post("===========================================")
backlog_post("[DEBUG 1] Creating OSCReceiver object...")

-- Create OSC receiver
receiver = OSCReceiver()
backlog_post("[DEBUG 2] OSCReceiver created successfully")
backlog_post("[DEBUG 3] Calling Initialize(7000)...")

if not receiver:Initialize(7000) then
    backlog_post("[ERROR] Failed to initialize OSC receiver on port 7000!")
    return
end

backlog_post("[DEBUG 4] OSC receiver initialized successfully")
backlog_post("[DEBUG 5] Setting up callbacks for /ch/1 through /ch/8...")

-- Set up callbacks for channels /ch/1 through /ch/8
for i = 1, 8 do
    local channel = "/ch/" .. i
    backlog_post("[DEBUG] Setting callback " .. i .. " for " .. channel)

    receiver:SetCallback(channel, function(msg)
        backlog_post("[CALLBACK] Entered callback for " .. channel)
        local addr = msg:GetAddress()
        backlog_post("[CALLBACK] Got address: " .. addr)

        if msg:GetFloatCount() > 0 then
            backlog_post("[CALLBACK] Has float data, getting value...")
            local value = msg:GetFloat(0)
            backlog_post(string.format("[OSC] %s = %.3f", addr, value))
        else
            backlog_post("[CALLBACK] No float data in message")
        end
    end)

    backlog_post("[DEBUG] Callback " .. i .. " registered")
end

backlog_post("[DEBUG 6] All callbacks registered successfully")
backlog_post("===========================================")
backlog_post("[READY] Waiting for OSC messages on 127.0.0.1:7000")
backlog_post("[DEBUG 7] Starting background update loop...")

-- Create a background process to update the receiver
local frame_count = 0
runProcess(function()
    backlog_post("[DEBUG 8] Background process started")
    while true do
        frame_count = frame_count + 1

        if frame_count % 120 == 0 then
            backlog_post("[DEBUG] Update loop alive - frame " .. frame_count)
        end

        if frame_count == 1 then
            backlog_post("[DEBUG 9] Calling receiver:Update() for first time...")
        end

        receiver:Update()

        if frame_count == 1 then
            backlog_post("[DEBUG 10] First Update() completed successfully")
        end

        update()
    end
end)

backlog_post("[DEBUG] Script initialization complete!")
