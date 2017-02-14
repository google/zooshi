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

#include "mathfu/internal/disable_warnings_begin.h"

#include "firebase/remote_config.h"

#include "mathfu/internal/disable_warnings_end.h"

#include "fplbase/debug_markers.h"
#include "fplbase/utilities.h"
#include "invites.h"
#include "states/game_menu_state.h"
#include "states/states_common.h"
#include "remote_config.h"

#include <iostream>

using mathfu::vec2;
using mathfu::vec3;

namespace fpl {
namespace zooshi {

flatui::Event GameMenuState::PlayButtonSound(flatui::Event event,
                                          pindrop::SoundHandle& sound) {
  if (event & flatui::kEventWentUp) {
    audio_engine_->PlaySound(sound);
  }
  return event;
}

flatui::Event GameMenuState::TextButton(const char* text, float size,
                                     const flatui::Margin& margin) {
  return TextButton(text, size, margin, sound_click_);
}

flatui::Event GameMenuState::TextButton(const char* text, float size,
                                     const flatui::Margin& margin,
                                     pindrop::SoundHandle& sound) {
  return PlayButtonSound(flatui::TextButton(text, size, margin), sound);
}

flatui::Event GameMenuState::TextButton(const fplbase::Texture& texture,
                                     const flatui::Margin& texture_margin,
                                     const char* text, float size,
                                     const flatui::Margin& margin,
                                     const flatui::ButtonProperty property) {
  return TextButton(texture, texture_margin, text, size, margin, property,
                    sound_click_);
}

flatui::Event GameMenuState::TextButton(const fplbase::Texture& texture,
                                     const flatui::Margin& texture_margin,
                                     const char* text, float size,
                                     const flatui::Margin& margin,
                                     const flatui::ButtonProperty property,
                                     pindrop::SoundHandle& sound) {
  return PlayButtonSound(flatui::TextButton(texture, texture_margin, text, size,
                                         margin, property), sound);
}

flatui::Event GameMenuState::ImageButtonWithLabel(const fplbase::Texture& tex,
                                                  float size,
                                                  const flatui::Margin& margin,
                                                  const char* label) {
  return ImageButtonWithLabel(tex, size, margin, label, sound_click_);
}

flatui::Event GameMenuState::ImageButtonWithLabel(const fplbase::Texture& tex,
                                                  float size,
                                                  const flatui::Margin& margin,
                                                  const char* label,
                                                  pindrop::SoundHandle& sound) {
  flatui::StartGroup(flatui::kLayoutVerticalLeft, size, "ImageButtonWithLabel");
  flatui::SetMargin(margin);
  auto event = PlayButtonSound(flatui::CheckEvent(), sound);
  flatui::EventBackground(event);
  flatui::ImageBackground(tex);
  flatui::Label(label, size);
  flatui::EndGroup();
  return event;
}

MenuState GameMenuState::StartMenu(fplbase::AssetManager& assetman,
                                   flatui::FontManager& fontman,
                                   fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateStart;

  PushDebugMarker("StartMenu");

  // Run() accepts a lambda function that is executed 2 times,
  // one for a layout pass and another one in a render pass.
  // In the lambda callback, the user can call Widget APIs to put widget in a
  // layout.
  flatui::Run(assetman, fontman, input, [&]() {
    flatui::StartGroup(flatui::kLayoutHorizontalTop, 0);

    // Background image.
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                       mathfu::vec2(0, -150));
    flatui::Image(*background_title_, 1400);
    flatui::EndGroup();

    flatui::SetTextColor(kColorBrown);
    flatui::SetTextFont(config_->menu_font()->c_str());

    // Menu items. Note that we are layering 2 layouts here
    // (background + menu items).
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 0);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                       mathfu::vec2(0, -150));
    flatui::SetMargin(flatui::Margin(200, 700, 200, 100));

    auto event = TextButton("Play Game", kMenuSize, flatui::Margin(0),
                            sound_start_);
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateFinished;
#ifdef ANDROID_GAMEPAD
      if (!flatui::IsLastEventPointerType()) {
        next_state = kMenuStateGamepad;
      }
#endif
    }
    if (fplbase::SupportsHeadMountedDisplay()) {
      event = TextButton("Cardboard", kMenuSize, flatui::Margin(0),
                         sound_start_);
      if (event & flatui::kEventWentUp) {
        next_state = kMenuStateCardboard;
      }
    }
#ifdef USING_GOOGLE_PLAY_GAMES
    auto logged_in = gpg_manager_->LoggedIn();
    event = TextButton(*image_gpg_, flatui::Margin(0, 50, 10, 0),
                       logged_in ? "Sign out" : "Sign in", kMenuSize,
                       flatui::Margin(0), flatui::kButtonPropertyImageLeft,
                       sound_select_);
    if (event & flatui::kEventWentUp) {
      gpg_manager_->ToggleSignIn();
    }
#endif
    // Since sending invites and admob video are currently not supported on
    // desktop, and the UI space is limited, only offer the option on Android.
#ifdef __ANDROID__
    event = TextButton("Send Invite", kMenuSize, flatui::Margin(0));
    if (event & flatui::kEventWentUp) {
      SendInvite();
      next_state = kMenuStateSendingInvite;
    }
    if (world_->admob_helper->rewarded_video_available() &&
        !world_->admob_helper->rewarded_video_watched() &&
        world_->admob_helper->GetRewardedVideoLocation() ==
            kRewardedVideoLocationPregame) {
      event = TextButton("Earn bonuses before playing", kMenuSize,
                         flatui::Margin(0));
      if (event & flatui::kEventWentUp) {
        StartRewardedVideo();
      }
    }
#endif  // __ANDROID__
    event = TextButton("Options", kMenuSize, flatui::Margin(0));
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateOptions;
      options_menu_state_ = kOptionsMenuStateMain;
    }
    event = TextButton("Quit", kMenuSize, flatui::Margin(0), sound_exit_);
    if (event & flatui::kEventWentUp) {
      // The exit sound is actually around 1.2s but since we fade out the
      // audio as well as the screen it's reasonable to shorten the duration.
      static const int kFadeOutTimeMilliseconds = 1000;
      fader_->Start(
          kFadeOutTimeMilliseconds, mathfu::kZeros3f, kFadeOut,
          vec3(vec2(flatui::VirtualToPhysical(mathfu::kZeros2f)), 0.0f),
          vec3(vec2(flatui::VirtualToPhysical(flatui::GetVirtualResolution())),
               0.0f));
      next_state = kMenuStateQuit;
    }
    flatui::EndGroup();

    // Sushi selection is done offset to the right of the menu layout.
    const UnlockableConfig* current_sushi = world_->SelectedSushi();
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 20);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::vec2(375, 100));
    flatui::SetTextColor(kColorLightBrown);
    event =
        ImageButtonWithLabel(*button_back_, 60, flatui::Margin(60, 35, 40, 50),
                             current_sushi->name()->c_str());
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateOptions;
      options_menu_state_ = kOptionsMenuStateSushi;
    }
    event =
        ImageButtonWithLabel(*button_back_, 60, flatui::Margin(60, 35, 40, 50),
                             world_->CurrentLevel()->name()->c_str());
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateOptions;
      options_menu_state_ = kOptionsMenuStateLevel;
    }
    flatui::EndGroup();
    flatui::EndGroup();
  });

  PopDebugMarker(); // StartMenu

  return next_state;
}

MenuState GameMenuState::OptionMenu(fplbase::AssetManager& assetman,
                                    flatui::FontManager& fontman,
                                    fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateOptions;

  PushDebugMarker("OptionMenu");

  // FlatUI UI definitions.
  flatui::Run(assetman, fontman, input, [&]() {
    flatui::StartGroup(flatui::kLayoutOverlay, 0);
    flatui::StartGroup(flatui::kLayoutHorizontalTop, 0);
    // Background image. Note that we are layering 3 layouts here
    // (background + menu items + back button).
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                       mathfu::vec2(0, -150));
    flatui::Image(*background_options_, 1400);
    flatui::EndGroup();

    flatui::SetTextColor(kColorBrown);
    flatui::SetTextFont(config_->menu_font()->c_str());

    // Menu items.
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 0);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                       mathfu::vec2(0, -100));

    switch (options_menu_state_) {
      case kOptionsMenuStateMain:
        OptionMenuMain();
        break;
      case kOptionsMenuStateAbout:
        OptionMenuAbout();
        break;
      case kOptionsMenuStateLicenses:
        OptionMenuLicenses();
        break;
      case kOptionsMenuStateAudio:
        OptionMenuAudio();
        break;
      case kOptionsMenuStateRendering:
        OptionMenuRendering();
        break;
      case kOptionsMenuStateSushi:
        OptionMenuSushi();
        break;
      case kOptionsMenuStateLevel:
        OptionMenuLevel();
        break;
      default:
        break;
    }

    flatui::EndGroup();

    // Foreground image (back button).
    flatui::StartGroup(flatui::kLayoutVerticalRight, 0);
    // Positioning the UI to up-left corner of the dialog.
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                       mathfu::vec2(-450, -250));
    flatui::SetTextColor(kColorLightBrown);

    auto event = ImageButtonWithLabel(*button_back_, 60,
                                      flatui::Margin(60, 35, 40, 50), "Back",
                                      sound_exit_);
    if (event & flatui::kEventWentUp) {
      // Save data when you leave the audio or rendering page.
      if (options_menu_state_ == kOptionsMenuStateAudio ||
          options_menu_state_ == kOptionsMenuStateRendering) {
        SaveData();
      }
      if (options_menu_state_ == kOptionsMenuStateMain ||
          options_menu_state_ == kOptionsMenuStateSushi ||
          options_menu_state_ == kOptionsMenuStateLevel) {
        next_state = kMenuStateStart;
      } else {
        options_menu_state_ = kOptionsMenuStateMain;
      }
    }
    flatui::EndGroup();
    flatui::EndGroup();

    flatui::EndGroup();  // Overlay group.
  });

  PopDebugMarker(); // OptionMenu

  return next_state;
}

void GameMenuState::OptionMenuMain() {
  flatui::SetMargin(flatui::Margin(200, 300, 200, 100));

  flatui::StartGroup(flatui::kLayoutVerticalLeft, 50, "menu");
  flatui::SetMargin(flatui::Margin(0, 20, 0, 50));
  flatui::SetTextColor(kColorBrown);
  flatui::EndGroup();

  if (TextButton("About", kButtonSize, flatui::Margin(2),
                 sound_select_) & flatui::kEventWentUp) {
    options_menu_state_ = kOptionsMenuStateAbout;
  }

#ifdef USING_GOOGLE_PLAY_GAMES
  auto logged_in = gpg_manager_->LoggedIn();
  auto property = flatui::kButtonPropertyImageLeft;

  if (!logged_in) {
    flatui::SetTextColor(kColorLightGray);
    property |= flatui::kButtonPropertyDisabled;
  }
  auto event = TextButton(*image_leaderboard_, flatui::Margin(0, 25, 10, 0),
                          "Leaderboard", kButtonSize, flatui::Margin(0),
                          property);
  if (logged_in && (event & flatui::kEventWentUp)) {
    // Fill in Leaderboard list.
    auto leaderboard_config = config_->gpg_config()->leaderboards();
    gpg_manager_->ShowLeaderboards(
        leaderboard_config->LookupByKey(kGPGDefaultLeaderboard)->id()->c_str());
  }

  event = TextButton(*image_achievements_, flatui::Margin(0, 20, 0, 0),
                     "Achievements", kButtonSize, flatui::Margin(0), property);
  if (logged_in && (event & flatui::kEventWentUp)) {
    gpg_manager_->ShowAchievements();
  }
  flatui::SetTextColor(kColorBrown);
#endif

  if (TextButton("Licenses", kButtonSize, flatui::Margin(2)) &
      flatui::kEventWentUp) {
    scroll_offset_ = mathfu::kZeros2f;
    options_menu_state_ = kOptionsMenuStateLicenses;
  }

  if (TextButton("Audio", kButtonSize, flatui::Margin(2)) &
      flatui::kEventWentUp) {
    options_menu_state_ = kOptionsMenuStateAudio;
  }

  if (TextButton("Rendering", kButtonSize, flatui::Margin(2)) &
      flatui::kEventWentUp) {
    options_menu_state_ = kOptionsMenuStateRendering;
  }

#if FPLBASE_ANDROID_VR
  // If the device supports a head mounted display allow the user to toggle
  // between gyroscopic and onscreen controls.
  if (fplbase::SupportsHeadMountedDisplay()) {
    const bool hmd_controller_enabled = world_->GetHmdControllerEnabled();
    if (TextButton(hmd_controller_enabled ? "Gyroscopic Controls" :
                   "Onscreen Controls", kButtonSize, flatui::Margin(2)) &
        flatui::kEventWentUp) {
      world_->SetHmdControllerEnabled(!hmd_controller_enabled);
      SaveData();
    }
  }
#endif  // FPLBASE_ANDROID_VR

  if (TextButton("Clear Cache", kButtonSize, flatui::Margin(2)) &
      flatui::kEventWentUp) {
    world_->unlockables->LockAll();
    world_->invites_listener->Reset();
  }
}

void GameMenuState::OptionMenuAbout() {
  flatui::SetMargin(flatui::Margin(200, 400, 200, 100));

  flatui::StartGroup(flatui::kLayoutVerticalLeft, 50, "menu");
  flatui::SetMargin(flatui::Margin(0, 20, 0, 55));
  flatui::SetTextColor(kColorBrown);
  flatui::Label("About", kButtonSize);
  flatui::EndGroup();

  flatui::SetTextColor(kColorDarkGray);
  flatui::SetTextFont(config_->license_font()->c_str());

  flatui::StartGroup(flatui::kLayoutHorizontalCenter);
  flatui::SetMargin(flatui::Margin(50, 0, 0, 0));
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 0, "scroll");
  flatui::StartScroll(kScrollAreaSize, &scroll_offset_);
  flatui::Label(about_text_.c_str(), 35, vec2(kScrollAreaSize.x, 0),
                flatui::TextAlignment::kTextAlignmentLeftJustify);
  vec2 scroll_size = flatui::GroupSize();
  flatui::EndScroll();
  flatui::EndGroup();

  // Normalize the scroll offset to use for the scroll bar value.
  auto scroll_height = (scroll_size.y - kScrollAreaSize.y);
  if (scroll_height > 0) {
    auto scrollbar_value = scroll_offset_.y / scroll_height;
    flatui::ScrollBar(*scrollbar_back_, *scrollbar_foreground_,
                   vec2(35, kScrollAreaSize.y),
                   kScrollAreaSize.y / scroll_size.y, "LicenseScrollBar",
                   &scrollbar_value);

    // Convert back the scroll bar value to the scroll offset.
    scroll_offset_.y = scrollbar_value * scroll_height;
  }

  flatui::EndGroup();
  flatui::SetTextFont(config_->menu_font()->c_str());
}

void GameMenuState::OptionMenuLicenses() {
  flatui::SetMargin(flatui::Margin(200, 300, 200, 100));

  flatui::StartGroup(flatui::kLayoutVerticalLeft, 50, "menu");
  flatui::SetMargin(flatui::Margin(0, 20, 0, 55));
  flatui::SetTextColor(kColorBrown);
  flatui::Label("Licenses", kButtonSize);
  flatui::EndGroup();

  flatui::SetTextColor(kColorDarkGray);
  flatui::SetTextFont(config_->license_font()->c_str());

  flatui::StartGroup(flatui::kLayoutHorizontalCenter);
  flatui::SetMargin(flatui::Margin(50, 0, 0, 0));
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 0, "scroll");
  // This check event makes the scroll group controllable with a gamepad.
  flatui::StartScroll(kScrollAreaSize, &scroll_offset_);
  auto event = flatui::CheckEvent(true);
  if (!flatui::IsLastEventPointerType())
    flatui::EventBackground(event);

  flatui::EnableTextHyphenation(true);
  flatui::Label(license_text_.c_str(), 25, vec2(kScrollAreaSize.x, 0),
                flatui::TextAlignment::kTextAlignmentLeftJustify);
  flatui::EnableTextHyphenation(false);

  vec2 scroll_size = flatui::GroupSize();
  flatui::EndScroll();
  flatui::EndGroup();

  // Normalize the scroll offset to use for the scroll bar value.
  auto scroll_height = (scroll_size.y - kScrollAreaSize.y);
  if (scroll_height > 0) {
    auto scrollbar_value = scroll_offset_.y / scroll_height;
    flatui::ScrollBar(*scrollbar_back_, *scrollbar_foreground_,
                   vec2(35, kScrollAreaSize.y),
                   kScrollAreaSize.y / scroll_size.y, "LicenseScrollBar",
                   &scrollbar_value);

    // Convert back the scroll bar value to the scroll offset.
    scroll_offset_.y = scrollbar_value * scroll_height;
  }

  flatui::EndGroup();
  flatui::SetTextFont(config_->menu_font()->c_str());
}

void GameMenuState::OptionMenuAudio() {
  auto original_music_volume = slider_value_music_;
  auto original_effect_volume = slider_value_effect_;
  flatui::SetMargin(flatui::Margin(200, 200, 200, 100));

  flatui::StartGroup(flatui::kLayoutVerticalLeft, 50, "menu");
  flatui::SetMargin(flatui::Margin(0, 50, 0, 50));
  flatui::SetTextColor(kColorBrown);
  flatui::Label("Audio", kButtonSize);
  flatui::EndGroup();

  flatui::StartGroup(flatui::kLayoutHorizontalCenter, 20);
  flatui::Label("Music volume", kAudioOptionButtonSize);
  flatui::SetMargin(flatui::Margin(0, 40, 0, 0));
  flatui::Slider(*slider_back_, *slider_knob_, vec2(400, 60), 0.6f,
                 "MusicVolume", &slider_value_music_);

  flatui::EndGroup();
  flatui::StartGroup(flatui::kLayoutHorizontalCenter, 20);
  flatui::Label("Effect volume", kAudioOptionButtonSize);
  flatui::SetMargin(flatui::Margin(0, 40, 0, 0));
  auto event = flatui::Slider(*slider_back_, *slider_knob_, vec2(400, 60), 0.6f,
                           "EffectVolume", &slider_value_effect_);
  if (event & (flatui::kEventWentUp | flatui::kEventEndDrag)) {
    audio_engine_->PlaySound(sound_adjust_);
  }
  flatui::EndGroup();

  if (original_music_volume != slider_value_music_ ||
      original_effect_volume != slider_value_effect_) {
    UpdateVolumes();
  }
}

void GameMenuState::OptionMenuRendering() {
  flatui::SetMargin(flatui::Margin(200, 200, 200, 100));

  flatui::StartGroup(flatui::kLayoutVerticalLeft, 50, "menu");
  flatui::SetMargin(flatui::Margin(0, 50, 0, 50));
  flatui::SetTextColor(kColorBrown);
  flatui::Label("Rendering", kButtonSize);
  flatui::EndGroup();

  bool render_shadows = world_->RenderingOptionEnabled(kShadowEffect);
  bool apply_phong = world_->RenderingOptionEnabled(kPhongShading);
  bool apply_specular = world_->RenderingOptionEnabled(kSpecularEffect);

  bool render_shadows_cardboard =
      world_->RenderingOptionEnabledCardboard(kShadowEffect);
  bool apply_phong_cardboard =
      world_->RenderingOptionEnabledCardboard(kPhongShading);
  bool apply_specular_cardboard =
      world_->RenderingOptionEnabledCardboard(kSpecularEffect);

  flatui::StartGroup(flatui::kLayoutHorizontalTop, 10);
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter, mathfu::kZeros2f);

  if (fplbase::SupportsHeadMountedDisplay()) {
    flatui::StartGroup(flatui::kLayoutVerticalLeft, 20);
    flatui::SetMargin(flatui::Margin(0, 50, 0, 50));
    flatui::Image(*cardboard_logo_, kButtonSize);
    flatui::CheckBox(*button_checked_, *button_unchecked_, "", kButtonSize,
                     flatui::Margin(0), &render_shadows_cardboard);
    flatui::CheckBox(*button_checked_, *button_unchecked_, "", kButtonSize,
                     flatui::Margin(0), &apply_phong_cardboard);
    flatui::CheckBox(*button_checked_, *button_unchecked_, "", kButtonSize,
                     flatui::Margin(0), &apply_specular_cardboard);
    flatui::EndGroup();
  }

  flatui::StartGroup(flatui::kLayoutVerticalCenter, 20);
  flatui::StartGroup(flatui::kLayoutVerticalLeft, 20);
  flatui::SetMargin(flatui::Margin(0, 70 + kButtonSize, 0, 50));
  flatui::CheckBox(*button_checked_, *button_unchecked_, "Shadows", kButtonSize,
                   flatui::Margin(6, 0), &render_shadows);
  flatui::CheckBox(*button_checked_, *button_unchecked_, "Phong Shading",
                   kButtonSize, flatui::Margin(6, 0), &apply_phong);
  flatui::CheckBox(*button_checked_, *button_unchecked_, "Specular",
                   kButtonSize, flatui::Margin(6, 0), &apply_specular);
  flatui::EndGroup();
  flatui::EndGroup();
  flatui::EndGroup();

  world_->SetRenderingOptionCardboard(kShadowEffect, render_shadows_cardboard);
  world_->SetRenderingOptionCardboard(kPhongShading, apply_phong_cardboard);
  world_->SetRenderingOptionCardboard(kSpecularEffect,
                                      apply_specular_cardboard);
  world_->SetRenderingOption(kShadowEffect, render_shadows);
  world_->SetRenderingOption(kPhongShading, apply_phong);
  world_->SetRenderingOption(kSpecularEffect, apply_specular);

  SaveData();
}

void GameMenuState::OptionMenuSushi() {
  flatui::SetMargin(flatui::Margin(200, 400, 200, 100));

  // Render information about the currently selected sushi.
  const UnlockableConfig* current_sushi = world_->SelectedSushi();
  const SushiConfig* sushi_data =
      static_cast<const SushiConfig*>(current_sushi->data());
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 10, "menu");
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                        mathfu::vec2(30, -210));
  flatui::SetTextColor(kColorBrown);
  flatui::Label(current_sushi->name()->c_str(), kButtonSize);
  flatui::SetTextColor(kColorDarkGray);
  flatui::Label(sushi_data->description()->c_str(), kButtonSize - 5);
  flatui::EndGroup();

  // Render the different sushi types that can be selected.
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 20);
  flatui::SetTextColor(kColorLightBrown);
  const size_t kSushiPerLine = 3;
  for (size_t i = 0; i < config_->sushi_config()->size(); i += kSushiPerLine) {
    flatui::StartGroup(flatui::kLayoutHorizontalCenter, 20);
    for (size_t j = 0;
         j < kSushiPerLine && i + j < config_->sushi_config()->size(); ++j) {
      size_t index = i + j;
      // TODO(amaurice) Replace with artwork for each sushi type.
      if (world_->unlockables->is_unlocked(UnlockableType_Sushi, index)) {
        auto event = ImageButtonWithLabel(
            *button_back_, 60, flatui::Margin(60, 35, 40, 50),
            config_->sushi_config()->Get(index)->name()->c_str());
        if (event & flatui::kEventWentUp) {
          world_->sushi_index = index;
        }
      } else {
        ImageButtonWithLabel(*button_back_, 60, flatui::Margin(60, 35, 40, 50),
                             "  ?????  ");
      }
    }
    flatui::EndGroup();
  }
  flatui::EndGroup();

  // Temporary debug buttons to trigger unlocking a random sushi,
  // and relocking all of them.
  flatui::StartGroup(flatui::kLayoutHorizontalBottom);
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignBottom,
                        mathfu::vec2(0, -50));
  {
    auto event = ImageButtonWithLabel(*button_back_, 60,
                                      flatui::Margin(60, 35, 40, 50), "Unlock");
    if (event & flatui::kEventWentUp) {
      world_->unlockables->UnlockRandom(nullptr);
    }
  }
  {
    auto event = ImageButtonWithLabel(*button_back_, 60,
                                      flatui::Margin(60, 35, 40, 50), "Reset");
    if (event & flatui::kEventWentUp) {
      world_->unlockables->LockAll();
    }
  }
  flatui::EndGroup();
}

void GameMenuState::OptionMenuLevel() {
  flatui::SetMargin(flatui::Margin(200, 400, 200, 100));

  // Render information about the currently selected level.
  const LevelDef* current_level = world_->CurrentLevel();
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 10, "menu");
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                        mathfu::vec2(30, -210));
  flatui::SetTextColor(kColorBrown);
  flatui::Label(current_level->name()->c_str(), kButtonSize);
  flatui::EndGroup();

  // Render the different levels that can be selected.
  flatui::StartGroup(flatui::kLayoutVerticalCenter, 20);
  flatui::SetTextColor(kColorLightBrown);
  const size_t kLevelPerLine = 3;
  const size_t level_count = config_->world_def()->levels()->size();
  for (size_t i = 0; i < level_count; i += kLevelPerLine) {
    flatui::StartGroup(flatui::kLayoutHorizontalCenter, 20);
    for (size_t j = 0; j < kLevelPerLine && i + j < level_count; ++j) {
      size_t index = i + j;
      auto event = ImageButtonWithLabel(
          *button_back_, 60, flatui::Margin(60, 35, 40, 50),
          config_->world_def()->levels()->Get(index)->name()->c_str());
      if (event & flatui::kEventWentUp && index != world_->level_index) {
        world_->level_index = index;
        LoadWorldDef(world_, world_def_);
      }
    }
    flatui::EndGroup();
  }
  flatui::EndGroup();
}

void GameMenuState::EmptyMenuBackground(
    fplbase::AssetManager& assetman, flatui::FontManager& fontman,
    fplbase::InputSystem& input, const std::function<void()>& gui_definition) {
  flatui::Run(assetman, fontman, input, [&]() {
    flatui::StartGroup(flatui::kLayoutOverlay, 0);
    flatui::StartGroup(flatui::kLayoutHorizontalTop, 0);
    // Background image.
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::vec2(0, -150));
    flatui::Image(*background_options_, 1400);
    flatui::EndGroup();

    gui_definition();

    flatui::EndGroup();
    flatui::EndGroup();
  });
}

bool GameMenuState::DisplayMessageBackButton() {
  flatui::StartGroup(flatui::kLayoutHorizontalBottom, 150);
  flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignBottom,
                        mathfu::vec2(0, -125));
  flatui::SetTextColor(kColorLightBrown);
  auto event = ImageButtonWithLabel(*button_back_, 60,
                                    flatui::Margin(60, 35, 40, 50), "Back");
  flatui::EndGroup();
  return event & flatui::kEventWentUp;
}

void GameMenuState::DisplayMessageUnlockable() {
  if (did_earn_unlockable_) {
    const int kBufferSize = 64;
    char buffer[kBufferSize];
    snprintf(buffer, kBufferSize, "%s unlocked!",
             earned_unlockable_.config->name()->c_str());
    flatui::Label(buffer, kScoreTextSize);
  }
}

MenuState GameMenuState::ScoreReviewMenu(fplbase::AssetManager& assetman,
                                         flatui::FontManager& fontman,
                                         fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateScoreReview;

  PushDebugMarker("ScoreReviewMenu");

  EmptyMenuBackground(assetman, fontman, input, [&]() {
    // Display the game end values, along with the score.
    flatui::SetTextColor(kColorBrown);
    flatui::StartGroup(flatui::kLayoutVerticalRight, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::vec2(-75, -200));
    flatui::Label("Patrons Fed:", kScoreSmallSize);
    flatui::Label("Sushi Thrown:", kScoreSmallSize);
    flatui::Label("Laps Finished:", kScoreSmallSize);
    flatui::Label("Final Score:", kScoreTextSize);
    flatui::EndGroup();
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::vec2(175, -200));
    const int kBufferSize = 64;
    char buffer[kBufferSize];
    snprintf(buffer, kBufferSize, "%d", patrons_fed_);
    flatui::Label(buffer, kScoreSmallSize);
    snprintf(buffer, kBufferSize, "%d", sushi_thrown_);
    flatui::Label(buffer, kScoreSmallSize);
    snprintf(buffer, kBufferSize, "%d", laps_finished_);
    flatui::Label(buffer, kScoreSmallSize);
    snprintf(buffer, kBufferSize, "%d", total_score_);
    flatui::Label(buffer, kScoreTextSize);
    flatui::EndGroup();

    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::vec2(0, 100));

    snprintf(buffer, kBufferSize, "%d XP earned", earned_xp_);
    flatui::Label(buffer, kScoreTextSize);
    if (did_earn_unlockable_) {
      snprintf(buffer, kBufferSize, "%s unlocked!",
               earned_unlockable_.config->name()->c_str());
      flatui::Label(buffer, kScoreTextSize);
    }
    if (world_->unlockables->remaining_locked_total() > 0) {
      snprintf(buffer, kBufferSize, "%d XP until next reward",
               world_->xp_system->xp_until_reward());
      flatui::Label(buffer, kScoreTextSize);
    } else {
      flatui::Label("Everything has been unlocked!", kScoreTextSize);
    }
    flatui::EndGroup();

    if (world_->admob_helper->rewarded_video_available() &&
        !world_->admob_helper->rewarded_video_watched() &&
        world_->admob_helper->GetRewardedVideoLocation() ==
        kRewardedVideoLocationScoreScreen) {
      flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
      flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                            mathfu::vec2(400, 10));
      flatui::SetTextColor(kColorLightBrown);
      auto event = ImageButtonWithLabel(
          *button_back_, 60, flatui::Margin(60, 35, 40, 50), "Bonus XP");
      if (event & flatui::kEventWentUp) {
        StartRewardedVideo();
      }
      flatui::EndGroup();
    }

    flatui::StartGroup(flatui::kLayoutHorizontalBottom, 150);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignBottom,
                          mathfu::vec2(0, -125));
    flatui::SetTextColor(kColorLightBrown);
    auto event = ImageButtonWithLabel(*button_back_, 60,
                                      flatui::Margin(60, 35, 40, 50), "Menu");
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateStart;
    }
    event = ImageButtonWithLabel(*button_back_, 60,
                                 flatui::Margin(60, 35, 40, 50), "Retry");
    if (event & flatui::kEventWentUp) {
      next_state = kMenuStateFinished;
    }
    flatui::EndGroup();
  });

  PopDebugMarker();  // ScoreReviewMenu

  return next_state;
}

MenuState GameMenuState::ReceivedInviteMenu(fplbase::AssetManager& assetman,
                                            flatui::FontManager& fontman,
                                            fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateReceivedInvite;

  PushDebugMarker("ReceivedInviteMenu");

  EmptyMenuBackground(assetman, fontman, input, [&]() {
    // Display a message indicating an invite was received.
    flatui::SetTextColor(kColorBrown);
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::kZeros2f);
    flatui::Label("Thanks for trying Zooshi!", kButtonSize);
    DisplayMessageUnlockable();
    flatui::EndGroup();

    if (DisplayMessageBackButton()) {
      next_state = kMenuStateStart;
    }
  });

  PopDebugMarker();  // ReceivedInviteMenu

  return next_state;
}

MenuState GameMenuState::SentInviteMenu(fplbase::AssetManager& assetman,
                                        flatui::FontManager& fontman,
                                        fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateSentInvite;

  PushDebugMarker("SentInviteMenu");

  EmptyMenuBackground(assetman, fontman, input, [&]() {
    // Display a message indicating invitations were sent.
    flatui::SetTextColor(kColorBrown);
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::kZeros2f);
    flatui::Label("Thanks for inviting others to Zooshi!", kButtonSize);
    DisplayMessageUnlockable();
    flatui::EndGroup();

    if (DisplayMessageBackButton()) {
      next_state = kMenuStateStart;
    }
  });

  PopDebugMarker();  // SentInviteMenu

  return next_state;
}

MenuState GameMenuState::ReceivedMessageMenu(fplbase::AssetManager& assetman,
                                             flatui::FontManager& fontman,
                                             fplbase::InputSystem& input) {
  MenuState next_state = kMenuStateReceivedMessage;

  PushDebugMarker("ReceivedMessageMenu");

  EmptyMenuBackground(assetman, fontman, input, [&]() {
    // Display the message that was received.
    flatui::SetTextColor(kColorBrown);
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::kZeros2f);
    flatui::Label(received_message_.c_str(), kButtonSize, kWrappedLabelSize,
                  flatui::kTextAlignmentCenter);
    flatui::EndGroup();

    if (DisplayMessageBackButton()) {
      next_state = kMenuStateStart;
    }
  });

  PopDebugMarker();  // ReceivedMessageMenu

  return next_state;
}

RewardedVideoState GameMenuState::RewardedVideoMenu(
    fplbase::AssetManager& assetman, flatui::FontManager& fontman,
    fplbase::InputSystem& input) {
  RewardedVideoState next_state = rewarded_video_state_;

  PushDebugMarker("AdMobVideoMenu");

  EmptyMenuBackground(assetman, fontman, input, [&]() {
    flatui::SetTextColor(kColorBrown);
    flatui::StartGroup(flatui::kLayoutVerticalCenter, 10);
    flatui::PositionGroup(flatui::kAlignCenter, flatui::kAlignCenter,
                          mathfu::kZeros2f);

    // We want to show different things based on the AdMob state.
    if (rewarded_video_state_ == kRewardedVideoStateDisplaying) {
      if (world_->admob_helper->rewarded_video_status() ==
          kAdMobStatusLoading) {
        flatui::Label("Loading video, please wait...", kButtonSize);
      } else {
        flatui::Label("Video loaded, please enjoy!", kButtonSize);
      }
    } else {
      if (world_->admob_helper->rewarded_video_watched()) {
        const char* label = menu_state_ == kMenuStateScoreReview
                                ? "A bonus has been granted!"
                                : "A bonus will be applied to your next game";
        flatui::Label(label, kButtonSize, kWrappedLabelSize,
                      flatui::kTextAlignmentCenter);
      } else {
        flatui::Label("The full video needs to be watched for the bonus",
                      kButtonSize, kWrappedLabelSize,
                      flatui::kTextAlignmentCenter);
      }
      if (DisplayMessageBackButton()) {
        next_state = kRewardedVideoStateIdle;
      }
    }

    flatui::EndGroup();
  });

  PopDebugMarker();  // AdMobVideoMenu

  return next_state;
}

}  // zooshi
}  // fpl
