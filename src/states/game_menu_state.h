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

#ifndef ZOOSHI_GAME_MENU_STATE_H_
#define ZOOSHI_GAME_MENU_STATE_H_

#include "assets_generated.h"
#include "camera.h"
#include "flatui/flatui.h"
#include "flatui/flatui_common.h"
#include "fplbase/input.h"
#include "gpg_manager.h"
#include "pindrop/pindrop.h"
#include "states/gameplay_state.h"
#include "states/state_machine.h"

namespace fpl {

class InputSystem;

namespace zooshi {

class BasePlayerController;
struct Config;
struct InputConfig;
struct WorldDef;

enum MenuState {
  kMenuStateStart,
  kMenuStateOptions,
  kMenuStateFinished,
  kMenuStateCardboard,
  kMenuStateGamepad,
  kMenuStateQuit,
};

enum OptionsMenuState {
  kOptionsMenuStateMain,
  kOptionsMenuStateAbout,
  kOptionsMenuStateLicenses,
  kOptionsMenuStateAudio,
};

// Constant defintions for UI elements. Colors, button sizes etc.
const auto kColorBrown = mathfu::vec4(0.37f, 0.24f, 0.09f, 0.85f);
const auto kColorLightBrown = mathfu::vec4(0.82f, 0.77f, 0.60f, 0.85f);
const auto kColorLightGray = mathfu::vec4(0.4f, 0.4f, 0.4f, 0.85f);
const auto kColorDarkGray = mathfu::vec4(0.1f, 0.1f, 0.1f, 0.85f);
const auto kPressedColor = mathfu::vec4(1.0f, 1.0f, 1.0f, 1.0f);
const auto kHoverColor = mathfu::vec4::Min(kColorBrown * 1.5f, mathfu::kOnes4f);
#ifdef USING_GOOGLE_PLAY_GAMES
const auto kMenuSize = 75.0f;
const auto kButtonSize = 75.0f;
#else
const auto kMenuSize = 150.0f;
const auto kButtonSize = 140.0f;
#endif
const auto kAudioOptionButtonSize = 100.0f;
const auto kScrollAreaSize = mathfu::vec2(900, 500);

const auto kEffectVolumeDefault = 1.0f;
const auto kMusicVolumeDefault = 1.0f;
const auto kSaveFileName = "save_data.zoosave";
const auto kSaveAppName = "zooshi";

class GameMenuState : public StateNode {
 public:
  virtual ~GameMenuState() {}

  void Initialize(fplbase::InputSystem* input_system, World* world,
                  const Config* config,
                  fplbase::AssetManager* asset_manager,
                  flatui::FontManager* font_manager,
                  const AssetManifest* manifest, GPGManager* gpg_manager,
                  pindrop::AudioEngine* audio_engine, FullScreenFader* fader);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void RenderPrep(fplbase::Renderer* renderer);
  virtual void Render(fplbase::Renderer* renderer);
  virtual void HandleUI(fplbase::Renderer* renderer);
  virtual void OnEnter(int previous_state);
  virtual void OnExit(int next_state);

 private:
  MenuState StartMenu(fplbase::AssetManager& assetman,
                      flatui::FontManager& fontman,
                      fplbase::InputSystem& input);
  MenuState OptionMenu(fplbase::AssetManager& assetman,
                       flatui::FontManager& fontman,
                       fplbase::InputSystem& input);
  void OptionMenuMain();
  void OptionMenuLicenses();
  void OptionMenuAbout();
  void OptionMenuAudio();

  // Instance a text button that plays a sound when selected.
  flatui::Event TextButton(const char* text, float size,
                        const flatui::Margin& margin);
  flatui::Event TextButton(const char* text, float size,
                        const flatui::Margin& margin,
                        pindrop::SoundHandle& sound);
  // Instance a text button showing an image that plays a sound when selected.
  flatui::Event TextButton(const fplbase::Texture& texture,
                        const flatui::Margin& texture_margin, const char* text,
                        float size, const flatui::Margin& margin,
                        const flatui::ButtonProperty property);
  flatui::Event TextButton(const fplbase::Texture& texture,
                        const flatui::Margin& texture_margin, const char* text,
                        float size, const flatui::Margin& margin,
                        const flatui::ButtonProperty property,
                        pindrop::SoundHandle& sound);
  // Instance an image button with label that plays a sound when selected.
  flatui::Event ImageButtonWithLabel(const fplbase::Texture& tex, float size,
                                  const flatui::Margin& margin,
                                  const char* label);
  flatui::Event ImageButtonWithLabel(const fplbase::Texture& tex, float size,
                                  const flatui::Margin& margin,
                                  const char* label,
                                  pindrop::SoundHandle& sound);
  // Play button sound effect when a selected event fires.
  flatui::Event PlayButtonSound(flatui::Event event,
                                pindrop::SoundHandle& sound);

  // Save/Load data to strage using FlatBuffres binary data.
  void SaveData();
  void LoadData();

  // Set sound volumes based on volume settings.
  void UpdateVolumes();

  // The world to display in the background.
  World* world_;

  // The camera(s) to use to render the background world.
  Camera main_camera_;
#ifdef ANDROID_HMD
  Camera cardboard_camera_;
#endif

  // In mobile platforms, UI has menu items for Google Play Game service.
  GPGManager* gpg_manager_;

  // FlatUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  fplbase::InputSystem* input_system_;

  // FlatUI loads resources using AssetManager.
  fplbase::AssetManager* asset_manager_;

  // FlatUI uses FontManager for a text rendering.
  flatui::FontManager* font_manager_;

  // Asset manifest used to retrieve configurations.
  const Config* config_;

  // Full screen fader, used to fade out when the game exits.
  FullScreenFader* fader_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_start_;
  pindrop::SoundHandle sound_click_;
  pindrop::SoundHandle sound_adjust_;
  pindrop::SoundHandle sound_select_;
  pindrop::SoundHandle sound_exit_;

  // This will eventually be removed when there are events to handle this logic.
  pindrop::SoundHandle music_menu_;
  pindrop::Channel music_channel_;

  // Menu state.
  MenuState menu_state_;

  // The world definition to load upon entering the menu.
  const WorldDef* world_def_;

  // Textures used in menu UI.
  fplbase::Texture* background_title_;
  fplbase::Texture* background_options_;
  fplbase::Texture* button_back_;
  fplbase::Texture* slider_back_;
  fplbase::Texture* slider_knob_;
  fplbase::Texture* scrollbar_back_;
  fplbase::Texture* scrollbar_foreground_;
#ifdef USING_GOOGLE_PLAY_GAMES
  fplbase::Texture* image_gpg_;
  fplbase::Texture* image_leaderboard_;
  fplbase::Texture* image_achievements_;
#endif

  // Option menu state.
  mathfu::vec2 scroll_offset_;
  std::string license_text_;
  std::string about_text_;

  // In-game menu state.
  OptionsMenuState options_menu_state_;

  float slider_value_effect_;
  float slider_value_music_;

  // Buses to control sound volumes.
  pindrop::Bus sound_effects_bus_;
  pindrop::Bus voices_bus_;
  pindrop::Bus music_bus_;
  pindrop::Bus master_bus_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_GAME_MENU_STATE_H_
