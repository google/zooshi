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

#ifndef COMPONENTS_RIVER_H
#define COMPONENTS_RIVER_H

#include "components_generated.h"
#include "entity/component.h"
#include "fplbase/mesh.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"
#include "rail_denizen.h"

namespace fpl {
namespace fpl_project {

// All the relevent data for rivers ends up tossed into other components.
// (Mostly rendermesh at the moment.)  This will probably be less empty
// once the river gets more animated.
struct RiverData {
  entity::EntityRef bank;
};

class RiverComponent : public entity::Component<RiverData> {
 public:
  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(entity::EntityRef& entity) const;

  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime /*delta_time*/) {}

 private:
  void CreateRiverMesh(entity::EntityRef& entity);
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RiverComponent,
                              fpl::fpl_project::RiverData)

#endif  // COMPONENTS_RIVER_H
