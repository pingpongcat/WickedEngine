#include "WickedEngine.h"
#include <SDL2/SDL.h>

class OSC_Example : public wi::RenderPath3D
{
private:
	wi::osc::OSCReceiver receiver;
	wi::ecs::Entity light_entities[8];
	float light_intensities[8] = {0};

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

		receiver.Initialize(7000);

		wi::scene::Scene& scene = wi::scene::GetScene();

		// Create 8 point lights in a circle
		const float PI = 3.14159265358979323846f;
		for (int i = 0; i < 8; i++)
		{
			light_entities[i] = wi::ecs::CreateEntity();

			float angle = (2.0f * PI * i) / 8;
			float x = 8.0f * cosf(angle);
			float z = 8.0f * sinf(angle);

			wi::scene::TransformComponent& transform = scene.transforms.Create(light_entities[i]);
			transform.Translate(XMFLOAT3(x, 3.0f, z));
			transform.UpdateTransform();

			wi::scene::LightComponent& light = scene.lights.Create(light_entities[i]);
			light.type = wi::scene::LightComponent::POINT;
			light.color = colors[i];
			light.range = 20.0f;
			light.intensity = 0.0f;
			light.SetCastShadow(false);

			std::string channel = "/ch/" + std::to_string(i + 1);
			int channel_index = i;

			receiver.SetCallback(channel, [this, channel_index](const wi::osc::OSCMessage& msg) {
				if (msg.GetFloatCount() > 0)
				{
					light_intensities[channel_index] = msg.GetFloat(0) * 100.0f;
				}
			});
		}

		wi::scene::LoadModel("../../Content/models/teapot.wiscene");

		wi::scene::CameraComponent& camera = wi::scene::GetCamera();
		wi::scene::TransformComponent cam_transform;
		cam_transform.Translate(XMFLOAT3(0, 5, -20));
		cam_transform.UpdateTransform();
		camera.TransformCamera(cam_transform);
		camera.UpdateCamera();

		setFXAAEnabled(true);
	}

	void Update(float dt) override
	{
		receiver.Update();

		wi::scene::Scene& scene = wi::scene::GetScene();
		for (int i = 0; i < 8; i++)
		{
			wi::scene::LightComponent* light = scene.lights.GetComponent(light_entities[i]);
			if (light)
			{
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
	sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
	sdl2::window_ptr_t window = sdl2::make_window(
		"WickedEngine - OSC Example",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	application.SetWindow(window.get());
	wi::arguments::Parse(argc, argv);

	OSC_Example example;
	example.Load();
	application.ActivatePath(&example);

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
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
					quit = true;
				else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					application.SetWindow(application.window);
				break;
			}
			wi::input::sdlinput::ProcessEvent(event);
		}
	}

	wi::jobsystem::ShutDown();
	SDL_Quit();

	return 0;
}
