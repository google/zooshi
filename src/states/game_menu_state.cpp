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
#include "firebase/admob.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "invites.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "save_data_generated.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

#if ANDROID_HMD
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

void GameMenuState::Initialize(
    fplbase::InputSystem *input_system, World *world, const Config *config,
    fplbase::AssetManager *asset_manager, flatui::FontManager *font_manager,
    const AssetManifest *manifest, GPGManager *gpg_manager,
    pindrop::AudioEngine *audio_engine, FullScreenFader *fader) {
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

#if ANDROID_HMD
  cardboard_camera_.set_viewport_angle(config->cardboard_viewport_angle());
#endif
  slider_back_ =
      asset_manager_->LoadTexture("textures/ui_scrollbar_background.webp");
  slider_knob_ = asset_manager_->LoadTexture("textures/ui_scrollbar_knob.webp");
  scrollbar_back_ = asset_manager_->LoadTexture(
      "textures/ui_scrollbar_background_vertical.webp");
  scrollbar_foreground_ =
      asset_manager_->LoadTexture("textures/ui_scrollbar_foreground.webp");

  button_checked_ =
      asset_manager_->LoadTexture("textures/ui_button_checked.webp");
  button_unchecked_ =
      asset_manager_->LoadTexture("textures/ui_button_unchecked.webp");
  cardboard_logo_ = asset_manager_->LoadTexture("textures/cardboard_logo.webp");

  if (!fplbase::LoadFile(manifest->about_file()->c_str(), &about_text_)) {
    fplbase::LogError("About text not found.");
  }

  if (!fplbase::LoadFile(manifest->license_file()->c_str(), &license_text_)) {
    fplbase::LogError("License text not found.");
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

  patrons_fed_ = 0;
  sushi_thrown_ = 0;
  laps_finished_ = 0;
  total_score_ = 0;

  rewarded_video_state_ = kRewardedVideoStateIdle;

  UpdateVolumes();
}

void GameMenuState::AdvanceFrame(int delta_time, int *next_state) {
  world_->entity_manager.UpdateComponents(delta_time);
  UpdateMainCamera(&main_camera_, world_);

  if (rewarded_video_state_ == kRewardedVideoStateDisplaying) {
    HandleRewardedVideo();
    return;
  }

  bool back_button =
      input_system_->GetButton(fplbase::FPLK_ESCAPE).went_down() ||
      input_system_->GetButton(fplbase::FPLK_AC_BACK).went_down();
  if (back_button) {
    if (rewarded_video_state_ == kRewardedVideoStateFinished) {
      // If on the finished screen for rewarded video, clear that state.
      rewarded_video_state_ = kRewardedVideoStateIdle;
      return;
    }

    if (menu_state_ == kMenuStateOptions) {
      // Save data when you leave the audio page.
      if (options_menu_state_ == kOptionsMenuStateAudio) {
        SaveData();
      }
      if (options_menu_state_ == kOptionsMenuStateMain ||
          options_menu_state_ == kOptionsMenuStateSushi ||
          options_menu_state_ == kOptionsMenuStateLevel) {
        menu_state_ = kMenuStateStart;
      } else {
        options_menu_state_ = kOptionsMenuStateMain;
      }
    } else if (menu_state_ == kMenuStateStart) {
      menu_state_ = kMenuStateQuit;
    } else if (menu_state_ == kMenuStateScoreReview ||
               menu_state_ == kMenuStateReceivedInvite ||
               menu_state_ == kMenuStateSendingInvite ||
               menu_state_ == kMenuStateSentInvite ||
               menu_state_ == kMenuStateReceivedMessage) {
      menu_state_ = kMenuStateStart;
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
    master_bus_.SetGain(
        cos(fader_->GetOffset() * 0.5f * static_cast<float>(M_PI)));
    if (fader_->Finished()) {
      *next_state = kGameStateExit;
    }
  } else if (menu_state_ == kMenuStateSendingInvite) {
    bool did_send, first_sent;
    if (UpdateSentInviteStatus(&did_send, &first_sent)) {
      if (did_send) {
        // Put up a message thanking them for inviting others.
        // If it was the first time they've sent an invite, reward them.
        menu_state_ = kMenuStateSentInvite;
        if (first_sent) {
          did_earn_unlockable_ =
              world_->unlockables->UnlockRandom(&earned_unlockable_);
        }
      } else {
        // No invites were sent, so return to normal.
        menu_state_ = kMenuStateStart;
      }
    }
  }

  // If we are not transitioning to another state, check for received invites
  // and messages.
  if (menu_state_ == kMenuStateStart && *next_state == kGameStateGameMenu) {
    if (world_->invites_listener->has_pending_invite()) {
      world_->invites_listener->HandlePendingInvite();
      did_earn_unlockable_ =
          world_->unlockables->UnlockRandom(&earned_unlockable_);
      menu_state_ = kMenuStateReceivedInvite;
    } else if (world_->message_listener->has_pending_message()) {
      received_message_ =
          world_->message_listener->HandlePendingMessage(world_);
      menu_state_ = kMenuStateReceivedMessage;
    }
  }
}

void GameMenuState::RenderPrep() {
  world_->world_renderer->RenderPrep(main_camera_, world_);
}

void GameMenuState::Render(fplbase::Renderer *renderer) {
  // Ensure assets are instantiated after they've been loaded.
  // This must be called from the render thread.
  loading_complete_ = asset_manager_->TryFinalize();

  Camera *cardboard_camera = nullptr;
#if ANDROID_HMD
  cardboard_camera = &cardboard_camera_;
#endif
  RenderWorld(*renderer, world_, main_camera_, cardboard_camera, input_system_);
}

void GameMenuState::HandleUI(fplbase::Renderer *renderer) {
  // Don't show game menu until everything has finished loading.
  if (!loading_complete_) {
    return;
  }

  // No culling when drawing the menu.
  renderer->SetCulling(fplbase::kCullingModeNone);

  if (rewarded_video_state_ != kRewardedVideoStateIdle) {
    rewarded_video_state_ =
        RewardedVideoMenu(*asset_manager_, *font_manager_, *input_system_);
    return;
  }

  switch (menu_state_) {
    case kMenuStateStart:
      menu_state_ = StartMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    case kMenuStateOptions:
      menu_state_ = OptionMenu(*asset_manager_, *font_manager_, *input_system_);
      break;
    case kMenuStateScoreReview:
      menu_state_ =
          ScoreReviewMenu(*asset_manager_, *font_manager_, *input_system_);
      // If leaving the score review page, clear the cached scores.
      if (menu_state_ != kMenuStateScoreReview) {
        ResetScore();
        world_->admob_helper->ResetRewardedVideo();
      }
      break;
    case kMenuStateReceivedInvite:
      menu_state_ =
          ReceivedInviteMenu(*asset_manager_, *font_manager_, *input_system_);
      if (menu_state_ != kMenuStateReceivedInvite) {
        did_earn_unlockable_ = false;
      }
      break;
    case kMenuStateSentInvite:
      menu_state_ =
          SentInviteMenu(*asset_manager_, *font_manager_, *input_system_);
      if (menu_state_ != kMenuStateSentInvite) {
        did_earn_unlockable_ = false;
      }
      break;
    case kMenuStateReceivedMessage:
      menu_state_ =
          ReceivedMessageMenu(*asset_manager_, *font_manager_, *input_system_);
      if (menu_state_ != kMenuStateReceivedMessage) {
        received_message_ = "";
      }
      break;
    case kMenuStateQuit: {
      flatui::Run(*asset_manager_, *font_manager_, *input_system_, [&]() {
        flatui::CustomElement(
            flatui::GetVirtualResolution(), "fader",
            [&](const vec2i & /*pos*/, const vec2i & /*size*/) {
              fader_->Render(renderer);
            });
      });
      break;
    }

    default:
      break;
  }
}

void GameMenuState::OnEnter(int previous_state) {
  // If coming from the gameover state, we want to display the score review,
  // and preserve the values that we want to display, before resetting the
  // world.
  if (previous_state == kGameStateGameOver) {
    menu_state_ = kMenuStateScoreReview;
    auto attribute_data =
        world_->entity_manager.GetComponentData<AttributesData>(
            world_->active_player_entity);
    patrons_fed_ = attribute_data->attributes[AttributeDef_PatronsFed];
    sushi_thrown_ = attribute_data->attributes[AttributeDef_ProjectilesFired];
    corgi::EntityRef raft =
        world_->entity_manager.GetComponent<ServicesComponent>()->raft_entity();
    RailDenizenData *raft_rail_denizen =
        world_->entity_manager.GetComponentData<RailDenizenData>(raft);
    assert(raft_rail_denizen);
    laps_finished_ = raft_rail_denizen->lap_number;

    float accuracy = 0.0f;
    if (sushi_thrown_) {
      accuracy =
          static_cast<float>(patrons_fed_) / static_cast<float>(sushi_thrown_);
    }
    total_score_ = static_cast<int>(kScorePatronsFedFactor * patrons_fed_) +
                   static_cast<int>(kScoreLapsFinishedFactor * laps_finished_) +
                   static_cast<int>(kScoreAccuracyFactor * accuracy);
    // Calculate the earned xp based on the total score.
    earned_xp_ = world_->xp_system->ApplyBonuses(total_score_, true);
    if (world_->xp_system->GrantXP(earned_xp_)) {
      did_earn_unlockable_ =
          world_->unlockables->UnlockRandom(&earned_unlockable_);
    } else {
      did_earn_unlockable_ = false;
    }
  } else {
    menu_state_ = kMenuStateStart;
#if ANDROID_HMD
    if (world_->is_in_cardboard()) menu_state_ = kMenuStateCardboard;
#endif  // ANDROID_HMD
  }

  loading_complete_ = false;
  LoadWorldDef(world_, world_def_);
  UpdateMainCamera(&main_camera_, world_);
  music_channel_ = audio_engine_->PlaySound(music_menu_);
  world_->player_component.set_state(kPlayerState_Disabled);
  input_system_->SetRelativeMouseMode(false);
  world_->ResetControllerFacing();
  LoadData();

  // We only want to receive messages when in the game menu state.
  StartReceivingMessages(world_);
}

void GameMenuState::OnExit(int /*next_state*/) {
  music_channel_.Stop();
  StopReceivingMessages();
}

void GameMenuState::LoadData() {
  // Set default values.
  slider_value_effect_ = kEffectVolumeDefault;
  slider_value_music_ = kMusicVolumeDefault;

  // Retrieve save file path.
  std::string storage_path;
  std::string data;
  auto ret = fplbase::GetStoragePath(kSaveAppName, &storage_path);
  if (ret &&
      fplbase::LoadPreferences((storage_path + kSaveFileName).c_str(), &data)) {
    auto save_data = GetSaveData(static_cast<const void *>(data.c_str()));
    slider_value_effect_ = save_data->effect_volume();
    slider_value_music_ = save_data->music_volume();

    world_->SetRenderingOption(kShadowEffect, save_data->render_shadows());
    world_->SetRenderingOption(kPhongShading, save_data->apply_phong());
    world_->SetRenderingOption(kSpecularEffect, save_data->apply_specular());
    world_->SetRenderingOptionCardboard(kShadowEffect,
                                        save_data->render_shadows_cardboard());
    world_->SetRenderingOptionCardboard(kPhongShading,
                                        save_data->apply_phong_cardboard());
    world_->SetRenderingOptionCardboard(kSpecularEffect,
                                        save_data->apply_specular_cardboard());
#if ANDROID_HMD
    world_->SetHmdControllerEnabled(save_data->gyroscopic_controls_enabled() !=
                                    0);
#endif  // ANDROID_HMD
  }
}

void GameMenuState::SaveData() {
  // Create FlatBuffer for save data.
  flatbuffers::FlatBufferBuilder fbb;
  SaveDataBuilder builder(fbb);
  builder.add_effect_volume(slider_value_effect_);
  builder.add_music_volume(slider_value_music_);
  builder.add_render_shadows(world_->RenderingOptionEnabled(kShadowEffect));
  builder.add_apply_phong(world_->RenderingOptionEnabled(kPhongShading));
  builder.add_apply_specular(world_->RenderingOptionEnabled(kSpecularEffect));
  builder.add_render_shadows_cardboard(
      world_->RenderingOptionEnabledCardboard(kShadowEffect));
  builder.add_apply_phong_cardboard(
      world_->RenderingOptionEnabledCardboard(kPhongShading));
  builder.add_apply_specular_cardboard(
      world_->RenderingOptionEnabledCardboard(kSpecularEffect));
#if ANDROID_HMD
  builder.add_gyroscopic_controls_enabled(
      world_->GetHmdControllerEnabled() ? 1 : 0);
#endif  // ANDROID_HMD
  auto offset = builder.Finish();
  FinishSaveDataBuffer(fbb, offset);

  // Retrieve save file path.
  std::string storage_path;
  auto ret = fplbase::GetStoragePath(kSaveAppName, &storage_path);
  if (ret) {
    fplbase::SavePreferences((storage_path + kSaveFileName).c_str(),
                             fbb.GetBufferPointer(), fbb.GetSize());
  }
}

void GameMenuState::UpdateVolumes() {
  sound_effects_bus_.SetGain(slider_value_effect_);
  voices_bus_.SetGain(slider_value_effect_);
  music_bus_.SetGain(slider_value_music_);
}

void GameMenuState::ResetScore() {
  patrons_fed_ = 0;
  sushi_thrown_ = 0;
  laps_finished_ = 0;
  total_score_ = 0;
  earned_xp_ = 0;
  did_earn_unlockable_ = false;
}

void GameMenuState::StartRewardedVideo() {
  rewarded_video_state_ = kRewardedVideoStateDisplaying;
  world_->admob_helper->ShowRewardedVideo();
}

void GameMenuState::HandleRewardedVideo() {
  if (world_->admob_helper->CheckShowRewardedVideo()) {
    rewarded_video_state_ = kRewardedVideoStateFinished;
    // Start loading the next rewarded video now.
    world_->admob_helper->LoadNewRewardedVideo();

    // If a reward was not earned, return now.
    if (!world_->admob_helper->rewarded_video_watched())
      return;

    if (menu_state_ == kMenuStateScoreReview) {
      // If already on the Score Review state, we want to apply the bonuses
      // immediately, and give an unlockable if necessary.
      int reward_value =
          static_cast<int>(world_->admob_helper->reward_value());
      earned_xp_ += reward_value;
      bool earned_reward = world_->xp_system->GrantXP(reward_value);
      // If an unlockable was not previously granted, grant one now.
      if (earned_reward && !did_earn_unlockable_) {
        did_earn_unlockable_ =
            world_->unlockables->UnlockRandom(&earned_unlockable_);
      }
    } else {
      // Otherwise, we want to use the XP system to handle applying the
      // bonus.
      world_->xp_system->AddBonus(BonusApplyType_Addition,
                                  world_->admob_helper->reward_value(),
                                  1, UniqueBonusId_AdMobRewardedVideo);
    }
  }
}

}  // zooshi
}  // fpl
