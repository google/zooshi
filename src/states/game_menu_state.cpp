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

#include "game.h"
#include "states/game_menu_state.h"

#include "components/sound.h"
#include "config_generated.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "save_data_generated.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

#ifdef ANDROID_HMD
#include "fplbase/renderer_hmd.h"
#endif

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

void GameMenuState::Initialize(InputSystem* input_system, World* world,
                               const Config* config,
                               AssetManager* asset_manager,
                               FontManager* font_manager,
                               const AssetManifest* manifest,
                               GPGManager* gpg_manager,
                               pindrop::AudioEngine* audio_engine,
                               FullScreenFader* fader) {
  world_ = world;

  // Set references used in GUI.
  input_system_ = input_system;
  asset_manager_ = asset_manager;
  font_manager_ = font_manager;
  audio_engine_ = audio_engine;
  config_ = config;
  fader_ = fader;

  sound_start_ = audio_engine->GetSoundHandle("start");
  sound_click_ = audio_engine->GetSoundHandle("click");
  sound_select_ = audio_engine->GetSoundHandle("select");
  sound_adjust_ = sound_select_;
  sound_exit_ = audio_engine->GetSoundHandle("exit");
  music_menu_ = audio_engine->GetSoundHandle("music_menu");

  // Set menu state.
  menu_state_ = kMenuStateStart;
  options_menu_state_ = kOptionsMenuStateMain;

  // Set the world def to load upon entering this state.
  world_def_ = config->world_def();

  // Retrieve references to textures. (Loading process is done already.)
  background_title_ =
      asset_manager_->LoadTexture("textures/ui_background_main.webp");
  background_options_ =
      asset_manager_->LoadTexture("textures/ui_background_base.webp");
  button_back_ = asset_manager_->LoadTexture("textures/ui_button_back.webp");

#ifdef ANDROID_HMD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#endif
  slider_back_ =
      asset_manager_->LoadTexture("textures/ui_scrollbar_background.webp");
  slider_knob_ = asset_manager_->LoadTexture("textures/ui_scrollbar_knob.webp");
  scrollbar_back_ = asset_manager_->LoadTexture(
      "textures/ui_scrollbar_background_vertical.webp");
  scrollbar_foreground_ =
      asset_manager_->LoadTexture("textures/ui_scrollbar_foreground.webp");

  if (!LoadFile(manifest->about_file()->c_str(), &about_text_)) {
    LogError("About text not found.");
  }

  if (!LoadFile(manifest->license_file()->c_str(), &license_text_)) {
    LogError("License text not found.");
  }

  gpg_manager_ = gpg_manager;

#ifdef USING_GOOGLE_PLAY_GAMES
  image_gpg_ = asset_manager_->LoadTexture("textures/games_controller.webp");
  image_leaderboard_ =
      asset_manager_->LoadTexture("textures/games_leaderboards_green.webp");
  image_achievements_ =
      asset_manager_->LoadTexture("textures/games_achievements_green.webp");
#endif

  sound_effects_bus_ = audio_engine->FindBus("sound_effects");
  voices_bus_ = audio_engine->FindBus("voices");
  music_bus_ = audio_engine->FindBus("music");
  master_bus_ = audio_engine->FindBus("master");
  LoadData();

  UpdateVolumes();
}

void GameMenuState::AdvanceFrame(int delta_time, int* next_state) {
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  bool back_button = input_system_->GetButton(FPLK_ESCAPE).went_down() ||
                     input_system_->GetButton(FPLK_AC_BACK).went_down();
  if (back_button) {
    if (menu_state_ == kMenuStateOptions) {
      // Save data when you leave the audio page.
      if (options_menu_state_ == kOptionsMenuStateAudio) {
        SaveData();
      }
      if (options_menu_state_ == kOptionsMenuStateMain) {
        menu_state_ = kMenuStateStart;
      } else {
        options_menu_state_ = kOptionsMenuStateMain;
      }
    } else if (menu_state_ == kMenuStateStart) {
      menu_state_ = kMenuStateQuit;
    }
  }

  if (menu_state_ == kMenuStateStart) {
    world_->SetIsInCardboard(false);
  } else if (menu_state_ == kMenuStateFinished) {
    *next_state = kGameStateGameplay;
    world_->SetIsInCardboard(false);
    world_->SetActiveController(kControllerDefault);
  } else if (menu_state_ == kMenuStateCardboard) {
    *next_state = kGameStateIntro;
	world_->SetHmdControllerEnabled(true);
    world_->SetIsInCardboard(true);
    world_->SetActiveController(kControllerDefault);
  } else if (menu_state_ == kMenuStateGamepad) {
    *next_state = kGameStateGameplay;
    world_->SetIsInCardboard(false);
    world_->SetActiveController(kControllerGamepad);
  } else if (menu_state_ == kMenuStateQuit) {
    fader_->AdvanceFrame(delta_time);
    // Perform a roughly inverse logarithmic fade out.
    master_bus_.SetGain(cos(fader_->GetOffset() * 0.5f *
                            static_cast<float>(M_PI)));
    if (fader_->Finished()) {
      *next_state = kGameStateExit;
    }
  }
}

void GameMenuState::RenderPrep(Renderer* renderer) {
  world_->world_renderer->RenderPrep(main_camera_, *renderer, world_);
}

void GameMenuState::Render(Renderer* renderer) {
  Camera* cardboard_camera = nullptr;
#ifdef ANDROID_HMD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void GameMenuState::HandleUI(Renderer* renderer) {
  // No culling when drawing the menu.
  renderer->SetCulling(Renderer::kNoCulling);

  switch (menu_state_) {
    case kMenuStateStart:
      menu_state_ = StartMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    case kMenuStateOptions:
      menu_state_ = OptionMenu(*asset_manager_, *font_manager_, *input_system_);
      break;

  case kMenuStateQuit: {
      gui::Run(*asset_manager_, *font_manager_, *input_system_, [&]() {
          gui::CustomElement(gui::GetVirtualResolution(), "fader", [&](
                             const vec2i& /*pos*/, const vec2i& /*size*/) {
                fader_->Render(renderer);
              });
          });
      break;
    }

    default:
      break;
  }
}

void GameMenuState::OnEnter(int /*previous_state*/) {
  LoadWorldDef(world_, world_def_);
  UpdateMainCamera(&main_camera_, world_);
  music_channel_ = audio_engine_->PlaySound(music_menu_);
  world_->player_component.set_state(kPlayerState_Disabled);
  input_system_->SetRelativeMouseMode(false);
  world_->ResetControllerFacing();
  menu_state_ = kMenuStateStart;
#ifdef ANDROID_HMD
  if (world_->is_in_cardboard()) menu_state_ = kMenuStateCardboard;
#endif  // ANDROID_HMD
  LoadData();
}

void GameMenuState::OnExit(int /*next_state*/) { music_channel_.Stop(); }

void GameMenuState::LoadData() {
  // Set default values.
  slider_value_effect_ = kEffectVolumeDefault;
  slider_value_music_ = kMusicVolumeDefault;

  // Retrieve save file path.
  std::string storage_path;
  std::string data;
  auto ret = GetStoragePath(kSaveAppName, &storage_path);
  if (ret && LoadPreferences((storage_path + kSaveFileName).c_str(), &data)) {
    auto save_data = GetSaveData(static_cast<const void*>(data.c_str()));
    slider_value_effect_ = save_data->effect_volume();
    slider_value_music_ = save_data->music_volume();
#ifdef ANDROID_HMD
    world_->SetHmdControllerEnabled(
        save_data->gyroscopic_controls_enabled() != 0);
#endif  // ANDROID_HMD
  }
}

void GameMenuState::SaveData() {
  // Create FlatBuffer for save data.
  flatbuffers::FlatBufferBuilder fbb;
  SaveDataBuilder builder(fbb);
  builder.add_effect_volume(slider_value_effect_);
  builder.add_music_volume(slider_value_music_);
#ifdef ANDROID_HMD
  builder.add_gyroscopic_controls_enabled(
      world_->GetHmdControllerEnabled() ? 1 : 0);
#endif  // ANDROID_HMD
  auto offset = builder.Finish();
  FinishSaveDataBuffer(fbb, offset);

  // Retrieve save file path.
  std::string storage_path;
  auto ret = GetStoragePath(kSaveAppName, &storage_path);
  if (ret) {
    SavePreferences((storage_path + kSaveFileName).c_str(),
                    fbb.GetBufferPointer(), fbb.GetSize());
  }
}

void GameMenuState::UpdateVolumes() {
  sound_effects_bus_.SetGain(slider_value_effect_);
  voices_bus_.SetGain(slider_value_effect_);
  music_bus_.SetGain(slider_value_music_);
}

}  // zooshi
}  // fpl
