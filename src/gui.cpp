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

#include "fplbase/utilities.h"
#include "states/game_menu_state.h"
#include "states/states_common.h"

namespace fpl {
namespace fpl_project {

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
    }
#ifdef ANDROID_CARDBOARD
    event = TextButton("Cardboard", kMenuSize, gui::Margin(0));
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateCardboard;
    }
#endif  // ANDROID_CARDBOARD
    event = TextButton("Options", kMenuSize, gui::Margin(0));
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateOptions;
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
                       mathfu::vec2(0, -150));
    gui::SetMargin(gui::Margin(200, 400, 200, 100));

    if (show_licences_) {
      // Show 'Licenses' screen.
      OptionMenuLicenses();
    } else {
      OptionMenuMain();
    }

    gui::EndGroup();

    // Foreground image (back button).
    gui::StartGroup(gui::kLayoutVerticalRight, 0);
    // Positioning the UI to up-left corner of the dialog.
    gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                       mathfu::vec2(-360, -260));
    gui::SetTextColor(kColorLightBrown);
    if (ImageButtonWithLabel(*button_back_, 60, gui::Margin(50, 25, 30, 40),
                             "Back") &
        gui::kEventWentUp) {
      if (show_licences_) {
        show_licences_ = false;
      } else {
        next_state = kMenuStateStart;
      }
    }
    gui::EndGroup();
    gui::EndGroup();

    // Show 'About' dialog box.
    if (show_about_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "about_overlay");
      gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                         mathfu::kZeros2f);
      gui::ModalGroup();
      gui::SetMargin(gui::Margin(10));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::SetTextColor(vec4(1.0, 1.0, 1.0, 1.0));
      gui::Label("Zooshi is an awesome game.", 32);
      if (TextButton("Got it.", 32, gui::Margin(2)) & gui::kEventWentUp) {
        show_about_ = false;
      }
      gui::EndGroup();
    }

    // Show 'How to play' dialog box.
    if (show_how_to_play_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "how_to_play_overlay");
      gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                         mathfu::kZeros2f);
      gui::ModalGroup();
      gui::SetMargin(gui::Margin(10));
      gui::SetTextColor(vec4(1.0, 1.0, 1.0, 1.0));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("How to play.", 32);
      if (TextButton("OK", 32, gui::Margin(2)) & gui::kEventWentUp) {
        show_how_to_play_ = false;
      }
      gui::EndGroup();
    }

    // Show 'Audio' dialog box.
    if (show_audio_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "audio_overlay");
      gui::PositionGroup(gui::kAlignCenter, gui::kAlignCenter,
                         mathfu::kZeros2f);
      gui::ModalGroup();
      gui::SetMargin(gui::Margin(10));
      gui::SetTextColor(vec4(1.0, 1.0, 1.0, 1.0));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("Audio settings.", 32);
      gui::Label("Music volume: <slider>", 32);
      gui::Label("Effect volume: <slider>", 32);
      if (TextButton("OK", 32, gui::Margin(2)) & gui::kEventWentUp) {
        show_audio_ = false;
      }
      gui::EndGroup();
    }

    gui::EndGroup();  // Overlay group.
  });

  return next_state;
}

void GameMenuState::OptionMenuMain() {
  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(5));
  gui::SetTextColor(kColorBrown);
  gui::Label("Options", kMenuSize);
  gui::EndGroup();

  if (TextButton("About", kButtonSize, gui::Margin(2)) & gui::kEventWentUp) {
    show_about_ = true;
  }

  if (TextButton("Licenses", kButtonSize, gui::Margin(2)) &
      gui::kEventWentUp) {
    scroll_offset_ = mathfu::kZeros2i;
    show_licences_ = true;
  }

  if (TextButton("How to play", kButtonSize, gui::Margin(2)) &
      gui::kEventWentUp) {
    show_how_to_play_ = true;
  }

  if (TextButton("Audio", kButtonSize, gui::Margin(2)) & gui::kEventWentUp) {
    show_audio_ = true;
  }
}

void GameMenuState::OptionMenuLicenses() {
  gui::StartGroup(gui::kLayoutVerticalLeft, 50, "menu");
  gui::SetMargin(gui::Margin(5));
  gui::SetTextColor(kColorBrown);
  gui::Label("Licenses", kButtonSize);
  gui::EndGroup();

  gui::SetTextColor(kColorDarkGray);
  gui::SetTextFont("fonts/NotoSans-Bold.ttf");
  gui::StartGroup(gui::kLayoutVerticalLeft, 20, "scroll");
  gui::StartScroll(kScrollAreaSize, &scroll_offset_);
  gui::Label(license_text_.c_str(), 25, vec2(900, 0));
  gui::EndScroll();
  gui::EndGroup();
  gui::SetTextFont("fonts/RaviPrakash-Regular.ttf");
}
}  // fpl_project
}  // fpl
