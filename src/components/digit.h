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

#ifndef FPL_ZOOSHI_COMPONENTS_SIGN_H_
#define FPL_ZOOSHI_COMPONENTS_SIGN_H_

#include "attributes_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "entity/entity_manager.h"

namespace fpl {

class Mesh;
class Shader;

namespace fpl_project {

struct DigitData {
  AttributeDef attribute;
  Shader* shader;
  Mesh* digits[10];
  int divisor;
};

class DigitComponent : public entity::Component<DigitData> {
 public:
  virtual void Init() {}
  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  // Currently only exists in prototypes, so no ExportRawData is needed
  virtual RawDataUniquePtr ExportRawData(entity::EntityRef& /*entity*/) const {
    return nullptr;
  }
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::DigitComponent,
                              fpl::fpl_project::DigitData)

#endif  // FPL_ZOOSHI_COMPONENTS_SIGN_H_

