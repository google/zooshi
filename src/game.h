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

#ifndef ZOOSHI_GAME_H
#define ZOOSHI_GAME_H

#include "camera.h"
#include "config_generated.h"
#include "entity/entity_manager.h"
#include "event/event_system.h"
#include "event/graph.h"
#include "flatbuffers/flatbuffers.h"
#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/renderer.h"
#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"
#include "rail_def_generated.h"
#include "states/intro_state.h"
#include "states/gameplay_state.h"
#include "states/pause_state.h"
#include "states/state_machine.h"
#include "states/states.h"
#include "states/world_editor_state.h"
#include "world.h"
#include "zooshi_graph_factory.h"

#if defined(PLATFORM_MOBILE)
#define USING_GOOGLE_PLAY_GAMES
#endif

// Header files that has a dependency to GPG definitions.
#include "gpg_manager.h"
#include "states/game_menu_state.h"

#ifdef __ANDROID__
#include "inputcontrollers/android_cardboard_controller.h"
#else
#include "inputcontrollers/mouse_controller.h"
#endif

namespace fpl {
namespace fpl_project {

struct Config;
struct InputConfig;
struct AssetManifest;

class Game {
 public:
  Game();
  bool Initialize(const char* const binary_directory);
  void Run();

  // TODO(ccornell): Replace with SDL threads.
  static pthread_mutex_t renderthread_mutex_;
  static pthread_mutex_t updatethread_mutex_;
  static pthread_mutex_t gameupdate_mutex_;
  static pthread_cond_t start_render_cv_;
  static pthread_cond_t start_update_cv_;

 private:
  bool InitializeRenderer();
  bool InitializeAssets();
  void InitializeEventSystem();

  Mesh* CreateVerticalQuadMesh(const char* material_name,
                               const mathfu::vec3& offset,
                               const mathfu::vec2& pixel_bounds,
                               float pixel_to_world_scale);
  void Update(WorldTime delta_time);
  void UpdateMainCamera();
  void UpdateMainCameraAndroid();
  void UpdateMainCameraMouse();
  const Config& GetConfig() const;
  const InputConfig& GetInputConfig() const;
  const RailDef& GetRailDef() const;
  const AssetManifest& GetAssetManifest() const;
  Mesh* GetCardboardFront(int renderable_id);

  void SetRelativeMouseMode(bool relative_mouse_mode);
  void ToggleRelativeMouseMode();

  // Hold configuration binary data.
  std::string config_source_;

  // Hold the configuration for the input system data.
  std::string input_config_source_;

  // Hold the configuration for the asset manifest source.
  std::string asset_manifest_source_;

#ifdef __ANDROID__
  // Input controller for looking around when using accelerometers on android.
  AndroidCardboardController input_controller_;
#else
  // Input controller for looking around when using mouse.
  MouseController input_controller_;
#endif

  // The top level state machine that drives the game.
  StateMachine<kGameStateCount> state_machine_;
  PauseState pause_state_;
  GameplayState gameplay_state_;
  GameMenuState game_menu_state_;
  IntroState intro_state_;
  WorldEditorState world_editor_state_;

  // Report touches, button presses, keyboard presses.
  InputSystem input_;

  // Hold rendering context.
  Renderer renderer_;

  // Load and own rendering resources.
  AssetManager asset_manager_;

  FontManager font_manager_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

  // The event system.
  event::EventSystem event_system_;
  ZooshiGraphFactory graph_factory_;

  // Shaders we use.
  Shader* shader_lit_textured_normal_;
  Shader* shader_textured_;

  // World time of previous update. We use this to calculate the delta_time of
  // the current update. This value is tied to the real-world clock.  Note that
  // it is distict from game_state_.time_, which is *not* tied to the real-world
  // clock. If the game is paused, game_state.time_ will pause, but
  // prev_world_time_ will keep chugging.
  WorldTime prev_render_time_;

  bool game_exiting_;

  std::string rail_source_;

  pindrop::AudioConfig* audio_config_;

  World world_;
  WorldRenderer world_renderer_;

  std::unique_ptr<editor::WorldEditor> world_editor_;

  bool relative_mouse_mode_;

  // String version number of the game.
  const char* version_;

  // Google Play Game Services Manager.
  GPGManager gpg_manager_;

};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_H
