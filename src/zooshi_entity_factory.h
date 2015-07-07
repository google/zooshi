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

#ifndef ZOOSHI_ENTITY_FACTORY_H_
#define ZOOSHI_ENTITY_FACTORY_H_

#include <unordered_map>
#include <string>
#include "components_generated.h"
#include "entity/entity_manager.h"
#include "flatbuffers/flatbuffers.h"

namespace fpl {
namespace fpl_project {

class ZooshiEntityFactory : public entity::EntityFactoryInterface {
 public:
  ZooshiEntityFactory() : entity_library_(nullptr), debug_(false) {}
  bool SetEntityLibrary(const char* entity_library_file);
  virtual entity::EntityRef CreateEntityFromData(
      const void* data, entity::EntityManager* entity_manager);
  entity::EntityRef CreateEntityFromPrototype(
      const char* prototype_name, entity::EntityManager* entity_manager);

  // When you register each component to the entity system, it will get
  // a component ID. This factory needs to know the component ID assigned
  // for each component data type (the data_type() in the flatbuffer union).
  void SetComponentId(unsigned int data_type, entity::ComponentId component_id);

  // Get the component for a given data type specifier.
  entity::ComponentId DataTypeToComponentId(unsigned int data_type) {
    if (data_type >= data_type_to_component_id_.size())
      return entity::kInvalidComponent;
    return data_type_to_component_id_[data_type];
  }
  // Get the data type specifier for a given component.
  unsigned int ComponentIdToDataType(entity::ComponentId component_id) {
    if (component_id >= component_id_to_data_type_.size())
      return ComponentDataUnion_NONE;
    return component_id_to_data_type_[component_id];
  }

  void set_debug(bool b) { debug_ = b; }

 private:
  void InstantiateEntity(const EntityDef* def,
                         entity::EntityManager* entity_manager,
                         entity::EntityRef& entity, bool is_prototype);
  std::string entity_library_data_;
  const EntityListDef* entity_library_;
  std::unordered_map<std::string, const EntityDef*> prototype_data_;
  std::unordered_map<std::string, std::vector<uint8_t>> prototype_requests_;
  // Look up ComponentId from data type, and vice versa.
  std::vector<unsigned int> data_type_to_component_id_;
  std::vector<entity::ComponentId> component_id_to_data_type_;
  bool debug_;
};

}  // namespace fpl_project
}  // namespace fpl

#endif  // ZOOSHI_ENTITY_FACTORY_H_
