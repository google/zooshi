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

#include "admob.h"
#include "components/attributes.h"
#include "components/audio_listener.h"
#include "components/lap_dependent.h"
#include "components/light.h"
#include "components/patron.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/rail_node.h"
#include "components/render_3d_text.h"
#include "components/river.h"
#include "components/scenery.h"
#include "components/services.h"
#include "components/shadow_controller.h"
#include "components/simple_movement.h"
#include "components/sound.h"
#include "components/time_limit.h"
#include "components_generated.h"
#include "corgi/entity_manager.h"
#include "corgi_component_library/animation.h"
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/entity_factory.h"
#include "corgi_component_library/graph.h"
#include "corgi_component_library/meta.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/rendermesh.h"
#include "corgi_component_library/transform.h"

#include "mathfu/internal/disable_warnings_begin.h"

#include "firebase/app.h"

#include "mathfu/internal/disable_warnings_end.h"

#include "fplbase/render_target.h"
#include "fplbase/renderer.h"
#include "inputcontrollers/base_player_controller.h"
#include "inputcontrollers/gamepad_controller.h"
#include "inputcontrollers/onscreen_controller.h"
#include "invites.h"
#include "messaging.h"
#include "railmanager.h"
#include "scene_lab/corgi/corgi_adapter.h"
#include "scene_lab/corgi/edit_options.h"
#include "scene_lab/scene_lab.h"
#include "unlockable_manager.h"
#include "world_renderer.h"
#include "xp_system.h"

namespace pindrop {

class AudioEngine;

}  // namespace pindrop

namespace fpl {

class InputSystem;
class AssetManager;

namespace zooshi {

// The #defines that can be applied to a shader.
enum ShaderDefines {
  kPhongShading,
  kSpecularEffect,
  kShadowEffect,
  kNormalMaps,
  kNumShaderDefines
};

class WorldRenderer;
struct Config;

struct World {
 public:
  World()
      : draw_debug_physics(false),
        skip_rendermesh_rendering(false),
        is_single_stepping(false),
        sushi_index(0),
        level_index(0),
        is_in_cardboard_(false) {
#if FPLBASE_ANDROID_VR
    hmd_controller = nullptr;
    onscreen_controller = nullptr;
#endif  // FPLBASE_ANDROID_VR
  }

  void Initialize(const Config& config, fplbase::InputSystem* input_system,
                  fplbase::AssetManager* asset_mgr,
                  WorldRenderer* worldrenderer,
                  flatui::FontManager* font_manager,
                  pindrop::AudioEngine* audio_engine,
                  breadboard::GraphFactory* graph_factory,
                  fplbase::Renderer* renderer, scene_lab::SceneLab* scene_lab,
                  UnlockableManager* unlockable_mgr, XpSystem* xp_system,
                  InvitesListener* invites_lstr, MessageListener* message_lstr,
                  AdMobHelper* admob_hlpr);

  // Entity manager
  corgi::EntityManager entity_manager;

  // Entity factory, for creating entities from data.
  std::unique_ptr<corgi::component_library::EntityFactory> entity_factory;

  // Rail Manager - manages loading and storing of rail definitions
  RailManager rail_manager;

  // Components
  corgi::component_library::TransformComponent transform_component;
  corgi::component_library::AnimationComponent animation_component;
  RailDenizenComponent rail_denizen_component;
  PlayerComponent player_component;
  PlayerProjectileComponent player_projectile_component;
  corgi::component_library::RenderMeshComponent render_mesh_component;
  corgi::component_library::PhysicsComponent physics_component;
  PatronComponent patron_component;
  TimeLimitComponent time_limit_component;
  AudioListenerComponent audio_listener_component;
  SoundComponent sound_component;
  AttributesComponent attributes_component;
  RiverComponent river_component;
  RailNodeComponent rail_node_component;
  SceneryComponent scenery_component;
  ServicesComponent services_component;
  LightComponent light_component;
  corgi::component_library::CommonServicesComponent common_services_component;
  ShadowControllerComponent shadow_controller_component;
  corgi::component_library::MetaComponent meta_component;
  scene_lab_corgi::EditOptionsComponent edit_options_component;
  SimpleMovementComponent simple_movement_component;
  LapDependentComponent lap_dependent_component;
  corgi::component_library::GraphComponent graph_component;
  Render3dTextComponent render_3d_text_component;

  // Each player has direct control over one entity.
  corgi::EntityRef active_player_entity;

  const Config* config;

  fplbase::AssetManager* asset_manager;
  WorldRenderer* world_renderer;

  UnlockableManager* unlockables;
  XpSystem* xp_system;

  std::vector<std::unique_ptr<BasePlayerController>> input_controllers;
  OnscreenControllerUI onscreen_controller_ui;
#if FPLBASE_ANDROID_VR
  BasePlayerController* hmd_controller;
#endif  // FPLBASE_ANDROID_VR
  BasePlayerController* onscreen_controller;

  firebase::App* firebase_app;
  InvitesListener* invites_listener;
  MessageListener* message_listener;
  AdMobHelper* admob_helper;

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

  fplbase::Material* cardboard_settings_gear;

  void AddController(BasePlayerController* controller);
  void SetActiveController(ControllerType controller_type);
  // Reset all controllers back to the default facing values.
  void ResetControllerFacing();

  bool is_in_cardboard() const { return is_in_cardboard_; }
  void SetIsInCardboard(bool in_cardboard);

  void SetRenderingOption(ShaderDefines s, bool enable_option);
  void SetRenderingOptionCardboard(ShaderDefines s, bool enable_option);
  bool RenderingOptionEnabled(ShaderDefines s);
  bool RenderingOptionEnabledCardboard(ShaderDefines s);
  bool RenderingOptionsDirty();

  void ResetRenderingDirty();

  void SetHmdControllerEnabled(bool enabled) {
#if FPLBASE_ANDROID_VR
    // hmd_controller can be NULL if the device does not support a head mounted
    // display like Cardboard (e.g lacks a gyro), onscreen_controller can be
    // NULL if support is compiled out.
    if (hmd_controller && onscreen_controller) {
      hmd_controller->set_enabled(enabled);
      onscreen_controller->set_enabled(!enabled);
      SetActiveController(kControllerDefault);
    }
#else
    (void)enabled;
#endif  // FPLBASE_ANDROID_VR
  }

  bool GetHmdControllerEnabled() const {
#if FPLBASE_ANDROID_VR
    return hmd_controller && hmd_controller->enabled();
#else
    return false;
#endif  // FPLBASE_ANDROID_VR
  }

  // Get the sushi config that should be used during gameplay.
  const UnlockableConfig* SelectedSushi() const {
    if (sushi_index >= config->sushi_config()->size()) {
      return config->sushi_config()->Get(0);
    } else {
      return config->sushi_config()->Get(
          static_cast<flatbuffers::uoffset_t>(sushi_index));
    }
  }

  // Get the currently selected level that should be used.
  const LevelDef* CurrentLevel() const {
    if (level_index >= config->world_def()->levels()->size()) {
      return config->world_def()->levels()->Get(0);
    } else {
      return config->world_def()->levels()->Get(
          static_cast<flatbuffers::uoffset_t>(level_index));
    }
  }

  bool is_single_stepping;

  // Records the start time of gameplay, used for analytics.
  double gameplay_start_time;

  // The index of the sushi to use out of the config.
  size_t sushi_index;

  // The index of the level layout to use.
  size_t level_index;

 private:
  // Determines if the game is in Cardboard mode (for special rendering).
  bool is_in_cardboard_;

  // Determine if shadows should be turned on.
  bool render_shadows_;

  // Determine if Phong shading should be turned on.
  bool apply_phong_;

  // Determine if specular effect should be turned on.
  bool apply_specular_;

  // Determine if shadows should be turned on in Cardboard mode.
  bool render_shadows_cardboard_;

  // Determine if Phong shading should be turned on in Cardboard mode.
  bool apply_phong_cardboard_;

  // Determine if specular effect should be turned on in Cardboard mode.
  bool apply_specular_cardboard_;

  // Whether any rendering option has been modified since last draw call.
  bool rendering_dirty_;
};

// Removes all entities from the world, then repopulates it based on the entity
// definitions given in the WorldDef. The input controller is required to hook
// up the player's controller to the player entity.
void LoadWorldDef(World* world, const WorldDef* world_def);

}  // zooshi
}  // fpl

#endif  // ZOOSHI_WORLD_H_
