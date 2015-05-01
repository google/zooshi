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

#ifndef COMPONENTS_PLAYER_H_
#define COMPONENTS_PLAYER_H_

#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {

class PlayerData {
 public:
  PlayerData() : facing(0, 0, 1, 0) {}

  // The direction of the Turret
  mathfu::quat facing;

 private:
};

class PlayerComponent : public entity::Component<PlayerData> {
 public:
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

 private:
};

}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::PlayerComponent, fpl::PlayerData,
                              fpl::ComponentDataUnion_PlayerDef)

#endif  // COMPONENTS_PLAYER_H_

