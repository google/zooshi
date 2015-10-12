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

#include <set>

#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "breadboard/event.h"
#include "inputcontrollers/base_player_controller.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace zooshi {

BREADBOARD_DECLARE_EVENT(kOnFireEventId)

struct ActionDef;

class PlayerData {
 public:
  PlayerData() : input_controller_(nullptr) {}

  mathfu::vec3 GetFacing() const { return input_controller_->facing().Value(); }
  mathfu::vec3 GetUp() const { return input_controller_->up().Value(); }

  BasePlayerController* input_controller() const { return input_controller_; }
  void set_input_controller(BasePlayerController* input_controller) {
    input_controller_ = input_controller;
  }

  std::set<std::string>& get_patrons_feed_status() {
    return patrons_feed_status_;
  }

 private:
  BasePlayerController* input_controller_;
  std::set<std::string> patrons_feed_status_;
};

class PlayerComponent : public entity::Component<PlayerData> {
 public:
  PlayerComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;

  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  entity::EntityRef SpawnProjectile(entity::EntityRef source);
  mathfu::vec3 CalculateProjectileDirection(entity::EntityRef source) const;

  void set_active(bool active) { active_ = active; }

 private:
  mathfu::vec3 RandomProjectileAngularVelocity() const;

  const Config* config_;
  bool active_;
};

}  // zooshi
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::zooshi::PlayerComponent,
                              fpl::zooshi::PlayerData)

#endif  // COMPONENTS_PLAYER_H_
