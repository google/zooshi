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

#include "gui.h"
#include "utilities.h"

namespace fpl {
namespace gui {

// Example how to create a button. We will provide convenient pre-made
// buttons like this, but it is expected many games will make custom buttons.
Event ImageButton(const char *texture_name, float size, const char *id) {
  StartGroup(LAYOUT_VERTICAL_LEFT, size, id);
  SetMargin(Margin(10));
  auto event = CheckEvent();
  if (event & EVENT_IS_DOWN)
    ColorBackground(vec4(1.0f, 1.0f, 1.0f, 0.5f));
  else if (event & EVENT_HOVER)
    ColorBackground(vec4(0.5f, 0.5f, 0.5f, 0.5f));
  Image(texture_name, size);
  EndGroup();
  return event;
}

void TestGUI(MaterialManager &matman, FontManager &fontman,
             InputSystem &input) {
  static float f = 0.0f;
  f += 0.04f;
  static bool show_about = false;
  static vec2i scroll_offset(mathfu::kZeros2i);

  auto click_about_example = [&](const char *id, bool about_on)
  {
    if (ImageButton("textures/guy.webp", 50, id) == EVENT_WENT_UP) {
      fpl::LogInfo("You clicked: %s", id);
      show_about = about_on;
    }
  };

  Run(matman, fontman, input, [&]() {
    PositionUI(1000, LAYOUT_HORIZONTAL_CENTER,
               LAYOUT_VERTICAL_RIGHT);
    StartGroup(LAYOUT_OVERLAY_CENTER, 0);
    StartGroup(LAYOUT_HORIZONTAL_TOP, 10);
    StartGroup(LAYOUT_VERTICAL_LEFT, 20);
    click_about_example("my_id1", true);
    StartGroup(LAYOUT_HORIZONTAL_TOP, 0);
    Label("Property T", 30);
    SetTextColor(mathfu::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    Label("Test ", 30);
    SetTextColor(mathfu::kOnes4f);
    Label("ffWAWÄテスト", 30);
    EndGroup();
    StartGroup(LAYOUT_VERTICAL_LEFT, 20);
    StartScroll(vec2(200, 100), &scroll_offset);
    auto splash_tex = matman.FindTexture("textures/guy.webp");
    ImageBackgroundNinePatch(*splash_tex,
                             vec4(0.2f, 0.2f, 0.8f, 0.8f));
    Label("The quick brown fox jumps over the lazy dog", 32);
    click_about_example("my_id4", true);
    Label("The quick brown fox jumps over the lazy dog", 24);
    Label("The quick brown fox jumps over the lazy dog", 20);
    EndScroll();
    EndGroup();
    EndGroup();
    StartGroup(LAYOUT_VERTICAL_CENTER, 40);
    click_about_example("my_id2", true);
    Image("textures/guy.webp", 40);
    Image("textures/guy.webp", 30);
    EndGroup();
    StartGroup(LAYOUT_VERTICAL_RIGHT, 0);
    SetMargin(Margin(100));
    Image("textures/guy.webp", 50);
    Image("textures/guy.webp", 40);
    Image("textures/guy.webp", 30);
    EndGroup();
    EndGroup();
    if (show_about) {
      StartGroup(LAYOUT_VERTICAL_LEFT, 20, "about_overlay");
      SetMargin(Margin(10));
      ColorBackground(vec4(0.5f, 0.5f, 0.0f, 1.0f));
      click_about_example("my_id3", false);
      Label("This is the about window! すし!", 32);
      Label("You should only be able to click on the", 24);
      Label("about button above, not anywhere else", 20);
      EndGroup();
    }
    EndGroup();
  });
}

}  // gui
}  // fpl
