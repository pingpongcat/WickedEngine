-- OSC-driven Music Visualization for WickedEngine
-- Maps OSC channels /ch/1 through /ch/8 to colored point lights
-- Expects OSC transmitter on 127.0.0.1:7000

backlog_post("===========================================")
backlog_post("  OSC Music Visualization")
backlog_post("===========================================")

-- Get the scene
scene = GetScene()

-- Create 8 colored lights for 8 OSC channels
light_entities = {}
colors = {
    Vector(1, 0, 0),      -- Red
    Vector(1, 0.5, 0),    -- Orange
    Vector(1, 1, 0),      -- Yellow
    Vector(0, 1, 0),      -- Green
    Vector(0, 1, 1),      -- Cyan
    Vector(0, 0, 1),      -- Blue
    Vector(0.5, 0, 1),    -- Purple
    Vector(1, 0, 1),      -- Magenta
}

backlog_post("Creating 8 colored lights...")

for i = 1, 8 do
    local entity = CreateEntity()
    local transform = scene.Component_CreateTransform(entity)
    transform.Translate(Vector((i - 4.5) * 3, 0, 0))

    local light = scene.Component_CreateLight(entity)
    light.SetType(ENTITY_TYPE_POINTLIGHT)
    light.SetColor(colors[i])
    light.SetRange(15.0)
    light.SetIntensity(0.0)  -- Start at 0, will be controlled by OSC

    light_entities[i] = entity
    backlog_post(string.format("  Light %d: %s at x=%.1f", i,
        ({"Red","Orange","Yellow","Green","Cyan","Blue","Purple","Magenta"})[i],
        (i - 4.5) * 3))
end

-- Initialize OSC receiver
receiver = OSCReceiver()
if not receiver:Initialize(7000) then
    backlog_post("[ERROR] Failed to initialize OSC receiver!")
    return
end

backlog_post("OSC receiver initialized on port 7000")

-- Set up callbacks for each channel
for i = 1, 8 do
    local channel = "/ch/" .. i
    local entity = light_entities[i]

    receiver:SetCallback(channel, function(msg)
        if msg:GetFloatCount() > 0 then
            local value = msg:GetFloat(0)

            -- Get the light component and set intensity
            local light = scene.Component_GetLight(entity)
            light.SetIntensity(value * 100.0)  -- Scale OSC value (0-1) to intensity
        end
    end)
end

backlog_post("Callbacks registered for /ch/1 through /ch/8")
backlog_post("===========================================")
backlog_post("")
backlog_post("[READY] Send OSC messages to 127.0.0.1:7000")
backlog_post("        Light intensity = OSC value * 100")

-- Main update loop
runProcess(function()
    while true do
        receiver:Update()
        update()
    end
end)
