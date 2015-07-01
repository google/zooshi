// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "zooshi_entity_factory.h"

#include <set>
#include "components/editor.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

bool ZooshiEntityFactory::SetEntityLibrary(
    const char* entity_library_filename) {
  if (!LoadFile(entity_library_filename, &entity_library_data_)) {
    if (debug_)
      LogInfo("ZooshiEntityFactory: Couldn't load entity library %s",
              entity_library_filename);
    return false;
  }
  entity_library_ =
      flatbuffers::GetRoot<EntityListDef>(entity_library_data_.c_str());

  // Index each entity in the entity library by entity_id (found in EditorDef).
  for (size_t i = 0; i < entity_library_->entity_list()->size(); i++) {
    std::string entity_id;
    const EntityDef* def = entity_library_->entity_list()->Get(i);
    // search all components in this entity for the EditorDef, which contains
    // the component's entity_id.
    for (size_t j = 0; j < def->component_list()->size(); j++) {
      const ComponentDefInstance* component_def = def->component_list()->Get(j);
      if (component_def->data_type() == ComponentDataUnion_EditorDef) {
        const EditorDef* editor_def =
            static_cast<const EditorDef*>(component_def->data());
        entity_id = editor_def->entity_id()->c_str();
        break;
      }
    }
    if (entity_id != "") {
      prototype_data_[entity_id] = def;
    } else {
      LogInfo("Warning: Entity #%d in component library has no entity ID.", i);
    }
  }
  return true;
}

void ZooshiEntityFactory::InstantiateEntity(
    const EntityDef* def, entity::EntityManager* entity_manager,
    entity::EntityRef& entity, bool is_prototype) {
  if (!is_prototype && debug_)
    LogInfo("InstantiateEntity: Creating an entity.");
  const EntityDef* prototype_def = nullptr;
  // First check for an EditorDef to see if there's a prototype.
  for (size_t i = 0; i < def->component_list()->size(); i++) {
    const ComponentDefInstance* component_def = def->component_list()->Get(i);
    if (component_def->data_type() == ComponentDataUnion_EditorDef) {
      const EditorDef* editor_def =
          static_cast<const EditorDef*>(component_def->data());
      if (editor_def->prototype() != nullptr) {
        if (debug_)
          LogInfo("InstantiateEntity: Reading prototype '%s'...",
                  editor_def->prototype()->c_str());
        prototype_def = prototype_data_[editor_def->prototype()->c_str()];
        // TODO: Allow you to use an already-loaded entity as a prototype?
        // To do this you would have to ExportRawData on that entity.
        // It would be slow. We should probably discuss.
        break;
      }
    }
  }

  if (prototype_def != nullptr) {
    // Read the prototype first, then resume loading the initial entity.
    InstantiateEntity(prototype_def, entity_manager, entity, true);
  }

  std::set<ComponentDataUnion> overridden_components;

  for (size_t i = 0; i < def->component_list()->size(); i++) {
    const ComponentDefInstance* currentInstance = def->component_list()->Get(i);
    overridden_components.insert(currentInstance->data_type());
    if (is_prototype &&
        currentInstance->data_type() == ComponentDataUnion_EditorDef) {
      // EditorDef from prototypes gets loaded special.
      EditorComponent* editor_component =
          entity_manager->GetComponent<EditorComponent>();
      assert(editor_component != nullptr);
      editor_component->AddFromPrototypeData(
          entity, static_cast<const EditorDef*>(currentInstance->data()));
    } else {
      if (debug_)
        LogInfo("InstantiateEntity: ...%s provides component %d",
                is_prototype ? "prototype" : "instance",
                currentInstance->data_type());
      entity::ComponentInterface* component =
          entity_manager->GetComponent(currentInstance->data_type());
      assert(component != nullptr);
      component->AddFromRawData(entity, currentInstance);
    }
  }

  if (!is_prototype) {
    EditorData* editor_data =
        entity_manager->GetComponent<EditorComponent>()->AddEntity(entity);
    for (int component_id = 0; component_id < entity::kMaxComponentCount;
         component_id++) {
      ComponentDataUnion component_type =
          static_cast<ComponentDataUnion>(component_id);
      // If we don't already have an EditorComponent, we should get one added.
      if (entity_manager->GetComponent(component_id) != nullptr &&
          entity->IsRegisteredForComponent(component_id) &&
          overridden_components.find(component_type) ==
              overridden_components.end()) {
        editor_data->components_from_prototype.insert(component_type);
      }
    }
  }
}

// Factory method for the entity manager, for converting data (in our case.
// flatbuffer definitions) into entities and sticking them into the system.
entity::EntityRef ZooshiEntityFactory::CreateEntityFromData(
    const void* data, entity::EntityManager* entity_manager) {
  const EntityDef* def = static_cast<const EntityDef*>(data);
  assert(def != nullptr);
  entity::EntityRef entity = entity_manager->AllocateNewEntity();
  InstantiateEntity(def, entity_manager, entity,
                    false);  // recursively reads prototypes
  return entity;
}

entity::EntityRef ZooshiEntityFactory::CreateEntityFromPrototype(
    const char* prototype_name, entity::EntityManager* entity_manager) {
  if (prototype_requests_.find(prototype_name) == prototype_requests_.end()) {
    // Create the prototype request for the first time. It will be cached
    // for future use.
    flatbuffers::FlatBufferBuilder fbb;
    auto prototype = fbb.CreateString(prototype_name);
    EditorDefBuilder builder(fbb);
    builder.add_prototype(prototype);
    auto component = CreateComponentDefInstance(
        fbb, ComponentDataUnion_EditorDef, builder.Finish().Union());
    auto entity_def = CreateEntityDef(
        fbb,
        fbb.CreateVector(
            std::vector<flatbuffers::Offset<ComponentDefInstance>>{component}));
    fbb.Finish(entity_def);
    prototype_requests_[prototype_name] = {
        fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize()};
  }
  return CreateEntityFromData(flatbuffers::GetRoot<EntityDef>(
                                  prototype_requests_[prototype_name].data()),
                              entity_manager);
}

}  // namespace fpl_project
}  // namespace fpl
