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

#include <string>
#include <vector>
#include "components_generated.h"
#include "entity/component.h"
#include "fplbase/mesh.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"
#include "rail_denizen.h"

namespace fpl {
namespace zooshi {

// All the relevent data for rivers ends up tossed into other components.
// (Mostly rendermesh at the moment.)  This will probably be less empty
// once the river gets more animated.
struct RiverData {
  RiverData()
      : render_mesh_needs_update_(false),
        random_seed(static_cast<unsigned int>(rand())) {}
  std::vector<corgi::EntityRef> banks;
  std::string rail_name;
  // Flag for whether this river needs its meshes updated.
  bool render_mesh_needs_update_;
  // River generation has random elements, so we seed the random number
  // generator the same way every time we reload the river.
  unsigned int random_seed;
};

class RiverComponent : public corgi::Component<RiverData> {
 public:
  virtual ~RiverComponent() {}

  virtual void AddFromRawData(corgi::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;

  virtual void Init();
  virtual void UpdateAllEntities(corgi::WorldTime /*delta_time*/);

  void UpdateRiverMeshes(corgi::EntityRef entity);

  // Updates the meshes for the river.
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // IMPORTANT:  This will break if called from any thread other than
  // the main render thread.  Do not call from the update thread!
  void UpdateRiverMeshes();

  float river_offset() const { return river_offset_; }

 private:
  void TriggerRiverUpdate();
  void CreateRiverMesh(corgi::EntityRef& entity);
  float river_offset_;
};

}  // zooshi
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::zooshi::RiverComponent,
                              fpl::zooshi::RiverData)

#endif  // COMPONENTS_RIVER_H
