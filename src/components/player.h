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
#include "inputcontrollers/base_player_controller.h"
#include "config_generated.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace event {

class EventManager;

}  // event

namespace fpl_project {

struct ActionDef;

class PlayerData {
 public:
  PlayerData() : input_controller_(nullptr) {}

  mathfu::vec3 GetFacing() const { return input_controller_->facing().Value(); }
  mathfu::vec3 GetUp() const { return input_controller_->up().Value(); }

  fpl_project::BasePlayerController* input_controller() const {
    return input_controller_;
  }
  void set_input_controller(
      fpl_project::BasePlayerController* input_controller) {
    input_controller_ = input_controller;
  }

  const ActionDef* on_fire() { return on_fire_; }
  void set_on_fire(const ActionDef* on_fire) { on_fire_ = on_fire; }

 private:
  const ActionDef* on_fire_;
  fpl_project::BasePlayerController* input_controller_;
};

class PlayerComponent : public entity::Component<PlayerData> {
 public:
  PlayerComponent() : event_manager_(nullptr) {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  entity::EntityRef SpawnProjectile(entity::EntityRef source);

 private:
  const Config* config_;
  event::EventManager* event_manager_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PlayerComponent,
                              fpl::fpl_project::PlayerData,
                              ComponentDataUnion_PlayerDef)

#endif  // COMPONENTS_PLAYER_H_
