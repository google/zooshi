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

#ifndef ZOOSHI_WORLD_H_
#define ZOOSHI_WORLD_H_

#include <map>
#include <memory>
#include <string>

#include "component_library/animation.h"
#include "component_library/common_services.h"
#include "component_library/entity_factory.h"
#include "component_library/graph.h"
#include "component_library/meta.h"
#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "component_library/transform.h"
#include "components/attributes.h"
#include "components/audio_listener.h"
#include "components/digit.h"
#include "components/lap_dependent.h"
#include "components/patron.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/rail_node.h"
#include "components/river.h"
#include "components/scenery.h"
#include "components/services.h"
#include "components/shadow_controller.h"
#include "components/simple_movement.h"
#include "components/sound.h"
#include "components/time_limit.h"
#include "components_generated.h"
#include "entity/entity_manager.h"
#include "fplbase/render_target.h"
#include "fplbase/renderer.h"
#include "inputcontrollers/base_player_controller.h"
#include "inputcontrollers/gamepad_controller.h"
#include "railmanager.h"
#include "scene_lab/edit_options.h"
#include "scene_lab/scene_lab.h"
#include "world_renderer.h"

namespace pindrop {

class AudioEngine;

}  // namespace pindrop

namespace fpl {

class InputSystem;
class AssetManager;

namespace zooshi {

class WorldRenderer;
struct Config;

struct World {
 public:
  World()
      : draw_debug_physics(false),
        skip_rendermesh_rendering(false),
        is_in_cardboard_(false) {}

  void Initialize(const Config& config, InputSystem* input_system,
                  AssetManager* asset_mgr, WorldRenderer* worldrenderer,
                  FontManager* font_manager, pindrop::AudioEngine* audio_engine,
                  breadboard::GraphFactory* graph_factory, Renderer* renderer,
                  scene_lab::SceneLab* scene_lab);

  // Entity manager
  entity::EntityManager entity_manager;

  // Entity factory, for creating entities from data.
  std::unique_ptr<component_library::EntityFactory> entity_factory;

  // Rail Manager - manages loading and storing of rail definitions
  RailManager rail_manager;

  // Components
  component_library::TransformComponent transform_component;
  component_library::AnimationComponent animation_component;
  RailDenizenComponent rail_denizen_component;
  PlayerComponent player_component;
  PlayerProjectileComponent player_projectile_component;
  component_library::RenderMeshComponent render_mesh_component;
  component_library::PhysicsComponent physics_component;
  PatronComponent patron_component;
  TimeLimitComponent time_limit_component;
  AudioListenerComponent audio_listener_component;
  SoundComponent sound_component;
  AttributesComponent attributes_component;
  DigitComponent digit_component;
  RiverComponent river_component;
  RailNodeComponent rail_node_component;
  SceneryComponent scenery_component;
  ServicesComponent services_component;
  component_library::CommonServicesComponent common_services_component;
  ShadowControllerComponent shadow_controller_component;
  component_library::MetaComponent meta_component;
  scene_lab::EditOptionsComponent edit_options_component;
  SimpleMovementComponent simple_movement_component;
  LapDependentComponent lap_dependent_component;
  component_library::GraphComponent graph_component;

  // Each player has direct control over one entity.
  entity::EntityRef active_player_entity;

  const Config* config;

  AssetManager* asset_manager;
  WorldRenderer* world_renderer;

  std::vector<std::unique_ptr<BasePlayerController>> input_controllers;

  // TODO: Refactor all components so they don't require their source
  // data to remain in memory after their initial load. Then get rid of this,
  // which keeps all entity files loaded in memory.
  std::map<std::string, std::string> loaded_entity_files_;

  // Determines if the debug drawing of physics should be used.
  bool draw_debug_physics;

  // Used to skip rendering the render meshes, for debugging purposes.
  bool skip_rendermesh_rendering;

#ifdef USING_GOOGLE_PLAY_GAMES
  GPGManager* gpg_manager;

  // Network multiplayer library for multi-screen version
  GPGMultiplayer* gpg_multiplayer;
#endif

  void AddController(BasePlayerController* controller);
  void SetActiveController(ControllerType controller_type);
  // Reset all controllers back to the default facing values.
  void ResetControllerFacing();

  bool is_in_cardboard() const { return is_in_cardboard_; }
  void SetIsInCardboard(bool in_cardboard);

 private:
  // Determines if the game is in Cardboard mode (for special rendering).
  bool is_in_cardboard_;
};

// Removes all entities from the world, then repopulates it based on the entity
// definitions given in the WorldDef. The input controller is required to hook
// up the player's controller to the player entity.
void LoadWorldDef(World* world, const WorldDef* world_def);

}  // zooshi
}  // fpl

#endif  // ZOOSHI_WORLD_H_
