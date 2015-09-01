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
#include "component_library/meta.h"
#include "fplbase/utilities.h"
#include "flatbuffers/reflection.h"

namespace fpl {
namespace fpl_project {

using component_library::MetaComponent;

bool ZooshiEntityFactory::ReadEntityList(
    const void* entity_list, std::vector<const void*>* entity_defs) {
  const EntityListDef* list = flatbuffers::GetRoot<EntityListDef>(entity_list);
  entity_defs->clear();
  entity_defs->reserve(list->entity_list()->size());
  for (size_t i = 0; i < list->entity_list()->size(); i++) {
    entity_defs->push_back(list->entity_list()->Get(i));
  }
  return true;
}

bool ZooshiEntityFactory::ReadEntityDefinition(
    const void* entity_definition, std::vector<const void*>* component_defs) {
  const EntityDef* def = static_cast<const EntityDef*>(entity_definition);
  component_defs->clear();
  component_defs->resize(max_component_id() + 1, nullptr);
  for (size_t i = 0; i < def->component_list()->size(); i++) {
    const ComponentDefInstance* component_def = def->component_list()->Get(i);
    entity::ComponentId component_id =
        DataTypeToComponentId(component_def->data_type());
    if (component_id == entity::kInvalidComponent) {
      LogError("ReadEntityDefinition: Error, unknown component data type %d\n",
               component_def->data_type());
      return false;
    }
    (*component_defs)[component_id] = component_def->data();
  }
  return true;
}

bool ZooshiEntityFactory::CreatePrototypeRequest(
    const char* prototype_name, std::vector<uint8_t>* request) {
  // Create the prototype request for the first time. It will be cached
  // for future use.
  flatbuffers::FlatBufferBuilder fbb;
  auto prototype = fbb.CreateString(prototype_name);
  MetaDefBuilder builder(fbb);
  builder.add_prototype(prototype);
  auto component = CreateComponentDefInstance(
      fbb, ComponentDataUnion(
               ComponentIdToDataType(MetaComponent::GetComponentId())),
      builder.Finish().Union());
  std::vector<flatbuffers::Offset<ComponentDefInstance>> component_vec;
  component_vec.push_back(component);
  auto entity_def = CreateEntityDef(fbb, fbb.CreateVector(component_vec));
  std::vector<flatbuffers::Offset<EntityDef>> entity_vec;
  entity_vec.push_back(entity_def);
  auto entity_list = CreateEntityListDef(fbb, fbb.CreateVector(entity_vec));
  fbb.Finish(entity_list);
  *request = std::vector<uint8_t>(fbb.GetBufferPointer(),
                                  fbb.GetBufferPointer() + fbb.GetSize());

  return true;
}

bool ZooshiEntityFactory::CreateEntityDefinition(
    const std::vector<const void*>& component_data,
    std::vector<uint8_t>* entity_definition) {
  // Take an individual component data type such as TransformData, and wrap it
  // inside the union in ComponentDefInstance.

  // For each component, we only have a raw pointer to the flatbuffer data as
  // exported by that component. We use our knowledge of the data types for each
  // component ID to get the table type to copy, and to set the union data type
  // to the correct value.
  if (flatbuffer_binary_schema_data() == "") {
    LogError("CreateEntityDefinition: No schema loaded, can't CopyTable");
    return false;
  }
  auto schema = reflection::GetSchema(flatbuffer_binary_schema_data().c_str());
  if (schema == nullptr) {
    LogError(
        "CreateEntityDefinition: GetSchema() failed, is it a binary schema?");
    return false;
  }

  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<ComponentDefInstance>> component_list;
  for (size_t i = 0; i <= max_component_id(); i++) {
    // For each non-null component ID, create a ComponentDefInstance and copy
    // the data for that component in, using reflection.
    if (component_data[i] != nullptr) {
      const uint8_t* raw_data = static_cast<const uint8_t*>(component_data[i]);
      ComponentDataUnion data_type =
          static_cast<ComponentDataUnion>(ComponentIdToDataType(i));
      const char* table_name = ComponentIdToTableName(i);
      auto table_def = schema->objects()->LookupByKey(table_name);
      if (table_def != nullptr) {
        component_list.push_back(CreateComponentDefInstance(
            fbb, data_type,
            flatbuffers::CopyTable(fbb, *schema, *table_def,
                                   *flatbuffers::GetAnyRoot(raw_data))
                .o));
      } else {
        LogError(
            "CreateEntityDefinition: Unknown table for component %d with data "
            "type %d: '%s'",
            i, data_type, table_name);
      }
    }
  }
  auto entity_def = CreateEntityDef(fbb, fbb.CreateVector(component_list));
  fbb.Finish(entity_def);
  *entity_definition = std::vector<uint8_t>(
      fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize());
  return true;
}

bool ZooshiEntityFactory::CreateEntityList(
    const std::vector<const void*>& entity_defs,
    std::vector<uint8_t>* entity_list_output) {
  // Given a collection of EntityDef flatbuffers, put them all into a single
  // EntityListDef. Similar to CreateEntityDefinition, this function uses
  // flatbuffers deep copy to put the entity definitions in the new flatbuffer.
  if (flatbuffer_binary_schema_data() == "") {
    LogError("CreateEntityList: No schema loaded, can't CopyTable");
    return false;
  }
  auto schema = reflection::GetSchema(flatbuffer_binary_schema_data().c_str());
  if (schema == nullptr) {
    LogError("CreateEntityList: GetSchema() failed, is it a binary schema?");
    return false;
  }
  auto table_def = schema->objects()->LookupByKey("EntityDef");
  if (table_def == nullptr) {
    LogError("CreateEntityList: Can't look up EntityDef");
    return false;
  }
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<EntityDef>> entity_list;
  for (auto entity_def = entity_defs.begin(); entity_def != entity_defs.end();
       ++entity_def) {
    const uint8_t* raw_data = static_cast<const uint8_t*>(*entity_def);
    entity_list.push_back(
        flatbuffers::CopyTable(fbb, *schema, *table_def,
                               *flatbuffers::GetAnyRoot(raw_data))
            .o);
  }

  auto entity_list_def =
      CreateEntityListDef(fbb, fbb.CreateVector(entity_list));
  fbb.Finish(entity_list_def);
  *entity_list_output = std::vector<uint8_t>(
      fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize());
  return true;
}

}  // namespace fpl_project
}  // namespace fpl
