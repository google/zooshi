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

#include <map>
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

  void set_debug(bool b) { debug_ = b; }

 private:
  void InstantiateEntity(const EntityDef* def,
                         entity::EntityManager* entity_manager,
                         entity::EntityRef& entity, bool is_prototype);
  std::string entity_library_data_;
  const EntityListDef* entity_library_;
  std::map<std::string, const EntityDef*> prototype_data_;
  std::map<std::string, std::vector<uint8_t>> prototype_requests_;
  bool debug_;
};

}  // namespace fpl_project
}  // namespace fpl

#endif  // ZOOSHI_ENTITY_FACTORY_H_
