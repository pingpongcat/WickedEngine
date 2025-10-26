#include "stdafx.h"
#include "OSCWindow.h"
#include "Editor.h"
#include "IconDefinitions.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void OSCWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_OSC " OSC Component", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(520, 600));

	closeButton.SetTooltip("Delete OSCComponent");
	OnClose([=](wi::gui::EventArgs args) {
		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().oscs.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 120;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 200;

	// Helper lambda for applying to all selected entities
	auto forEachSelected = [this](auto func) {
		return [this, func](auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				OSCComponent* osc = scene.oscs.GetComponent(x.entity);
				if (osc != nullptr)
				{
					func(osc, args);
				}
			}
		};
	};

	// Enabled checkbox
	enabledCheckBox.Create("Enabled: ");
	enabledCheckBox.SetTooltip("Enable/disable OSC control for this entity");
	enabledCheckBox.SetPos(XMFLOAT2(x, y));
	enabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	enabledCheckBox.OnClick(forEachSelected([](auto osc, auto args) {
		if (args.bValue)
			osc->Enable();
		else
			osc->Disable();
	}));
	AddWidget(&enabledCheckBox);

	// Port label and input
	portLabel.Create("Listen Port:");
	portLabel.SetPos(XMFLOAT2(x, y += step));
	portLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&portLabel);

	portInput.Create("");
	portInput.SetTooltip("UDP port to listen on (default: 7000)");
	portInput.SetPos(XMFLOAT2(x, y += step));
	portInput.SetSize(XMFLOAT2(wid, hei));
	portInput.SetText("7000");
	portInput.OnInputAccepted(forEachSelected([](auto osc, auto args) {
		osc->listen_port = (uint16_t)std::atoi(args.sValue.c_str());
	}));
	AddWidget(&portInput);

	// IP Address label and inputs
	ipLabel.Create("Listen IP:");
	ipLabel.SetPos(XMFLOAT2(x, y += step));
	ipLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&ipLabel);

	float ip_wid = 45;
	ipInput[0].Create("");
	ipInput[0].SetTooltip("IP address octet (0-255). Use 127.0.0.1 for localhost");
	ipInput[0].SetPos(XMFLOAT2(x, y += step));
	ipInput[0].SetSize(XMFLOAT2(ip_wid, hei));
	ipInput[0].SetText("127");
	ipInput[0].OnInputAccepted([this, forEachSelected](wi::gui::EventArgs args) {
		forEachSelected([&args](auto osc, auto) {
			osc->listen_ip[0] = (uint8_t)std::atoi(args.sValue.c_str());
		})(args);
	});
	AddWidget(&ipInput[0]);

	ipInput[1].Create("");
	ipInput[1].SetTooltip("IP address octet (0-255). Use 127.0.0.1 for localhost");
	ipInput[1].SetPos(XMFLOAT2(x + ip_wid + 3, y));
	ipInput[1].SetSize(XMFLOAT2(ip_wid, hei));
	ipInput[1].SetText("0");
	ipInput[1].OnInputAccepted([this, forEachSelected](wi::gui::EventArgs args) {
		forEachSelected([&args](auto osc, auto) {
			osc->listen_ip[1] = (uint8_t)std::atoi(args.sValue.c_str());
		})(args);
	});
	AddWidget(&ipInput[1]);

	ipInput[2].Create("");
	ipInput[2].SetTooltip("IP address octet (0-255). Use 127.0.0.1 for localhost");
	ipInput[2].SetPos(XMFLOAT2(x + (ip_wid + 3) * 2, y));
	ipInput[2].SetSize(XMFLOAT2(ip_wid, hei));
	ipInput[2].SetText("0");
	ipInput[2].OnInputAccepted([this, forEachSelected](wi::gui::EventArgs args) {
		forEachSelected([&args](auto osc, auto) {
			osc->listen_ip[2] = (uint8_t)std::atoi(args.sValue.c_str());
		})(args);
	});
	AddWidget(&ipInput[2]);

	ipInput[3].Create("");
	ipInput[3].SetTooltip("IP address octet (0-255). Use 127.0.0.1 for localhost");
	ipInput[3].SetPos(XMFLOAT2(x + (ip_wid + 3) * 3, y));
	ipInput[3].SetSize(XMFLOAT2(ip_wid, hei));
	ipInput[3].SetText("1");
	ipInput[3].OnInputAccepted([this, forEachSelected](wi::gui::EventArgs args) {
		forEachSelected([&args](auto osc, auto) {
			osc->listen_ip[3] = (uint8_t)std::atoi(args.sValue.c_str());
		})(args);
	});
	AddWidget(&ipInput[3]);

	// Mappings section
	mappingsLabel.Create("Property Mappings:");
	mappingsLabel.SetPos(XMFLOAT2(x, y += step));
	mappingsLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&mappingsLabel);

	addMappingButton.Create("Add Mapping " ICON_FA_PLUS);
	addMappingButton.SetTooltip("Add a new OSC to property mapping");
	addMappingButton.SetPos(XMFLOAT2(x, y += step));
	addMappingButton.SetSize(XMFLOAT2(wid, hei));
	addMappingButton.OnClick([this](wi::gui::EventArgs args) {
		OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
		if (osc != nullptr)
		{
			OSCComponent::PropertyMapping mapping;
			mapping.osc_address = "/ch/1";
			mapping.target_path = AnimationComponent::AnimationChannel::Path::TRANSLATION;
			mapping.value_min = 0.0f;
			mapping.value_max = 1.0f;
			mapping.output_min = 0.0f;
			mapping.output_max = 10.0f;
			mapping.component_index = 0;  // X axis only

			osc->mappings.push_back(mapping);
			RefreshMappingList();
		}
	});
	AddWidget(&addMappingButton);

	// Mapping list
	mappingList.Create("Mappings");
	mappingList.SetPos(XMFLOAT2(x, y += step));
	mappingList.SetSize(XMFLOAT2(wid, 100));
	mappingList.OnSelect([this](wi::gui::EventArgs args) {
		if (args.iValue >= 0)
		{
			SelectMapping(args.iValue);
		}
	});
	AddWidget(&mappingList);

	y += 100;  // Move past the mapping list height

	// Remove mapping button (select a mapping first, then click this to remove it)
	removeMappingButton.Create("Remove Selected " ICON_FA_XMARK);
	removeMappingButton.SetTooltip("Remove the selected mapping from the list above");
	removeMappingButton.SetPos(XMFLOAT2(x, y += step));
	removeMappingButton.SetSize(XMFLOAT2(wid, hei));
	removeMappingButton.OnClick([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings.erase(osc->mappings.begin() + selected_mapping_index);
				selected_mapping_index = -1;
				RefreshMappingList();
			}
		}
	});
	AddWidget(&removeMappingButton);

	y += step;

	// Selected mapping controls
	addressLabel.Create("OSC Address:");
	addressLabel.SetPos(XMFLOAT2(x, y += step));
	addressLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&addressLabel);

	addressInput.Create("");
	addressInput.SetTooltip("OSC address pattern (e.g. /ch/1, /fader/1)");
	addressInput.SetPos(XMFLOAT2(x, y += step));
	addressInput.SetSize(XMFLOAT2(wid, hei));
	addressInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].osc_address = args.sValue;
				RefreshMappingList();
			}
		}
	});
	AddWidget(&addressInput);

	// Property selection
	propertyLabel.Create("Property:");
	propertyLabel.SetPos(XMFLOAT2(x, y += step));
	propertyLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&propertyLabel);

	propertyCombo.Create("Property");
	propertyCombo.SetTooltip("Select which property to control");
	propertyCombo.SetPos(XMFLOAT2(x, y += step));
	propertyCombo.SetSize(XMFLOAT2(wid, hei));
	propertyCombo.OnSelect([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].target_path =
					(AnimationComponent::AnimationChannel::Path)args.iValue;
			}
		}
	});
	UpdatePropertyCombo();
	AddWidget(&propertyCombo);

	// Value range
	valueRangeLabel.Create("OSC Range:");
	valueRangeLabel.SetPos(XMFLOAT2(x, y += step));
	valueRangeLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&valueRangeLabel);

	float range_wid = 95;
	valueMinInput.Create("");
	valueMinInput.SetTooltip("Minimum OSC input value");
	valueMinInput.SetPos(XMFLOAT2(x, y += step));
	valueMinInput.SetSize(XMFLOAT2(range_wid, hei));
	valueMinInput.SetText("0.0");
	valueMinInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].value_min = (float)std::atof(args.sValue.c_str());
			}
		}
	});
	AddWidget(&valueMinInput);

	valueMaxInput.Create("");
	valueMaxInput.SetTooltip("Maximum OSC input value");
	valueMaxInput.SetPos(XMFLOAT2(x + range_wid + 5, y));
	valueMaxInput.SetSize(XMFLOAT2(range_wid, hei));
	valueMaxInput.SetText("1.0");
	valueMaxInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].value_max = (float)std::atof(args.sValue.c_str());
			}
		}
	});
	AddWidget(&valueMaxInput);

	y += step;

	// Output range
	outputRangeLabel.Create("Output Range:");
	outputRangeLabel.SetPos(XMFLOAT2(x, y += step));
	outputRangeLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&outputRangeLabel);

	outputMinInput.Create("");
	outputMinInput.SetTooltip("Minimum output value");
	outputMinInput.SetPos(XMFLOAT2(x, y += step));
	outputMinInput.SetSize(XMFLOAT2(range_wid, hei));
	outputMinInput.SetText("0.0");
	outputMinInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].output_min = (float)std::atof(args.sValue.c_str());
			}
		}
	});
	AddWidget(&outputMinInput);

	outputMaxInput.Create("");
	outputMaxInput.SetTooltip("Maximum output value");
	outputMaxInput.SetPos(XMFLOAT2(x + range_wid + 5, y));
	outputMaxInput.SetSize(XMFLOAT2(range_wid, hei));
	outputMaxInput.SetText("1.0");
	outputMaxInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].output_max = (float)std::atof(args.sValue.c_str());
			}
		}
	});
	AddWidget(&outputMaxInput);

	y += step;

	// Smooth checkbox and slider
	smoothCheckBox.Create("Smooth: ");
	smoothCheckBox.SetTooltip("Enable smooth interpolation of values");
	smoothCheckBox.SetPos(XMFLOAT2(x, y += step));
	smoothCheckBox.SetSize(XMFLOAT2(hei, hei));
	smoothCheckBox.OnClick([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].smooth = args.bValue;
				smoothTimeSlider.SetEnabled(args.bValue);
			}
		}
	});
	AddWidget(&smoothCheckBox);

	smoothTimeSlider.Create(0.01f, 1.0f, 0.1f, 1000, "Smooth Time: ");
	smoothTimeSlider.SetTooltip("Time in seconds for smooth interpolation");
	smoothTimeSlider.SetPos(XMFLOAT2(x, y += step));
	smoothTimeSlider.SetSize(XMFLOAT2(wid, hei));
	smoothTimeSlider.OnSlide([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].smooth_time = args.fValue;
			}
		}
	});
	AddWidget(&smoothTimeSlider);

	// Component index combo
	componentIndexCombo.Create("Component:");
	componentIndexCombo.SetTooltip("For multi-component properties (X/Y/Z), select which component to control");
	componentIndexCombo.SetPos(XMFLOAT2(x, y += step));
	componentIndexCombo.SetSize(XMFLOAT2(wid, hei));
	componentIndexCombo.AddItem("All (X/Y/Z)", -1);
	componentIndexCombo.AddItem("X / R", 0);
	componentIndexCombo.AddItem("Y / G", 1);
	componentIndexCombo.AddItem("Z / B", 2);
	componentIndexCombo.AddItem("W / A", 3);
	componentIndexCombo.OnSelect([this](wi::gui::EventArgs args) {
		if (selected_mapping_index >= 0)
		{
			OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
			if (osc != nullptr && selected_mapping_index < (int)osc->mappings.size())
			{
				osc->mappings[selected_mapping_index].component_index = args.iValue;
			}
		}
	});
	AddWidget(&componentIndexCombo);

	// Lua mode section
	luaModeCheckBox.Create("Lua Callback Mode: ");
	luaModeCheckBox.SetTooltip("Use Lua code instead of direct property mappings");
	luaModeCheckBox.SetPos(XMFLOAT2(x, y += step * 2));
	luaModeCheckBox.SetSize(XMFLOAT2(hei, hei));
	luaModeCheckBox.OnClick(forEachSelected([](auto osc, auto args) {
		osc->SetLuaMode(args.bValue);
	}));
	AddWidget(&luaModeCheckBox);

	luaCallbackLabel.Create("Lua Callback Code:");
	luaCallbackLabel.SetPos(XMFLOAT2(x, y += step));
	luaCallbackLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&luaCallbackLabel);

	luaCallbackInput.Create("");
	luaCallbackInput.SetTooltip("Lua code executed on OSC message.\nAvailable: entity, osc_address, osc_value");
	luaCallbackInput.SetPos(XMFLOAT2(x, y += step));
	luaCallbackInput.SetSize(XMFLOAT2(wid, 80));
	luaCallbackInput.SetText("-- Example:\n-- local light = GetLight(entity)\n-- light:SetIntensity(osc_value * 50)");
	luaCallbackInput.OnInputAccepted(forEachSelected([](auto osc, auto args) {
		osc->lua_callback = args.sValue;
	}));
	AddWidget(&luaCallbackInput);

	y += 85;

	// Status label
	statusLabel.Create("Status: Inactive");
	statusLabel.SetPos(XMFLOAT2(x, y += step));
	statusLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&statusLabel);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void OSCWindow::SetEntity(Entity _entity)
{
	entity = _entity;

	Scene& scene = editor->GetCurrentScene();
	OSCComponent* osc = scene.oscs.GetComponent(entity);

	if (osc != nullptr)
	{
		enabledCheckBox.SetCheck(osc->IsEnabled());
		portInput.SetText(std::to_string(osc->listen_port));
		for (int i = 0; i < 4; i++)
		{
			ipInput[i].SetText(std::to_string(osc->listen_ip[i]));
		}

		luaModeCheckBox.SetCheck(osc->IsLuaMode());
		if (!osc->lua_callback.empty())
		{
			luaCallbackInput.SetText(osc->lua_callback);
		}

		RefreshMappingList();

		if (osc->IsEnabled())
		{
			statusLabel.SetText("Status: Enabled, listening on " +
				std::to_string((int)osc->listen_ip[0]) + "." +
				std::to_string((int)osc->listen_ip[1]) + "." +
				std::to_string((int)osc->listen_ip[2]) + "." +
				std::to_string((int)osc->listen_ip[3]) + ":" +
				std::to_string(osc->listen_port));
		}
		else
		{
			statusLabel.SetText("Status: Disabled");
		}
	}
}

void OSCWindow::UpdatePropertyCombo()
{
	propertyCombo.ClearItems();

	// Add common properties - these are available regardless of entity components
	propertyCombo.AddItem("Transform - Translation", (uint64_t)AnimationComponent::AnimationChannel::Path::TRANSLATION);
	propertyCombo.AddItem("Transform - Rotation", (uint64_t)AnimationComponent::AnimationChannel::Path::ROTATION);
	propertyCombo.AddItem("Transform - Scale", (uint64_t)AnimationComponent::AnimationChannel::Path::SCALE);

	propertyCombo.AddItem("Light - Intensity", (uint64_t)AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY);
	propertyCombo.AddItem("Light - Range", (uint64_t)AnimationComponent::AnimationChannel::Path::LIGHT_RANGE);
	propertyCombo.AddItem("Light - Color", (uint64_t)AnimationComponent::AnimationChannel::Path::LIGHT_COLOR);

	propertyCombo.AddItem("Material - Color", (uint64_t)AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR);
	propertyCombo.AddItem("Material - Emissive", (uint64_t)AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE);
	propertyCombo.AddItem("Material - Roughness", (uint64_t)AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS);
	propertyCombo.AddItem("Material - Metalness", (uint64_t)AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS);
	propertyCombo.AddItem("Material - Reflectance", (uint64_t)AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE);

	propertyCombo.AddItem("Camera - FOV", (uint64_t)AnimationComponent::AnimationChannel::Path::CAMERA_FOV);
	propertyCombo.AddItem("Camera - Focal Length", (uint64_t)AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH);

	propertyCombo.AddItem("Sound - Volume", (uint64_t)AnimationComponent::AnimationChannel::Path::SOUND_VOLUME);

	propertyCombo.SetSelected(0);
}

void OSCWindow::RefreshMappingList()
{
	mappingList.ClearItems();

	OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
	if (osc != nullptr)
	{
		for (size_t i = 0; i < osc->mappings.size(); i++)
		{
			const auto& mapping = osc->mappings[i];
			std::string label = mapping.osc_address + " → ";

			// Convert path to readable name
			switch (mapping.target_path)
			{
			case AnimationComponent::AnimationChannel::Path::TRANSLATION: label += "Position"; break;
			case AnimationComponent::AnimationChannel::Path::SCALE: label += "Scale"; break;
			case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY: label += "Light Intensity"; break;
			case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE: label += "Light Range"; break;
			case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR: label += "Light Color"; break;
			case AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR: label += "Material Color"; break;
			case AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE: label += "Material Emissive"; break;
			case AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS: label += "Roughness"; break;
			case AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS: label += "Metalness"; break;
			case AnimationComponent::AnimationChannel::Path::CAMERA_FOV: label += "Camera FOV"; break;
			case AnimationComponent::AnimationChannel::Path::SOUND_VOLUME: label += "Sound Volume"; break;
			default: label += "Unknown"; break;
			}

			mappingList.AddItem(label);
		}
	}

	if (selected_mapping_index >= 0 && selected_mapping_index < (int)mappingList.GetItemCount())
	{
		mappingList.Select(selected_mapping_index);
	}
}

void OSCWindow::SelectMapping(int index)
{
	selected_mapping_index = index;

	OSCComponent* osc = editor->GetCurrentScene().oscs.GetComponent(entity);
	if (osc != nullptr && index >= 0 && index < (int)osc->mappings.size())
	{
		const auto& mapping = osc->mappings[index];

		addressInput.SetText(mapping.osc_address);
		propertyCombo.SetSelectedByUserdataWithoutCallback((uint64_t)mapping.target_path);
		valueMinInput.SetText(std::to_string(mapping.value_min));
		valueMaxInput.SetText(std::to_string(mapping.value_max));
		outputMinInput.SetText(std::to_string(mapping.output_min));
		outputMaxInput.SetText(std::to_string(mapping.output_max));
		smoothCheckBox.SetCheck(mapping.smooth);
		smoothTimeSlider.SetValue(mapping.smooth_time);
		smoothTimeSlider.SetEnabled(mapping.smooth);
		componentIndexCombo.SetSelectedWithoutCallback(mapping.component_index);
	}
}

void OSCWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 120;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add(enabledCheckBox);
	add(portLabel);
	add(portInput);
	add(ipLabel);

	// IP inputs - 4 boxes in a row
	const float margin_left = 120;
	const float margin_right = padding;
	float ip_width = 45;
	ipInput[0].SetPos(XMFLOAT2(margin_left, y));
	ipInput[0].SetSize(XMFLOAT2(ip_width, ipInput[0].GetSize().y));
	ipInput[1].SetPos(XMFLOAT2(margin_left + ip_width + 3, y));
	ipInput[1].SetSize(XMFLOAT2(ip_width, ipInput[1].GetSize().y));
	ipInput[2].SetPos(XMFLOAT2(margin_left + (ip_width + 3) * 2, y));
	ipInput[2].SetSize(XMFLOAT2(ip_width, ipInput[2].GetSize().y));
	ipInput[3].SetPos(XMFLOAT2(margin_left + (ip_width + 3) * 3, y));
	ipInput[3].SetSize(XMFLOAT2(ip_width, ipInput[3].GetSize().y));
	y += ipInput[0].GetSize().y;
	y += padding;

	add(mappingsLabel);
	add(addMappingButton);
	add_fullwidth(mappingList);
	add(addressLabel);
	add(addressInput);
	add(propertyLabel);
	add(propertyCombo);
	add(valueRangeLabel);

	// Value range inputs - min/max in a row
	float range_width = (width - margin_left - margin_right - padding) / 2.0f;
	valueMinInput.SetPos(XMFLOAT2(margin_left, y));
	valueMinInput.SetSize(XMFLOAT2(range_width, valueMinInput.GetSize().y));
	valueMaxInput.SetPos(XMFLOAT2(margin_left + range_width + padding, y));
	valueMaxInput.SetSize(XMFLOAT2(range_width, valueMaxInput.GetSize().y));
	y += valueMinInput.GetSize().y;
	y += padding;

	add(outputRangeLabel);

	// Output range inputs - min/max in a row
	outputMinInput.SetPos(XMFLOAT2(margin_left, y));
	outputMinInput.SetSize(XMFLOAT2(range_width, outputMinInput.GetSize().y));
	outputMaxInput.SetPos(XMFLOAT2(margin_left + range_width + padding, y));
	outputMaxInput.SetSize(XMFLOAT2(range_width, outputMaxInput.GetSize().y));
	y += outputMinInput.GetSize().y;
	y += padding;

	add(smoothCheckBox);
	add(smoothTimeSlider);
	add(componentIndexCombo);
	add(removeMappingButton);
	add(luaModeCheckBox);
	add(luaCallbackLabel);
	add_fullwidth(luaCallbackInput);
	add(statusLabel);
}
