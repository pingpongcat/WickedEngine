#include "WickedEngine.h"
#include <SDL2/SDL.h>

// OSC Music Visualization Example for WickedEngine
// This example demonstrates OSC message reception and real-time scene manipulation
// Configure your OSC transmitter to send to 127.0.0.1:7000

class OSC_MusicViz : public wi::RenderPath3D
{
private:
	wi::osc::OSCReceiver receiver;
	wi::ecs::Entity light_entities[8];
	float light_intensities[8] = {0};

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

		// Initialize OSC receiver
		if (!receiver.Initialize(7000))
		{
			wi::backlog::post("Failed to initialize OSC receiver on port 7000!", wi::backlog::LogLevel::Error);
			return;
		}

		wi::backlog::post("OSC Music Visualization - Listening on port 7000");
		wi::backlog::post("Send OSC messages on /ch/1 through /ch/8 to control lights");

		// Get scene reference
		wi::scene::Scene& scene = wi::scene::GetScene();

		// Create 8 point lights in a row
		for (int i = 0; i < 8; i++)
		{
			light_entities[i] = wi::ecs::CreateEntity();

			// Create transform component
			wi::scene::TransformComponent& transform = scene.transforms.Create(light_entities[i]);
			transform.Translate(XMFLOAT3((i - 3.5f) * 4.0f, 2.0f, 0.0f));
			transform.UpdateTransform();

			// Create light component
			wi::scene::LightComponent& light = scene.lights.Create(light_entities[i]);
			light.type = wi::scene::LightComponent::POINT;
			light.color = colors[i];
			light.range = 20.0f;
			light.intensity = 0.0f;  // Start at 0, controlled by OSC
			light.SetCastShadow(false);

			// Set up OSC callback for this channel
			std::string channel = "/ch/" + std::to_string(i + 1);
			int channel_index = i;  // Capture by value

			receiver.SetCallback(channel, [this, channel_index](const wi::osc::OSCMessage& msg) {
				if (msg.GetFloatCount() > 0)
				{
					float value = msg.GetFloat(0);
					light_intensities[channel_index] = value * 100.0f;  // Scale to intensity
				}
			});

			wi::backlog::post("Light " + std::to_string(i + 1) + " created at position (" +
				std::to_string((i - 3.5f) * 4.0f) + ", 2.0, 0.0)");
		}

		// Create a simple ground plane for reference
		wi::ecs::Entity ground_entity = wi::ecs::CreateEntity();
		wi::scene::TransformComponent& ground_transform = scene.transforms.Create(ground_entity);
		ground_transform.Scale(XMFLOAT3(50.0f, 0.1f, 50.0f));
		ground_transform.Translate(XMFLOAT3(0, -1, 0));
		ground_transform.UpdateTransform();

		// Set camera position
		wi::scene::CameraComponent& camera = wi::scene::GetCamera();
		camera.TransformCamera(wi::scene::TransformComponent());
		camera.Eye = XMFLOAT3(0, 5, -20);
		camera.At = XMFLOAT3(0, 2, 0);
		camera.UpdateCamera();

		setFXAAEnabled(true);
		setAO(AO_DISABLED);
	}

	void Update(float dt) override
	{
		// Update OSC receiver (polls socket and invokes callbacks)
		receiver.Update();

		// Apply intensity values to lights (smooth interpolation)
		wi::scene::Scene& scene = wi::scene::GetScene();
		for (int i = 0; i < 8; i++)
		{
			wi::scene::LightComponent* light = scene.lights.GetComponent(light_entities[i]);
			if (light)
			{
				// Smooth fade
				light->intensity = wi::math::Lerp(light->intensity, light_intensities[i], dt * 10.0f);
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
