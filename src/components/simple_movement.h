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

#ifndef COMPONENTS_SIMPLE_MOVEMENT_H_
#define COMPONENTS_SIMPLE_MOVEMENT_H_

#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct SimpleMovementData {
  SimpleMovementData() : velocity(mathfu::kZeros3f) {}

  mathfu::vec3 velocity;
};

class SimpleMovementComponent : public entity::Component<SimpleMovementData> {
 public:
  SimpleMovementComponent() {}

  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;

  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::SimpleMovementComponent,
                              fpl::fpl_project::SimpleMovementData)

#endif  // COMPONENTS_SIMPLE_MOVEMENT_H_
