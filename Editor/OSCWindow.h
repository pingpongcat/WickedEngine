#pragma once
class EditorComponent;

class OSCWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	// Main controls
	wi::gui::CheckBox enabledCheckBox;
	wi::gui::Label portLabel;
	wi::gui::TextInputField portInput;
	wi::gui::Label ipLabel;
	wi::gui::TextInputField ipInput[4];  // Four octets

	// Mapping controls
	wi::gui::Label mappingsLabel;
	wi::gui::Button addMappingButton;
	wi::gui::TreeList mappingList;

	// Selected mapping controls
	wi::gui::Label addressLabel;
	wi::gui::TextInputField addressInput;
	wi::gui::Label targetLabel;
	wi::gui::ComboBox targetCombo;
	wi::gui::Label propertyLabel;
	wi::gui::ComboBox propertyCombo;
	wi::gui::Label valueRangeLabel;
	wi::gui::TextInputField valueMinInput;
	wi::gui::TextInputField valueMaxInput;
	wi::gui::Label outputRangeLabel;
	wi::gui::TextInputField outputMinInput;
	wi::gui::TextInputField outputMaxInput;
	wi::gui::CheckBox smoothCheckBox;
	wi::gui::Slider smoothTimeSlider;
	wi::gui::ComboBox componentIndexCombo;
	wi::gui::Button removeMappingButton;

	// Lua mode controls
	wi::gui::CheckBox luaModeCheckBox;
	wi::gui::Label luaCallbackLabel;
	wi::gui::TextInputField luaCallbackInput;

	// Status display
	wi::gui::Label statusLabel;

	int selected_mapping_index = -1;

	void UpdatePropertyCombo();
	void RefreshMappingList();
	void SelectMapping(int index);

	void ResizeLayout() override;
};
