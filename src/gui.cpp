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
#include "game_menu_state.h"

namespace fpl {
namespace fpl_project {

gui::Event GameMenuState::TextButton(const char *text, float size,
                                     const char *id) {
  gui::StartGroup(gui::kLayoutVerticalLeft, size, id);
  gui::SetMargin(gui::Margin(10));
  auto event = gui::CheckEvent();
  if (event & gui::kEventIsDown) {
    gui::ColorBackground(vec4(1.0f, 1.0f, 1.0f, 0.5f));
  } else if (event & gui::kEventHover) {
    gui::ColorBackground(vec4(0.5f, 0.5f, 0.5f, 0.5f));
  }
  gui::Label(text, size);
  gui::EndGroup();
  return event;
}

MenuState GameMenuState::StartMenu(AssetManager &assetman, FontManager &fontman,
                                   InputSystem &input) {
  MenuState next_state = kMenuStateStart;

  // Run() accepts a lambda function that is executed 2 times,
  // one for a layout pass and another one in a render pass.
  // In the lambda callback, the user can call Widget APIs to put widget in a
  // layout.
  gui::Run(assetman, fontman, input, [&]() {
    gui::PositionUI(1000, gui::kLayoutHorizontalCenter,
                    gui::kLayoutVerticalCenter);
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::Label("Zooshi", 120);
    gui::SetMargin(gui::Margin(30));
    auto event = TextButton("START", 100, "button");
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateFinished;
    }
#ifdef ANDROID_CARDBOARD
    event = TextButton("CARDBOARD", 100, "button");
    if (event & gui::EVENT_WENT_UP) {
      next_state = kMenuStateCardboard;
    }
#endif  // ANDROID_CARDBOARD
    event = TextButton("OPTIONS", 100, "button");
    if (event & gui::kEventWentUp) {
      next_state = kMenuStateOptions;
    }
    gui::EndGroup();
  });

  return next_state;
}

MenuState GameMenuState::OptionMenu(AssetManager &assetman,
                                    FontManager &fontman, InputSystem &input) {
  MenuState next_state = kMenuStateOptions;

  gui::Run(assetman, fontman, input, [&]() {
    gui::PositionUI(1000, gui::kLayoutHorizontalCenter,
                    gui::kLayoutVerticalCenter);
    gui::StartGroup(gui::kLayoutOverlayCenter, 0);
    gui::StartGroup(gui::kLayoutVerticalCenter, 0);
    gui::Label("Options", 120);
    gui::SetMargin(gui::Margin(30));
    if (TextButton("About", 100, "button") & gui::kEventWentUp) {
      show_about_ = true;
    }
    if (TextButton("Licenses", 100, "button") & gui::kEventWentUp) {
      show_licences_ = true;
    }
    if (TextButton("How to play", 100, "button") & gui::kEventWentUp) {
      show_how_to_play_ = true;
    }
    if (TextButton("Audio", 100, "button") & gui::kEventWentUp) {
      show_audio_ = true;
    }
    if (TextButton("Back", 100, "button") & gui::kEventWentUp) {
      next_state = kMenuStateStart;
    }
    gui::EndGroup();

    // Show 'About' dialog box.
    if (show_about_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "about_overlay");
      gui::SetMargin(gui::Margin(10));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("Zooshi is an awesome game.", 32);
      if (TextButton("Got it.", 32, "button") & gui::kEventWentUp) {
        show_about_ = false;
      }
      gui::EndGroup();
    }

    // Show 'Licenses' dialog box.
    if (show_licences_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "licenses_overlay");
      gui::SetMargin(gui::Margin(10));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("Licenses.", 32);
      gui::Label("Licensing text.", 20);
      gui::Label("Licensing text.", 20);
      gui::Label("Licensing text.", 20);
      if (TextButton("OK", 32, "button") & gui::kEventWentUp) {
        show_licences_ = false;
      }
      gui::EndGroup();
    }

    // Show 'How to play' dialog box.
    if (show_how_to_play_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "how_to_play_overlay");
      gui::SetMargin(gui::Margin(10));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("How to play.", 32);
      if (TextButton("OK", 32, "button") & gui::kEventWentUp) {
        show_how_to_play_ = false;
      }
      gui::EndGroup();
    }

    // Show 'Audio' dialog box.
    if (show_audio_) {
      gui::StartGroup(gui::kLayoutVerticalCenter, 20, "audio_overlay");
      gui::SetMargin(gui::Margin(10));
      gui::ColorBackground(vec4(0.2f, 0.2f, 0.2f, 0.8f));
      gui::Label("Audio settings.", 32);
      gui::Label("Music volume: <slider>", 32);
      gui::Label("Effect volume: <slider>", 32);
      if (TextButton("OK", 32, "button") & gui::kEventWentUp) {
        show_audio_ = false;
      }
      gui::EndGroup();
    }

    gui::EndGroup();  // Overlay group.
  });

  return next_state;
}
}  // gui
}  // fpl
