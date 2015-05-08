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

#ifndef ZOOSHI_GAME_STATE_H_
#define ZOOSHI_GAME_STATE_H_

#include "components/physics.h"
#include "components/family.h"
#include "components/player.h"
#include "components/rail_denizen.h"
#include "components/rendermesh.h"
#include "components/transform.h"
#include "entity/entity_manager.h"
#include "fplbase/utilities.h"
#include "motive/engine.h"

#ifdef __ANDROID__
#include "inputcontrollers/android_cardboard_controller.h"
#else
#include "inputcontrollers/mouse_controller.h"
#endif

namespace fpl {

class InputSystem;

namespace fpl_project {

struct InputConfig;
struct Config;

class ZooshiEntityFactory : public entity::EntityFactoryInterface {
 public:
  virtual entity::EntityRef CreateEntityFromData(
      const void* data, entity::EntityManager* entity_manager);
};

class GameState {
 public:
  GameState();

  // TODO: We need to remove Mesh and Shader here, and replace them
  // with a reference to meshrenderer, and load things from that.
  // (rather than passing in the shader and mesh)
  void Initialize(const vec2i& window_size, const Config& config,
                  const InputConfig& input_config,
                  InputSystem* input_system_,
                  Mesh** meshes, int num_meshes, Shader* shader);

  void Render(Renderer* renderer);

  void UpdateMainCamera();
  void Update(WorldTime delta_time);

 private:
  Camera main_camera_;

  // Entity manager
  entity::EntityManager entity_manager_;
  ZooshiEntityFactory entity_factory_;

  // Components
  motive::MotiveEngine motive_engine_;
  TransformComponent transform_component_;
  FamilyComponent family_component_;
  RailDenizenComponent rail_denizen_component_;
  PlayerComponent player_component_;
  RenderMeshComponent render_mesh_component_;
  PhysicsComponent physics_component_;

  // Input controller for mouse
#ifdef __ANDROID__
  AndroidCardboardController input_controller_;
#else
  MouseController input_controller_;
#endif

  // Each player has direct control over one entity.
  entity::EntityRef active_player_entity_;

  const Config* config_;
  const InputConfig* input_config_;

  InputSystem* input_system_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_STATE_H_

