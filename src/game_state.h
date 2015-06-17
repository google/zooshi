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

#include "components/attributes.h"
#include "components/audio_listener.h"
#include "components/patron.h"
#include "components/physics.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/rendermesh.h"
#include "components/river.h"
#include "components/sound.h"
#include "components/services.h"
#include "components/time_limit.h"
#include "components/transform.h"
#include "editor/world_editor.h"
#include "entity/entity_manager.h"
#include "fplbase/material_manager.h"
#include "fplbase/utilities.h"
#include "motive/engine.h"
#include "railmanager.h"

#ifdef USING_GOOGLE_PLAY_GAMES
#include "gpg_manager.h"
#include "gpg_multiplayer.h"
#endif

#ifdef __ANDROID__
#include "inputcontrollers/android_cardboard_controller.h"
#else
#include "inputcontrollers/mouse_controller.h"
#endif

namespace pindrop {

class AudioEngine;

}  // namespace pindrop

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

class GameState : event::EventListener {
 public:
  GameState();

  // TODO: We need to remove Mesh and Shader here, and replace them
  // with a reference to meshrenderer, and load things from that.
  // (rather than passing in the shader and mesh)
  void Initialize(const vec2i& window_size, const Config& config,
                  const InputConfig& input_config, InputSystem* input_system,
                  MaterialManager* material_manager, FontManager* font_manager,
                  pindrop::AudioEngine* audio_engine);

  void Render(Renderer* renderer);

  void UpdateMainCamera();
  void Update(WorldTime delta_time);

  virtual void OnEvent(const event::EventPayload& event_payload);

  bool is_in_cardboard() const { return is_in_cardboard_; }
  void set_is_in_cardboard(bool b) { is_in_cardboard_ = b; }

#ifdef USING_GOOGLE_PLAY_GAMES
  void set_gpg_manager(GPGManager* gpg_manager) { gpg_manager_ = gpg_manager; }
  void set_gpg_multiplayer(GPGMultiplayer* gpg_multiplayer) {
    gpg_multiplayer_ = gpg_multiplayer;
  }
#endif

 private:
  void RenderForCardboard(Renderer* renderer);

  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif

  pindrop::AudioEngine* audio_engine_;

  // Game-wide event manager.
  event::EventManager event_manager_;

  // Entity manager.
  entity::EntityManager entity_manager_;

  // Entity factory, for creating entities from data.
  ZooshiEntityFactory entity_factory_;

  // Rail Manager - manages loading and storing of rail definitions
  RailManager rail_manager_;

  // Components
  motive::MotiveEngine motive_engine_;
  TransformComponent transform_component_;
  RailDenizenComponent rail_denizen_component_;
  PlayerComponent player_component_;
  PlayerProjectileComponent player_projectile_component_;
  RenderMeshComponent render_mesh_component_;
  PhysicsComponent physics_component_;
  PatronComponent patron_component_;
  TimeLimitComponent time_limit_component_;
  AudioListenerComponent audio_listener_component_;
  SoundComponent sound_component_;
  AttributesComponent attributes_component_;
  RiverComponent river_component_;

  ServicesComponent services_component_;

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
  MaterialManager* material_manager_;

  std::unique_ptr<editor::WorldEditor> world_editor_;

  // Determines if the game is in Cardboard mode (for special rendering)
  bool is_in_cardboard_;

  // Determines if the debug drawing of physics should be used
  bool draw_debug_physics_;

#ifdef USING_GOOGLE_PLAY_GAMES
  GPGManager* gpg_manager;

  // Network multiplayer library for multi-screen version
  GPGMultiplayer* gpg_multiplayer_;
#endif
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_STATE_H_
