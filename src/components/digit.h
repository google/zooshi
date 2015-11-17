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
#include "corgi/component.h"
#include "corgi/entity_manager.h"
#include "fplbase/mesh.h"
#include "fplbase/shader.h"

namespace fpl {
namespace zooshi {

struct DigitData {
  AttributeDef attribute;
  fplbase::Shader* shader;
  fplbase::Mesh* digits[10];
  int divisor;
};

class DigitComponent : public corgi::Component<DigitData> {
 public:
  virtual ~DigitComponent() {}

  virtual void Init() {}
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* raw_data);
  // Currently only exists in prototypes, so no ExportRawData is needed
  virtual RawDataUniquePtr ExportRawData(
      const corgi::EntityRef& /*entity*/) const {
    return nullptr;
  }
  virtual void InitEntity(corgi::EntityRef& entity);
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::DigitComponent, fpl::zooshi::DigitData)

#endif  // FPL_ZOOSHI_COMPONENTS_SIGN_H_
