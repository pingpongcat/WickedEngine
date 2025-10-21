# Example_OSC - Music Visualization

This example demonstrates real-time OSC (Open Sound Control) message reception and music visualization in WickedEngine.

## What It Does

The application creates 8 colored point lights arranged in a row. Each light is controlled by an OSC channel (/ch/1 through /ch/8). When OSC messages are received, the light intensity responds in real-time.

## Features

- **OSC Receiver**: Listens on UDP port 7000
- **8 Channels**: Monitors `/ch/1` through `/ch/8`
- **Real-time Response**: Light intensities update based on OSC float values
- **Smooth Animation**: Interpolated light transitions for smooth visuals
- **Color-coded Lights**: Each channel has a unique color (Red, Orange, Yellow, Green, Cyan, Blue, Purple, Magenta)

## Building

### Linux

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make Example_OSC -j
```

### Running

```bash
cd build/Samples/Example_OSC
./Example_OSC
```

## OSC Configuration

**Receiver Settings**:
- IP: 127.0.0.1 (localhost)
- Port: 7000
- Channels: /ch/1, /ch/2, /ch/3, /ch/4, /ch/5, /ch/6, /ch/7, /ch/8
- Expected message format: Float value (0.0 - 1.0)

**Transmitter Configuration** (Your OSC Software):
- Target IP: 127.0.0.1
- Target Port (Out Port): 7000
- Send messages on addresses: /ch/1 through /ch/8
- Value range: 0.0 (off) to 1.0 (max brightness)

## Example OSC Software

Compatible with any OSC transmitter, such as:
- **TouchOSC**: Mobile OSC controller
- **Pure Data**: Visual programming for audio
- **Max/MSP**: Audio/visual programming environment
- **SuperCollider**: Audio synthesis platform
- **Custom applications**: Using libraries like liblo, oscpack, or tinyosc

## Scene Layout

```
        Light1  Light2  Light3  Light4  Light5  Light6  Light7  Light8
        (Red)  (Orange)(Yellow)(Green) (Cyan)  (Blue) (Purple)(Magenta)
          ●      ●       ●       ●       ●       ●       ●       ●  <- Point lights at Y=2.0
          │      │       │       │       │       │       │       │
          ◯      ◯       ◯       ◯       ◯       ◯       ◯       ◯  <- White spheres at Y=0.5
        ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  <- Gray ground plane

        Lights: Position (-14,2,0) to (14,2,0), Range: 20 units
        Spheres: Position (-14,0.5,0) to (14,0.5,0), Radius: 0.5
        Ground: 50x50 units, Gray (0.3, 0.3, 0.3)
        Camera: (0, 5, -20) looking at origin
```

## Controls

- **ESC**: Exit application
- **WASD**: Move camera
- **Mouse**: Look around (hold right button)
- **HOME**: Open backlog console

## Troubleshooting

### No lights visible?
- Check that your OSC transmitter is sending to 127.0.0.1:7000
- Send test messages with values between 0.0 and 1.0
- Open backlog (HOME key) to see OSC initialization messages

### OSC receiver fails to initialize?
- Port 7000 might be in use by another application
- Check firewall settings
- Ensure you have permission to bind to the port

### Lights don't respond to OSC?
- Verify OSC address format: `/ch/1` (not `/ch1` or `ch/1`)
- Check OSC message contains float argument
- Use backlog to verify messages are being received

## C++ Code Overview

```cpp
// OSC Receiver
wi::osc::OSCReceiver receiver;
receiver.Initialize(7000);

// Set callback for channel
receiver.SetCallback("/ch/1", [](const wi::osc::OSCMessage& msg) {
    float value = msg.GetFloat(0);
    // Update light intensity
});

// In update loop
receiver.Update();  // Polls socket and invokes callbacks
```

## Lua Script Alternative

You can also use Lua scripts for OSC visualization:

```lua
-- In WickedEngine Editor, press HOME and run:
dofile("scripts/osc_music_viz.lua")
```

See `Content/scripts/osc_music_viz.lua` for the Lua implementation.

## Extending This Example

**Ideas for customization**:
- Map OSC to material colors instead of lights
- Control particle emitters with OSC
- Drive object transformations (rotation, scale, position)
- Trigger animations based on OSC thresholds
- Create frequency analyzer visualizations
- Add more channels for complex scenes
- Implement OSC transmitter to send engine state back

## License

Same as WickedEngine - ISC License

## See Also

- [OSC Specification](http://opensoundcontrol.org/)
- [WickedEngine Documentation](../../Content/Documentation/WickedEngine-Documentation.md)
- [Lua OSC API](../../Content/Documentation/ScriptingAPI-Documentation.md#osc)
- [tinyosc Library](https://github.com/mhroth/tinyosc)
