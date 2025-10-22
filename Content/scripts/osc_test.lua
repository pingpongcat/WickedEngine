-- OSC Receiver Test Script for WickedEngine
-- This script demonstrates configurable OSC message reception
-- You can customize: port, IP address, and channel path template

backlog_post("===========================================")
backlog_post("  OSC Receiver Test - Channel Monitor")
backlog_post("===========================================")

-- ============================================
-- CONFIGURATION - Edit these values as needed
-- ============================================
local LISTEN_PORT = 7000                -- Port to listen on
local BIND_IP = {0, 0, 0, 0}            -- IP to bind to: {0,0,0,0} = all interfaces, {127,0,0,1} = localhost only
local CHANNEL_PATH_TEMPLATE = "/ch/%d"  -- Channel path template (e.g., "/ch/%d", "/fader/%d", "/slider/%d")
local NUM_CHANNELS = 8                  -- Number of channels to monitor (fixed at 8 for now)

backlog_post(string.format("Configuration:"))
backlog_post(string.format("  Listen Port: %d", LISTEN_PORT))
backlog_post(string.format("  Bind IP: %d.%d.%d.%d", BIND_IP[1], BIND_IP[2], BIND_IP[3], BIND_IP[4]))
backlog_post(string.format("  Channel Path Template: %s", CHANNEL_PATH_TEMPLATE))
backlog_post(string.format("  Number of Channels: %d", NUM_CHANNELS))
backlog_post("===========================================")

-- Create OSC receiver
receiver = OSCReceiver()
if not receiver:Initialize(LISTEN_PORT, BIND_IP[1], BIND_IP[2], BIND_IP[3], BIND_IP[4]) then
    backlog_post(string.format("[ERROR] Failed to initialize OSC receiver on port %d!", LISTEN_PORT))
    return
end

-- Set custom channel path template
receiver:SetChannelPath(CHANNEL_PATH_TEMPLATE)

backlog_post("OSC receiver initialized successfully")
backlog_post(string.format("Monitoring channels: %s through %s",
    receiver:GetChannelPath(1),
    receiver:GetChannelPath(NUM_CHANNELS)))
backlog_post("===========================================")
backlog_post("")

-- Table to store latest OSC values
osc_values = {}
for i = 1, NUM_CHANNELS do
    osc_values[i] = 0.0
end

-- Message counter
message_count = 0

-- Set up callbacks for channels 1 through NUM_CHANNELS
for i = 1, NUM_CHANNELS do
    local channel = receiver:GetChannelPath(i)
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

backlog_post(string.format("Callbacks registered for channels 1-%d", NUM_CHANNELS))
backlog_post("Waiting for OSC messages...")
backlog_post("")

-- Create a background process to update the receiver
runProcess(function()
    while true do
        receiver:Update()
        update()
    end
end)

backlog_post(string.format("[INFO] OSC test running - send messages to port %d", LISTEN_PORT))
backlog_post(string.format("[INFO] Example addresses: %s, %s, ...",
    receiver:GetChannelPath(1),
    receiver:GetChannelPath(2)))
