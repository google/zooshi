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

#ifndef COMPONENTS_PATRON_H_
#define COMPONENTS_PATRON_H_

#include "config_generated.h"
#include "components_generated.h"
#include "entity/entity_manager.h"
#include "entity/component.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/constants.h"

namespace fpl {
namespace event {

class EventManager;

}  // event

namespace fpl_project {

// Data for scene object components.
struct PatronData {
  PatronData() : fallen(false), at_rest(false) {}
  bool fallen;
  bool at_rest;
  // misc data for simulating the fall:
  mathfu::quat original_orientation;
  mathfu::quat falling_rotation;
  float y;
  float dy;
};

class PatronComponent : public entity::Component<PatronData> {
 public:
  PatronComponent() {}

  void Initialize(const Config* config, event::EventManager* event_manager);

  virtual void AddFromRawData(entity::EntityRef& parent, const void* raw_data);
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

 private:
  void SpawnSplatter(const mathfu::vec3& position, int count);

  const Config* config_;
  event::EventManager* event_manager_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PatronComponent,
                              fpl::fpl_project::PatronData,
                              ComponentDataUnion_PatronDef)

#endif  // COMPONENTS_PATRON_H_
