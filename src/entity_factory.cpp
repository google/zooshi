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

#include "entity_factory.h"

#include <set>
#include "components/editor.h"
#include "fplbase/utilities.h"

namespace fpl {

using fpl_project::EditorComponent;
using fpl_project::EditorData;

bool EntityFactory::AddEntityLibrary(const char* entity_library_filename) {
  std::string library_data;
  if (!LoadFile(entity_library_filename, &library_data)) {
    LogInfo("EntityFactory: Couldn't load entity library %s",
            entity_library_filename);
    return false;
  }
  loaded_files_[entity_library_filename] = library_data;

  std::vector<const void*> entities;
  if (!ReadEntityList(loaded_files_[entity_library_filename].c_str(),
                      &entities)) {
    LogInfo("EntityFactory: Couldn't read entity library list '%s'",
            entity_library_filename);
    return false;
  }
  if (debug_entity_creation()) {
    LogInfo("EntityFactory: Reading %d prototypes from file %s",
            entities.size(), entity_library_filename);
  }
  unsigned int editor_id = EditorComponent::GetComponentId();

  for (size_t i = 0; i < entities.size(); i++) {
    std::vector<const void*> components;
    if (!ReadEntityDefinition(entities[i], &components)) {
      LogInfo("EntityFactory: Library entity %d read error, skipping", i);
      continue;  // try reading the rest of the prototypes
    }

    // Read the name from the entity component definition.
    const EditorDef* editor_def =
        static_cast<const EditorDef*>(components[editor_id]);
    if (editor_def == nullptr || editor_def->entity_id() == nullptr) {
      LogInfo("EntityFactory: Library entity %d has no entity_id, skipping", i);
      continue;
    }

    if (debug_entity_creation()) {
      LogInfo("EntityFactory: Loaded prototype %s from file %s",
              editor_def->entity_id()->c_str(), entity_library_filename);
    }
    // This entity has an entity ID, so index the pointer to it.
    prototype_data_[editor_def->entity_id()->c_str()] = entities[i];
  }
  return true;
}

int EntityFactory::LoadEntitiesFromFile(const char* filename,
                                        entity::EntityManager* entity_manager) {
  LogInfo("EntityFactory::LoadEntitiesFromFile: Reading %s", filename);
  if (loaded_files_.find(filename) == loaded_files_.end()) {
    std::string data;
    if (!LoadFile(filename, &data)) {
      LogInfo("EntityFactory::LoadEntitiesFromFile: Couldn't open file %s",
              filename);
      return kErrorLoadingEntities;
    }
    loaded_files_[filename] = data;
  }

  std::vector<entity::EntityRef> entities_loaded;
  int total = LoadEntityListFromMemory(loaded_files_[filename].c_str(),
                                       entity_manager, &entities_loaded);

  // Go through all the entities we just created and mark them with their source
  // file.
  EditorComponent* editor_component =
      entity_manager->GetComponent<EditorComponent>();
  for (size_t i = 0; i < entities_loaded.size(); i++) {
    entity::EntityRef& entity = entities_loaded[i];
    // Track which source file this entity came from, so we can later output
    // it to the same file if we save it in the editor.
    if (editor_component != nullptr) {
      entity_manager->GetComponent<EditorComponent>()->AddWithSourceFile(
          entity, filename);
    }
  }
  LogInfo("EntityFactory::LoadEntitiesFromFile: Loaded %d entities from %s",
          total, filename);
  return total;
}

int EntityFactory::LoadEntityListFromMemory(
    const void* entity_list, entity::EntityManager* entity_manager,
    std::vector<entity::EntityRef>* entities_loaded) {
  std::vector<const void*> entities;
  if (!ReadEntityList(entity_list, &entities)) {
    LogInfo("EntityFactory: Couldn't read entity list");
    return kErrorLoadingEntities;
  }

  if (entities_loaded != nullptr) entities_loaded->reserve(entities.size());
  int total = 0;
  for (size_t i = 0; i < entities.size(); i++) {
    total++;
    entity::EntityRef entity =
        entity_manager->CreateEntityFromData(entities[i]);
    if (entity && entities_loaded != nullptr) {
      entities_loaded->push_back(entity);
    }
  }
  return total;
}

void EntityFactory::LoadEntityData(const void* def,
                                   entity::EntityManager* entity_manager,
                                   entity::EntityRef& entity,
                                   bool is_prototype) {
  unsigned int editor_id = EditorComponent::GetComponentId();
  EditorComponent* editor_component =
      entity_manager->GetComponent<EditorComponent>();

  std::vector<const void*> components;
  if (!ReadEntityDefinition(def, &components)) {
    LogError("EntityFactory::LoadEntityData: Couldn't load entity data.");
    return;
  }

  const EditorDef* editor_def =
      static_cast<const EditorDef*>(components[editor_id]);
  if (editor_def != nullptr && editor_def->prototype() != nullptr) {
    const char* prototype_name = editor_def->prototype()->c_str();
    if (prototype_data_.find(prototype_name) != prototype_data_.end()) {
      if (debug_entity_creation()) {
        LogInfo("EntityFactory::LoadEntityData: Loading prototype: %s",
                prototype_name);
      }
      LoadEntityData(prototype_data_[prototype_name], entity_manager, entity,
                     true);
    } else {
      LogError("EntityFactory::LoadEntityData: Invalid prototype: '%s'",
               prototype_name);
    }
  }

  std::set<entity::ComponentId> overridden_components;

  for (size_t i = 0; i < components.size(); i++) {
    const void* component_data = components[i];
    if (component_data != nullptr) {
      if (debug_entity_creation()) {
        LogInfo("...reading %s from %s", ComponentIdToTableName(i),
                is_prototype ? "prototype" : "entity", i);
      }
      overridden_components.insert(i);
      if (is_prototype && i == editor_id) {
        // EditorDef from prototypes gets loaded special.
        editor_component->AddFromPrototypeData(
            entity, static_cast<const EditorDef*>(component_data));
      } else {
        entity::ComponentInterface* component = entity_manager->GetComponent(i);
        assert(component != nullptr);
        component->AddFromRawData(entity, component_data);
      }
    }
  }

  // Final clean-up on the top-level entity load: keep track of which parts
  // came from the initial entity and which came from prototypes.
  if (!is_prototype) {
    EditorData* editor_data =
        entity_manager->GetComponent<EditorComponent>()->AddEntity(entity);
    for (int component_id = 0; component_id <= max_component_id();
         component_id++) {
      // If we don't already have an EditorComponent, we should get one added.
      if (entity_manager->GetComponent(component_id) != nullptr &&
          entity->IsRegisteredForComponent(component_id) &&
          overridden_components.find(component_id) ==
              overridden_components.end()) {
        editor_data->components_from_prototype.insert(component_id);
      }
    }
  }
}

// Factory method for the entity manager, for converting data (in our case.
// flatbuffer definitions) into entities and sticking them into the system.
entity::EntityRef EntityFactory::CreateEntityFromData(
    const void* data, entity::EntityManager* entity_manager) {
  assert(data != nullptr);
  if (debug_entity_creation()) {
    LogInfo("EntityFactory::CreateEntityFromData: Creating entity...");
  }
  entity::EntityRef entity = entity_manager->AllocateNewEntity();
  LoadEntityData(data, entity_manager, entity, false);
  return entity;
}

entity::EntityRef EntityFactory::CreateEntityFromPrototype(
    const char* prototype_name, entity::EntityManager* entity_manager) {
  if (prototype_requests_.find(prototype_name) == prototype_requests_.end()) {
    std::vector<uint8_t> prototype_request;
    if (CreatePrototypeRequest(prototype_name, &prototype_request)) {
      if (debug_entity_creation()) {
        LogInfo("EntityFactory: Created prototype request for '%s'",
                prototype_name);
      }
      prototype_requests_[prototype_name] = prototype_request;
    } else {
      LogError("EntityFactory::CreatePrototypeRequest(%s) failed",
               prototype_name);
      return entity::EntityRef();
    }
  }
  std::vector<entity::EntityRef> entities_loaded;
  LoadEntityListFromMemory(
      static_cast<const void*>(prototype_requests_[prototype_name].data()),
      entity_manager, &entities_loaded);
  if (entities_loaded.size() > 0)
    return entities_loaded[0];
  else
    return entity::EntityRef();
}

bool EntityFactory::SerializeEntity(
    entity::EntityRef& entity, entity::EntityManager* entity_manager,
    std::vector<uint8_t>* entity_serialized_output) {
  auto editor_component = entity_manager->GetComponent<EditorComponent>();

  std::vector<entity::ComponentInterface::RawDataUniquePtr> exported_data;
  std::vector<const void*> exported_pointers;
  std::vector<flatbuffers::Offset<ComponentDefInstance>> component_vector;

  exported_pointers.resize(max_component_id() + 1, nullptr);
  for (int component_id = 0; component_id <= max_component_id();
       component_id++) {
    const EditorData* editor_data = editor_component->GetComponentData(entity);
    if (editor_data->components_from_prototype.find(component_id) ==
        editor_data->components_from_prototype.end()) {
      auto component = entity_manager->GetComponent(component_id);
      if (component != nullptr) {
        exported_data.push_back(component->ExportRawData(entity));
        exported_pointers[component_id] = exported_data.back().get();
      }
    }
  }

  return CreateEntityDefinition(exported_pointers, entity_serialized_output);
}

bool EntityFactory::SerializeEntityList(
    const std::vector<std::vector<uint8_t>>& entity_definitions,
    std::vector<uint8_t>* entity_list_serialized) {
  std::vector<const void*> entity_definition_pointers;
  entity_definition_pointers.reserve(entity_definitions.size());
  for (size_t i = 0; i < entity_definitions.size(); i++) {
    entity_definition_pointers.push_back(entity_definitions[i].data());
  }
  return CreateEntityList(entity_definition_pointers, entity_list_serialized);
}

void EntityFactory::SetComponentType(entity::ComponentId component_id,
                                     unsigned int data_type,
                                     const char* table_name) {
  if (data_type_to_component_id_.size() <= data_type)
    data_type_to_component_id_.resize(data_type + 1, entity::kInvalidComponent);
  data_type_to_component_id_[data_type] = component_id;

  if (component_id_to_data_type_.size() <= component_id)
    component_id_to_data_type_.resize(component_id + 1, kDataTypeNone);
  component_id_to_data_type_[component_id] = data_type;

  if (component_id_to_table_name_.size() <= component_id)
    component_id_to_table_name_.resize(component_id + 1, "");
  component_id_to_table_name_[component_id] = table_name;

  if (component_id > max_component_id_) max_component_id_ = component_id;

  LogInfo("EntityFactory: ComponentID %d = DataType %d = %s", component_id,
          data_type, table_name);
}

void EntityFactory::SetFlatbufferSchema(const char* binary_schema_filename) {
  if (!LoadFile(binary_schema_filename, &flatbuffer_binary_schema_data_)) {
    LogInfo(
        "EntityFactory::SetFlatbufferSchema: Can't load binary schema file %s",
        binary_schema_filename);
  }
}

}  // namespace fpl
