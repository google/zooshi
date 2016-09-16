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

#include <math.h>

#include "SDL_thread.h"
#include "breadboard/graph.h"
#include "breadboard/module_registry.h"
#include "camera.h"
#include "config_generated.h"
#include "corgi/entity_manager.h"
#include "flatbuffers/flatbuffers.h"
#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/renderer.h"
#include "fplbase/utilities.h"
#include "full_screen_fader.h"
#include "mathfu/glsl_mappings.h"
#include "module_library/default_graph_factory.h"
#include "pindrop/pindrop.h"
#include "rail_def_generated.h"
#include "states/intro_state.h"
#include "states/loading_state.h"
#include "states/pause_state.h"
#include "states/scene_lab_state.h"
#include "states/state_machine.h"
#include "states/states.h"
#include "world.h"

#if defined(PLATFORM_MOBILE)
#define USING_GOOGLE_PLAY_GAMES
#endif

#define DISPLAY_FRAMERATE_HISTOGRAM 0

#ifdef __ANDROID__
#define FPLBASE_ENABLE_SYSTRACE 0
#endif

// Header files that has a dependency to GPG definitions.
#include "corgi/entity_common.h"
#include "gpg_manager.h"
#include "states/game_menu_state.h"
#include "states/game_over_state.h"
#include "states/gameplay_state.h"

#ifdef __ANDROID__
#include "inputcontrollers/android_cardboard_controller.h"
#else
#include "inputcontrollers/mouse_controller.h"
#endif
#ifdef ANDROID_GAMEPAD
#include "inputcontrollers/gamepad_controller.h"
#endif

namespace fpl {
namespace zooshi {

const auto kGPGDefaultLeaderboard = "LeaderboardMain";

// On low RAM devices below the threshould value, the game applies a texture
// scaling to reduce a memory footprint.
const auto kLowRamProfileThreshold = 512;
const auto kLowRamDeviceTextureScale = mathfu::vec2(0.5f, 0.5f);

struct Config;
struct InputConfig;
struct AssetManifest;

// Mutexes/CVs used in synchronizing the render and update threads
struct GameSynchronization {
  SDL_mutex* renderthread_mutex_;
  SDL_mutex* updatethread_mutex_;
  SDL_mutex* gameupdate_mutex_;
  SDL_cond* start_render_cv_;
  SDL_cond* start_update_cv_;
  GameSynchronization();
};

class Game {
 public:
  Game();
  bool Initialize(const char* const binary_directory);
  void Run();

  // Set the overlay directory name to optionally load assets from.
  static void SetOverlayName(const char* overlay_name) {
    overlay_name_ = overlay_name;
  }

#if defined(__ANDROID__)
  // Parse launch mode and overlay directory name from Intent data.
  static void ParseViewIntentData(const std::string& intent_data,
                                  std::string* launch_mode,
                                  std::string* overlay);
#endif  // defined(__ANDROID__)

 private:
  bool InitializeRenderer();
  bool InitializeAssets();
  void InitializeBreadboardModules();

  void Update(corgi::WorldTime delta_time);
  void UpdateMainCamera();
  void UpdateMainCameraAndroid();
  void UpdateMainCameraMouse();
  const Config& GetConfig() const;
  const InputConfig& GetInputConfig() const;
  const RailDef& GetRailDef() const;
  const AssetManifest& GetAssetManifest() const;
  fplbase::Mesh* GetCardboardFront(int renderable_id);

  void SetRelativeMouseMode(bool relative_mouse_mode);
  void ToggleRelativeMouseMode();

  void UpdateProfiling(corgi::WorldTime frame_time);

  // Overrides fplbase::LoadFile() in order to optionally load files from
  // overlay directories.
  static bool LoadFile(const char* filename, std::string* dest);

  // Mutexes/CVs used in synchronizing the render and update threads:
  GameSynchronization sync_;

  // Hold configuration binary data.
  std::string config_source_;

  // Hold the configuration for the input system data.
  std::string input_config_source_;

  // Hold the configuration for the asset manifest source.
  std::string asset_manifest_source_;

  // The top level state machine that drives the game.
  StateMachine<kGameStateCount> state_machine_;
  LoadingState loading_state_;
  PauseState pause_state_;
  GameplayState gameplay_state_;
  GameMenuState game_menu_state_;
  IntroState intro_state_;
  GameOverState game_over_state_;
  SceneLabState scene_lab_state_;

  // Report touches, button presses, keyboard presses.
  fplbase::InputSystem input_;

  // Hold rendering context.
  fplbase::Renderer renderer_;

  // Load and own rendering resources.
  fplbase::AssetManager asset_manager_;

  flatui::FontManager font_manager_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

  // The event system.
  breadboard::ModuleRegistry module_registry_;
  breadboard::module_library::DefaultGraphFactory graph_factory_;

  // Shaders we use.
  fplbase::Shader* shader_textured_;

#if DISPLAY_FRAMERATE_HISTOGRAM
  // Profiling data.
  static const int kHistogramSize = 64;
  corgi::WorldTime last_printout;
  corgi::WorldTime histogram[kHistogramSize];
#endif

  bool game_exiting_;

  std::string rail_source_;

  pindrop::AudioConfig* audio_config_;

  World world_;
  WorldRenderer world_renderer_;

  // Fade the screen to back and from black.
  FullScreenFader fader_;

  std::unique_ptr<scene_lab::SceneLab> scene_lab_;

  bool relative_mouse_mode_;

  // String version number of the game.
  const char* version_;

  // Google Play Game Services Manager.
  GPGManager gpg_manager_;

  // Name of the optional overlay to load assets from.
  static std::string overlay_name_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_GAME_H
