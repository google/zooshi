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

#ifndef COMPONENTS_PLAYER_PROJECTILE_H_
#define COMPONENTS_PLAYER_PROJECTILE_H_

#include <string>

#include "component_library/graph.h"
#include "components_generated.h"
#include "entity/component.h"
#include "fplbase/utilities.h"
#include "pindrop/pindrop.h"

using fpl::component_library::SerializableGraphState;
namespace fpl {
namespace fpl_project {

static const WorldTime kMaxProjectileDuration = 3 * kMillisecondsPerSecond;

// Data for scene object components.
struct PlayerProjectileData {
  entity::EntityRef owner;  // The player that "owns" this projectile.

  // The graph that may trigger when colliding with another entity.
  std::map<std::string, SerializableGraphState> on_collision;
};

class PlayerProjectileComponent
    : public entity::Component<PlayerProjectileData> {
 public:
  virtual void InitEntity(entity::EntityRef& /*entity*/) {}
  virtual void CleanupEntity(entity::EntityRef& /*entity*/) {}

  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime /*delta_time*/) {}
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PlayerProjectileComponent,
                              fpl::fpl_project::PlayerProjectileData)

#endif  // COMPONENTS_PLAYER_PROJECTILE_H_
