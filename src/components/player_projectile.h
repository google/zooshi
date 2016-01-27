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

#ifndef FPL_ZOOSHI_COMPONENTS_PLAYER_PROJECTILE_H_
#define FPL_ZOOSHI_COMPONENTS_PLAYER_PROJECTILE_H_

#include <string>

#include "components_generated.h"
#include "corgi/component.h"
#include "corgi_component_library/graph.h"
#include "fplbase/utilities.h"
#include "pindrop/pindrop.h"

using corgi::component_library::SerializableGraphState;
namespace fpl {
namespace zooshi {

static const corgi::WorldTime kMaxProjectileDuration = 3000;

// Data for scene object components.
struct PlayerProjectileData {
  corgi::EntityRef owner;  // The player that "owns" this projectile.

  // The graph that may trigger when colliding with another entity.
  std::map<std::string, SerializableGraphState> on_collision;
};

class PlayerProjectileComponent
    : public corgi::Component<PlayerProjectileData> {
 public:
  virtual ~PlayerProjectileComponent() {}

  virtual void InitEntity(corgi::EntityRef& /*entity*/) {}
  virtual void CleanupEntity(corgi::EntityRef& /*entity*/) {}

  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(corgi::WorldTime /*delta_time*/) {}
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::PlayerProjectileComponent,
                         fpl::zooshi::PlayerProjectileData)

#endif  // FPL_ZOOSHI_COMPONENTS_PLAYER_PROJECTILE_H_
