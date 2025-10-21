-- OSC Receiver Test Script for WickedEngine
-- This script tests OSC message reception on port 7000
-- Compatible with transmitter on 127.0.0.1 out port: 7000, in port: 7001

backlog_post("===========================================")
backlog_post("  OSC Receiver Test - Channel Monitor")
backlog_post("===========================================")
backlog_post("Starting OSC receiver on port 7000...")

-- Create OSC receiver
receiver = OSCReceiver()
if not receiver:Initialize(7000) then
    backlog_post("[ERROR] Failed to initialize OSC receiver on port 7000!")
    return
end

backlog_post("OSC receiver initialized successfully")
backlog_post("Monitoring channels: /ch/1 through /ch/8")
backlog_post("===========================================")
backlog_post("")

-- Table to store latest OSC values
osc_values = {}
for i = 1, 8 do
    osc_values[i] = 0.0
end

-- Message counter
message_count = 0

-- Set up callbacks for channels /ch/1 through /ch/8
for i = 1, 8 do
    local channel = "/ch/" .. i
    receiver:SetCallback(channel, function(msg)
        local addr = msg:GetAddress()
        if msg:GetFloatCount() > 0 then
            local value = msg:GetFloat(0)
            osc_values[i] = value
            message_count = message_count + 1
            backlog_post(string.format("[OSC] %s = %.3f (#%d)", addr, value, message_count))
        else
            backlog_post(string.format("[OSC] %s (no float data)", addr))
        end
    end)
end

backlog_post("Callbacks registered for /ch/1 through /ch/8")
backlog_post("Waiting for OSC messages...")
backlog_post("")

-- Create a background process to update the receiver
runProcess(function()
    while true do
        receiver:Update()
        update()
    end
end)

backlog_post("[INFO] OSC test running - send messages to 127.0.0.1:7000")
