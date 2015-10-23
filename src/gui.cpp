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
#include "fplbase/utilities.h"
#include "states/game_menu_state.h"
#include "states/states_common.h"

namespace fpl {
namespace zooshi {

using gui::TextButton;

MenuState GameMenuState::StartMenu(AssetManager &assetman, FontManager &fontman,
                                   InputSystem &input) {
  MenuState next_state = kMenuStateStart;

  // Run() accepts a lambda function that is executed 2 times,
  // one for a layout pass and another one in a render pass.
  // In the lambda callback, the user can call Widget APIs to put widget in a
  // layout.
  gui::Run(assetman, fontman, input, [&]() {
    gui::StartGroup(gui::kLayoutHorizontalTop, 0);

    // Background image.
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::Image(*background_title_, 1400);
    gui::EndGroup();

    gui::SetTextColor(kColorBrown);

    // Menu items. Note that we are layering 2 layouts here
    // (background + menu items).
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::SetMargin(gui::Margin(200, 700, 200, 100));

    auto event = TextButton("Play Game", kMenuSize, gui::Margin(0));
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateFinished;
#ifdef ANDROID_GAMEPAD
      if (!gui::IsLastEventPointerType()) {
        next_state = kMenuStateGamepad;
      }
#endif
    }
    if (SupportsHeadMountedDisplay()) {
      event = TextButton("Cardboard", kMenuSize, gui::Margin(0));
      if (event & gui::kEventWentUp) {
        next_state = kMenuStateCardboard;
      }
    }
#ifdef USING_GOOGLE_PLAY_GAMES
    auto logged_in = gpg_manager_->LoggedIn();
    event = TextButton(*image_gpg_, gui::Margin(0, 50, 10, 0),
                       logged_in ? "Sign out" : "Sign in", kMenuSize,
                       gui::Margin(0), fpl::gui::kButtonPropertyImageLeft);
    if (event & gui::kEventWentUp) {
      gpg_manager_->ToggleSignIn();
    }
#endif
    event = TextButton("Options", kMenuSize, gui::Margin(0));
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateOptions;
    }
    event = TextButton("Quit", kMenuSize, gui::Margin(0));
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateQuit;
    }
    gui::EndGroup();
    gui::EndGroup();
  });

  return next_state;
}

MenuState GameMenuState::OptionMenu(AssetManager &assetman,
                                    FontManager &fontman, InputSystem &input) {
  MenuState next_state = kMenuStateOptions;

  // FlatUI UI definitions.
  gui::Run(assetman, fontman, input, [&]() {
    gui::StartGroup(gui::kLayoutOverlay, 0);
    gui::StartGroup(gui::kLayoutHorizontalTop, 0);
    // Background image. Note that we are layering 3 layouts here
    // (background + menu items + back button).
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    // Positioning the UI slightly above of the center.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(0, -150));
    gui::Image(*background_options_, 1400);
    gui::EndGroup();

    gui::SetTextColor(kColorBrown);

    // Menu items.
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
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
      default:
        break;
    }

    gui::EndGroup();

    // Foreground image (back button).
    gui::StartGroup(gui::kLayoutVerticalRight, 0);
    // Positioning the UI to up-left corner of the dialog.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(-450, -250));
    gui::SetTextColor(kColorLightBrown);

    auto event = ImageButtonWithLabel(*button_back_, 60,
                                      gui::Margin(60, 35, 40, 50), "Back");
    if (event & gui::kEventWentUp) {
      // Save data when you leave the audio page.
      if (options_menu_state_ == kOptionsMenuStateAudio) {
        SaveData();
      }
      if (options_menu_state_ == kOptionsMenuStateMain) {
        next_state = kMenuStateStart;
      } else {
        options_menu_state_ = kOptionsMenuStateMain;
      }
    }
    gui::EndGroup();
    gui::EndGroup();

    gui::EndGroup();  // Overlay group.
  });

  return next_state;
}

void GameMenuState::OptionMenuMain() {
  gui::SetMargin(gui::Margin(200, 300, 200, 100));

  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(0, 20, 0, 50));
  gui::SetTextColor(kColorBrown);
  gui::EndGroup();

  if (TextButton("About", kButtonSize, gui::Margin(2)) & gui::kEventWentUp) {
    options_menu_state_ = kOptionsMenuStateAbout;
  }

#ifdef USING_GOOGLE_PLAY_GAMES
  auto logged_in = gpg_manager_->LoggedIn();
  auto property = fpl::gui::kButtonPropertyImageLeft;

  if (!logged_in) {
    gui::SetTextColor(kColorLightGray);
    property |= fpl::gui::kButtonPropertyDisabled;
  }
  auto event = TextButton(*image_leaderboard_, gui::Margin(0, 25, 10, 0),
                          "Leaderboard", kButtonSize, gui::Margin(0), property);
  if (logged_in && (event & gui::kEventWentUp)) {
    // Fill in Leaderboard list.
    auto leaderboard_config = config_->gpg_config()->leaderboards();
    gpg_manager_->ShowLeaderboards(
        leaderboard_config->LookupByKey(kGPGDefaultLeaderboard)->id()->c_str());
  }

  event = TextButton(*image_achievements_, gui::Margin(0, 20, 0, 0),
                     "Achievements", kButtonSize, gui::Margin(0), property);
  if (logged_in && (event & gui::kEventWentUp)) {
    gpg_manager_->ShowAchievements();
  }
  gui::SetTextColor(kColorBrown);
#endif

  if (TextButton("Licenses", kButtonSize, gui::Margin(2)) & gui::kEventWentUp) {
    scroll_offset_ = mathfu::kZeros2f;
    options_menu_state_ = kOptionsMenuStateLicenses;
  }

  if (TextButton("Audio", kButtonSize, gui::Margin(2)) & gui::kEventWentUp) {
    options_menu_state_ = kOptionsMenuStateAudio;
  }
}

void GameMenuState::OptionMenuAbout() {
  gui::SetMargin(gui::Margin(200, 400, 200, 100));

  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(0, 20, 0, 55));
  gui::SetTextColor(kColorBrown);
  gui::Label("About", kButtonSize);
  gui::EndGroup();

  gui::SetTextColor(kColorDarkGray);
  gui::SetTextFont("fonts/NotoSans-Bold.ttf");

  gui::StartGroup(gui::kLayoutHorizontalCenter);
  gui::SetMargin(gui::Margin(50, 0, 0, 0));
  gui::StartGroup(gui::kLayoutVerticalCenter, 0, "scroll");
  gui::StartScroll(kScrollAreaSize, &scroll_offset_);
  gui::Label(about_text_.c_str(), 35, vec2(kScrollAreaSize.x(), 0));
  vec2 scroll_size = gui::GroupSize();
  gui::EndScroll();
  gui::EndGroup();

  // Normalize the scroll offset to use for the scroll bar value.
  auto scroll_height = (scroll_size.y() - kScrollAreaSize.y());
  if (scroll_height > 0) {
    auto scrollbar_value = scroll_offset_.y() / scroll_height;
    gui::ScrollBar(*scrollbar_back_, *scrollbar_foreground_,
                   vec2(35, kScrollAreaSize.y()),
                   kScrollAreaSize.y() / scroll_size.y(), "LicenseScrollBar",
                   &scrollbar_value);

    // Convert back the scroll bar value to the scroll offset.
    scroll_offset_.y() = scrollbar_value * scroll_height;
  }

  gui::EndGroup();
  gui::SetTextFont("fonts/RaviPrakash-Regular.ttf");
}

void GameMenuState::OptionMenuLicenses() {
  gui::SetMargin(gui::Margin(200, 300, 200, 100));

  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(0, 20, 0, 55));
  gui::SetTextColor(kColorBrown);
  gui::Label("Licenses", kButtonSize);
  gui::EndGroup();

  gui::SetTextColor(kColorDarkGray);
  gui::SetTextFont("fonts/NotoSans-Bold.ttf");

  gui::StartGroup(gui::kLayoutHorizontalCenter);
  gui::SetMargin(gui::Margin(50, 0, 0, 0));
  gui::StartGroup(gui::kLayoutVerticalCenter, 0, "scroll");
  // This check event makes the scroll group controllable with a gamepad.
  gui::StartScroll(kScrollAreaSize, &scroll_offset_);
  auto event = gui::CheckEvent(true);
  if (!gui::IsLastEventPointerType())
    gui::EventBackground(event);
  gui::Label(license_text_.c_str(), 25, vec2(kScrollAreaSize.x(), 0));
  vec2 scroll_size = gui::GroupSize();
  gui::EndScroll();
  gui::EndGroup();

  // Normalize the scroll offset to use for the scroll bar value.
  auto scroll_height = (scroll_size.y() - kScrollAreaSize.y());
  if (scroll_height > 0) {
    auto scrollbar_value = scroll_offset_.y() / scroll_height;
    gui::ScrollBar(*scrollbar_back_, *scrollbar_foreground_,
                   vec2(35, kScrollAreaSize.y()),
                   kScrollAreaSize.y() / scroll_size.y(), "LicenseScrollBar",
                   &scrollbar_value);

    // Convert back the scroll bar value to the scroll offset.
    scroll_offset_.y() = scrollbar_value * scroll_height;
  }

  gui::EndGroup();
  gui::SetTextFont("fonts/RaviPrakash-Regular.ttf");
}

void GameMenuState::OptionMenuAudio() {
  auto original_music_volume = slider_value_music_;
  auto original_effect_volume = slider_value_effect_;
  gui::SetMargin(gui::Margin(200, 200, 200, 100));

  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(0, 50, 0, 50));
  gui::SetTextColor(kColorBrown);
  gui::Label("Audio", kButtonSize);
  gui::EndGroup();

  gui::StartGroup(gui::kLayoutHorizontalCenter, 20);
  gui::Label("Music volume", kAudioOptionButtonSize);
  gui::SetMargin(gui::Margin(0, 40, 0, 0));
  gui::Slider(*slider_back_, *slider_knob_, vec2(400, 60), 0.6f, "MusicVolume",
              &slider_value_music_);

  gui::EndGroup();
  gui::StartGroup(gui::kLayoutHorizontalCenter, 20);
  gui::Label("Effect volume", kAudioOptionButtonSize);
  gui::SetMargin(gui::Margin(0, 40, 0, 0));
  auto event = gui::Slider(*slider_back_, *slider_knob_, vec2(400, 60), 0.6f,
                           "EffectVolume", &slider_value_effect_);
  if (event & gui::kEventWentUp || event & gui::kEventEndDrag) {
    // Play some effect.
    audio_engine_->PlaySound("fed_bird");
  }
  gui::EndGroup();

  if (original_music_volume != slider_value_music_ ||
      original_effect_volume != slider_value_effect_) {
    UpdateVolumes();
  }
}

}  // zooshi
}  // fpl
