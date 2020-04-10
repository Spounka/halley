#include "entity_editor.h"

#include "add_component_window.h"
#include "entity_editor_factories.h"
#include "halley/tools/ecs/ecs_data.h"
#include "scene_editor_window.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
	, context(factory, [=] () { onEntityUpdated(); })
{
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
	makeUI();
}

void EntityEditor::update(Time t, bool moved)
{
	if (needToReloadUI) {
		reloadEntity();
		needToReloadUI = false;
	}
}

void EntityEditor::setSceneEditorWindow(SceneEditorWindow& editor)
{
	sceneEditor = &editor;
}

void EntityEditor::setECSData(ECSData& ecs)
{
	ecsData = &ecs;
}

void EntityEditor::makeUI()
{
	add(factory.makeUI("ui/halley/entity_editor"), 1);
	fields = getWidget("fields");
	fields->setMinSize(Vector2f(300, 20));

	entityName = getWidgetAs<UITextInput>("entityName");

	setHandle(UIEventType::ButtonClicked, "addComponentButton", [=](const UIEvent& event)
	{
		addComponent();
	});

	setHandle(UIEventType::TextSubmit, "entityName", [=] (const UIEvent& event)
	{
		setName(event.getStringData());
	});
}

bool EntityEditor::loadEntity(const String& id, ConfigNode& data, bool force)
{
	Expects(ecsData);

	if (currentId == id && currentEntityData == &data && !force) {
		return false;
	}
	
	currentEntityData = &data;
	currentId = id;
	
	reloadEntity();
	return true;
}

void EntityEditor::reloadEntity()
{
	fields->clear();
	if (getEntityData()["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: getEntityData()["components"].asSequence()) {
			for (auto& c: componentNode.asMap()) {
				loadComponentData(c.first, c.second);
			}
		}
	}

	entityName->setText(getEntityData()["name"].asString(""));
}

void EntityEditor::loadComponentData(const String& componentType, ConfigNode& data)
{
	auto componentUI = factory.makeUI("ui/halley/entity_editor_component");
	componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));
	componentUI->setHandle(UIEventType::ButtonClicked, "deleteComponentButton", [=] (const UIEvent& event)
	{
		deleteComponent(componentType);
	});

	auto componentFields = componentUI->getWidget("componentFields");
	
	const auto iter = ecsData->getComponents().find(componentType);
	if (iter != ecsData->getComponents().end()) {
		const auto& componentData = iter->second;
		for (auto& member: componentData.members) {
			const String fieldName = member.name;
			
			auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromUserString(fieldName));
			label->setMaxWidth(100);
			label->setMarquee(true);

			auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
			labelBox->add(label);
			
			componentFields->add(labelBox, 0, {}, UISizerAlignFlags::CentreVertical);
			componentFields->add(createEditField(member.type.name, fieldName, data, member.defaultValue), 1);
		}
	}
	
	fields->add(componentUI);
}

std::shared_ptr<IUIElement> EntityEditor::createEditField(const String& fieldType, const String& fieldName, ConfigNode& componentData, const String& defaultValue)
{
	auto iter = fieldFactories.find(fieldType);
	if (iter != fieldFactories.end()) {
		return iter->second->createField(context, fieldName, componentData, defaultValue);
	} else {
		return std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromHardcodedString("N/A"));
	}
}

void EntityEditor::addComponent()
{
	std::set<String> existingComponents;
	auto& components = getEntityData()["components"];
	if (components.getType() == ConfigNodeType::Sequence) {
		for (auto& c: components.asSequence()) {
			for (auto& kv: c.asMap()) {
				existingComponents.insert(kv.first);
			}
		}
	}
	
	std::vector<String> componentNames;
	for (auto& c: ecsData->getComponents()) {
		if (existingComponents.find(c.first) == existingComponents.end()) {
			componentNames.push_back(c.first);
		}
	}
	std::sort(componentNames.begin(), componentNames.end());
	
	getRoot()->addChild(std::make_shared<AddComponentWindow>(factory, componentNames, [=] (std::optional<String> result)
	{
		if (result) {
			addComponent(result.value());
		}
	}));
}

void EntityEditor::addComponent(const String& name)
{
	auto& components = getEntityData()["components"];
	if (components.getType() != ConfigNodeType::Sequence) {
		components = ConfigNode::SequenceType();
	}

	ConfigNode compNode = ConfigNode::MapType();
	compNode[name] = ConfigNode::MapType();
	
	components.asSequence().emplace_back(std::move(compNode));

	needToReloadUI = true;	
	onEntityUpdated();
}

void EntityEditor::deleteComponent(const String& name)
{
	auto& components = getEntityData()["components"];
	if (components.getType() == ConfigNodeType::Sequence) {
		auto& componentSequence = components.asSequence();
		bool found = false;
		
		for (size_t i = 0; i < componentSequence.size(); ++i) {
			for (auto& c: componentSequence[i].asMap()) {
				if (c.first == name) {
					found = true;
					break;
				}
			}

			if (found) {
				componentSequence.erase(componentSequence.begin() + i);
				break;
			}
		}
	}

	needToReloadUI = true;
	onEntityUpdated();
}

void EntityEditor::setName(const String& name)
{
	getEntityData()["name"] = name;
	onEntityUpdated();
}

void EntityEditor::onEntityUpdated()
{
	sceneEditor->onEntityModified(currentId);
}

ConfigNode& EntityEditor::getEntityData()
{
	return *currentEntityData;
}

void EntityEditor::addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}