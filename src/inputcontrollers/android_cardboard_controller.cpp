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

#include "inputcontrollers/android_cardboard_controller.h"
#include "mathfu/glsl_mappings.h"
#include "camera.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace zooshi {

void AndroidCardboardController::Update() {
  UpdateOrientation();
  UpdateButtons();
}

void AndroidCardboardController::UpdateOrientation() {
  facing_.Update();
  up_.Update();

#ifdef ANDROID_CARDBOARD
  // Cardboard uses a different coordinate space than we use, so we have to
  // remap the axes and swap the handedness before we can use the
  // vectors as our facing/up vectors:
  const vec3 cardboard_forward = input_system_->cardboard_input().forward();
  const vec3 forward = vec3(cardboard_forward.x(), -cardboard_forward.z(),
                            cardboard_forward.y());
  const vec3 cardboard_up = input_system_->cardboard_input().up();
  const vec3 up = vec3(cardboard_up.x(), -cardboard_up.z(), cardboard_up.y());
  facing_.SetValue(forward);
  up_.SetValue(up);
#endif  // ANDROID_CARDBOARD
}

void AndroidCardboardController::UpdateButtons() {
  for (int i = 0; i < kLogicalButtonCount; i++) {
    buttons_[i].Update();
  }
  // Save the position of the last touch.
  auto button = &input_system_->GetPointerButton(0);
  const InputPointer* pointer = &input_system_->get_pointers()[0];
  if (button->went_down()) {
    last_position_ = pointer->mousepos;
  }
#ifdef ANDROID_CARDBOARD
  buttons_[kFireProjectile].SetValue(
      input_system_->cardboard_input().triggered());
#endif  // ANDROID_CARDBOARD
}

}  // fpl_base
}  // fpl
