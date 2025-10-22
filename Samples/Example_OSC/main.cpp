#include "WickedEngine.h"
#include <SDL2/SDL.h>

// OSC Music Visualization Example for WickedEngine
// This example demonstrates OSC message reception and real-time scene manipulation
// Configure your OSC transmitter to send to the configured IP and port

// ============================================
// CONFIGURATION - Edit these values as needed
// ============================================
constexpr uint16_t OSC_LISTEN_PORT = 7000;
constexpr uint8_t OSC_BIND_IP[4] = {0 0, 0, 0}; // Bind IP: {0,0,0,0} = all interfaces, {127,0,0,1} = localhost
constexpr const char* OSC_CHANNEL_PATH_TEMPLATE = "/ch/%d";
constexpr int OSC_NUM_CHANNELS = 8;
constexpr float LIGHT_CIRCLE_RADIUS = 16.0f;     // Distance of lights from center
constexpr float LIGHT_HEIGHT = 3.0f;             // Height of lights above ground
constexpr bool ENABLE_TEAPOT_MOVEMENT = true;    // Enable OSC-based teapot movement

class OSC_MusicViz : public wi::RenderPath3D
{
private:
	wi::osc::OSCReceiver receiver;
	wi::ecs::Entity light_entities[8];
	float light_intensities[8] = {0};
	wi::ecs::Entity teapot_entity = wi::ecs::INVALID_ENTITY;

	// Colors for each channel
	XMFLOAT3 colors[8] = {
		XMFLOAT3(1.0f, 0.0f, 0.0f),   // Red
		XMFLOAT3(1.0f, 0.5f, 0.0f),   // Orange
		XMFLOAT3(1.0f, 1.0f, 0.0f),   // Yellow
		XMFLOAT3(0.0f, 1.0f, 0.0f),   // Green
		XMFLOAT3(0.0f, 1.0f, 1.0f),   // Cyan
		XMFLOAT3(0.0f, 0.0f, 1.0f),   // Blue
		XMFLOAT3(0.5f, 0.0f, 1.0f),   // Purple
		XMFLOAT3(1.0f, 0.0f, 1.0f),   // Magenta
	};

public:
	void Load() override
	{
		wi::RenderPath3D::Load();

		// Initialize OSC receiver with configured port and IP
		if (!receiver.Initialize(OSC_LISTEN_PORT, OSC_BIND_IP[0], OSC_BIND_IP[1], OSC_BIND_IP[2], OSC_BIND_IP[3]))
		{
			wi::backlog::post("Failed to initialize OSC receiver on port " +
				std::to_string(OSC_LISTEN_PORT) + "!", wi::backlog::LogLevel::Error);
			return;
		}

		// Set custom channel path template
		receiver.SetChannelPath(OSC_CHANNEL_PATH_TEMPLATE);

		wi::backlog::post("==========================================");
		wi::backlog::post("  OSC Music Visualization - Configuration");
		wi::backlog::post("==========================================");
		wi::backlog::post("Listen Port: " + std::to_string(OSC_LISTEN_PORT));
		wi::backlog::post("Bind IP: " +
			std::to_string(OSC_BIND_IP[0]) + "." +
			std::to_string(OSC_BIND_IP[1]) + "." +
			std::to_string(OSC_BIND_IP[2]) + "." +
			std::to_string(OSC_BIND_IP[3]));
		wi::backlog::post("Channel Path Template: " + std::string(OSC_CHANNEL_PATH_TEMPLATE));
		wi::backlog::post("Number of Channels: " + std::to_string(OSC_NUM_CHANNELS));
		wi::backlog::post("Light Circle Radius: " + std::to_string(LIGHT_CIRCLE_RADIUS));
		wi::backlog::post("Teapot Movement: " + std::string(ENABLE_TEAPOT_MOVEMENT ? "Enabled" : "Disabled"));
		wi::backlog::post("==========================================");

		// Get scene reference
		wi::scene::Scene& scene = wi::scene::GetScene();

		// Create 8 point lights arranged in a circle around the center
		const float PI = 3.14159265358979323846f;
		for (int i = 0; i < OSC_NUM_CHANNELS; i++)
		{
			light_entities[i] = wi::ecs::CreateEntity();

			// Calculate position in circle
			// Distribute lights evenly around 360 degrees
			float angle = (2.0f * PI * i) / OSC_NUM_CHANNELS;
			float x = LIGHT_CIRCLE_RADIUS * cosf(angle);
			float z = LIGHT_CIRCLE_RADIUS * sinf(angle);

			// Create transform component
			wi::scene::TransformComponent& transform = scene.transforms.Create(light_entities[i]);
			transform.Translate(XMFLOAT3(x, LIGHT_HEIGHT, z));
			transform.UpdateTransform();

			// Create light component
			wi::scene::LightComponent& light = scene.lights.Create(light_entities[i]);
			light.type = wi::scene::LightComponent::POINT;
			light.color = colors[i];
			light.range = 20.0f;
			light.intensity = 0.0f;  // Start at 0, controlled by OSC
			light.SetCastShadow(false);

			// Set up OSC callback for this channel using configurable path
			std::string channel = receiver.GetChannelPath(i + 1);
			int channel_index = i;  // Capture by value

			receiver.SetCallback(channel, [this, channel_index](const wi::osc::OSCMessage& msg) {
				if (msg.GetFloatCount() > 0)
				{
					float value = msg.GetFloat(0);
					light_intensities[channel_index] = value * 100.0f;  // Scale to intensity
				}
			});

			wi::backlog::post("Light " + std::to_string(i + 1) + " (" + channel + ") at position (" +
				std::to_string(x) + ", " + std::to_string(LIGHT_HEIGHT) + ", " + std::to_string(z) + ")");
		}

		// Load teapot model as a visible reference
		wi::scene::LoadModel("../../Content/models/teapot.wiscene");

		// Find the teapot entity by name (it's typically the root of the loaded scene)
		// We'll search for an entity with a transform at origin or first object entity
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			wi::ecs::Entity entity = scene.transforms.GetEntity(i);
			// Store first valid entity we find (likely the teapot)
			if (teapot_entity == wi::ecs::INVALID_ENTITY)
			{
				teapot_entity = entity;
				wi::backlog::post("Teapot entity found: " + std::to_string(entity));
				break;
			}
		}

		wi::backlog::post("Teapot model loaded - you should see it in the center");

		if (ENABLE_TEAPOT_MOVEMENT && teapot_entity != wi::ecs::INVALID_ENTITY)
		{
			wi::backlog::post("Teapot movement enabled:");
			wi::backlog::post("  " + receiver.GetChannelPath(1) + " -> X position");
			wi::backlog::post("  " + receiver.GetChannelPath(2) + " -> Y position");
			wi::backlog::post("  " + receiver.GetChannelPath(3) + " -> Z position");
			wi::backlog::post("  " + receiver.GetChannelPath(4) + " -> Y rotation");
		}

		// Set camera position
		wi::scene::CameraComponent& camera = wi::scene::GetCamera();
		wi::scene::TransformComponent cam_transform;
		cam_transform.Translate(XMFLOAT3(0, 5, -20));
		cam_transform.UpdateTransform();
		camera.TransformCamera(cam_transform);
		camera.UpdateCamera();

		setFXAAEnabled(true);
		setAO(AO_DISABLED);

		wi::backlog::post("==========================================");
		wi::backlog::post("OSC receiver ready - waiting for messages");
		wi::backlog::post("==========================================");
	}

	void Update(float dt) override
	{
		// Update OSC receiver (polls socket and invokes callbacks)
		receiver.Update();

		// Get scene reference
		wi::scene::Scene& scene = wi::scene::GetScene();

		// Apply intensity values to lights (smooth interpolation)
		for (int i = 0; i < OSC_NUM_CHANNELS; i++)
		{
			wi::scene::LightComponent* light = scene.lights.GetComponent(light_entities[i]);
			if (light)
			{
				// Smooth fade
				light->intensity = wi::math::Lerp(light->intensity, light_intensities[i], dt * 10.0f);
			}
		}

		// Apply OSC values to teapot movement if enabled
		if (ENABLE_TEAPOT_MOVEMENT && teapot_entity != wi::ecs::INVALID_ENTITY)
		{
			wi::scene::TransformComponent* transform = scene.transforms.GetComponent(teapot_entity);
			if (transform)
			{
				// Map OSC values (0-1) to movement ranges
				// Channel 1: X position (-10 to +10)
				// Channel 2: Y position (0 to 5)
				// Channel 3: Z position (-10 to +10)
				// Channel 4: Y rotation (0 to 360 degrees)

				float x_pos = (light_intensities[0] / 100.0f) * 20.0f - 10.0f;  // -10 to +10
				float y_pos = (light_intensities[1] / 100.0f) * 5.0f;           // 0 to 5
				float z_pos = (light_intensities[2] / 100.0f) * 20.0f - 10.0f;  // -10 to +10
				float y_rot = (light_intensities[3] / 100.0f) * 6.28318530718f; // 0 to 2*PI radians

				// Apply position
				XMFLOAT3 current_pos = transform->GetPosition();
				XMFLOAT3 target_pos = XMFLOAT3(x_pos, y_pos, z_pos);

				// Smooth interpolation for position
				transform->ClearTransform();
				transform->Translate(XMFLOAT3(
					wi::math::Lerp(current_pos.x, target_pos.x, dt * 5.0f),
					wi::math::Lerp(current_pos.y, target_pos.y, dt * 5.0f),
					wi::math::Lerp(current_pos.z, target_pos.z, dt * 5.0f)
				));

				// Apply rotation around Y axis
				transform->RotateRollPitchYaw(XMFLOAT3(0, y_rot, 0));
				transform->UpdateTransform();
			}
		}

		wi::RenderPath3D::Update(dt);
	}

	void Stop() override
	{
		receiver.Shutdown();
		wi::RenderPath3D::Stop();
	}
};

wi::Application application;

int main(int argc, char *argv[])
{
	// SDL window setup:
	sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
	sdl2::window_ptr_t window = sdl2::make_window(
		"WickedEngine - OSC Music Visualization",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_Event event;

	// Set SDL window to engine:
	application.SetWindow(window.get());

	// Process command line string:
	wi::arguments::Parse(argc, argv);

	// Create and activate our OSC visualization render path
	OSC_MusicViz vizPath;
	vizPath.Load();
	application.ActivatePath(&vizPath);

	// Show info display
	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.resolution = true;
	application.infoDisplay.fpsinfo = true;

	bool quit = false;
	while (!quit)
	{
		SDL_PumpEvents();
		application.Run();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					quit = true;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					application.SetWindow(application.window);
					break;
				default:
					break;
				}
			default:
				break;
			}
			wi::input::sdlinput::ProcessEvent(event);
		}
	}

	wi::jobsystem::ShutDown(); // waits for jobs to finish before shutdown

	SDL_Quit();

	return 0;
}
