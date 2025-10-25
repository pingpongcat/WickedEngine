-- This script will load a teapot model and control the lid position via OSC /ch/1 messages
killProcesses()

backlog_post("---> START SCRIPT: osc_test.lua")

scene = GetScene()
scene.Clear()
LoadModel(script_dir() .. "../models/teapot.wiscene")
top_entity = scene.Entity_FindByName("Top")
transform_component = scene.Component_GetTransform(top_entity)
rest_matrix = transform_component.GetMatrix()

receiver = OSCReceiver()
receiver:Initialize(7000)

osc_value = 0.0
current_value = 0.0
smoothing = 0.05  -- Smoothing factor (0 = instant, 1 = very smooth)

receiver:SetCallback("/ch/1", function(msg)
    if msg:GetFloatCount() > 0 then
        osc_value = msg:GetFloat(0)
    end
end)

runProcess(function()
    while true do
        receiver:Update()
        -- Smooth interpolation
        current_value = current_value + (osc_value - current_value) * smoothing
        transform_component.ClearTransform()
        transform_component.MatrixTransform(rest_matrix)
        transform_component.Translate(Vector(0, current_value, 0))
        update()
    end
end)

backlog_post("---> END SCRIPT: osc_test.lua")
