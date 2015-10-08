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

#include "inputcontrollers/gamepad_controller.h"
#include "mathfu/glsl_mappings.h"
#include "camera.h"
#include "fplbase/utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace zooshi {

void GamepadController::Update() {
  UpdateFacing();
  UpdateButtons();
}

void GamepadController::UpdateFacing() {
  facing_.Update();
  up_.Update();

  up_.SetValue(kCameraUp);
  vec2 delta(0.0f);

#ifdef ANDROID_GAMEPAD
  // TODO: Use only one gamepad per controller.
  for (auto it = input_system_->GamepadMap().begin();
       it != input_system_->GamepadMap().end(); ++it) {
    int device_id = it->first;
    fpl::Gamepad& gamepad = input_system_->GetGamepad(device_id);
    if (gamepad.GetButton(Gamepad::kLeft).is_down()) {
      delta.x()++;
    }
    if (gamepad.GetButton(Gamepad::kRight).is_down()) {
      delta.x()--;
    }
    if (gamepad.GetButton(Gamepad::kUp).is_down()) {
      delta.y()++;
    }
    if (gamepad.GetButton(Gamepad::kDown).is_down()) {
      delta.y()--;
    }
  }
#endif
  delta *= input_config_->gamepad_sensitivity();
  if (input_config_->invert_x()) {
    delta.x() *= -1.0f;
  }
  if (input_config_->invert_y()) {
    delta.y() *= -1.0f;
  }

  // We assume that the player is looking along the x axis, before
  // camera transformations are applied:
  vec3 facing_vector = facing_.Value();
  vec3 side_vector =
      quat::FromAngleAxis(-static_cast<float>(M_PI_2), mathfu::kAxisZ3f) *
      facing_vector;

  quat pitch_adjustment = quat::FromAngleAxis(delta.y(), side_vector);
  quat yaw_adjustment = quat::FromAngleAxis(delta.x(), mathfu::kAxisZ3f);

  facing_vector = pitch_adjustment * yaw_adjustment * facing_vector;

  facing_.SetValue(facing_vector);
}

void GamepadController::UpdateButtons() {
  for (int i = 0; i < kLogicalButtonCount; i++) {
    buttons_[i].Update();
  }
#ifdef ANDROID_GAMEPAD
  // TODO: Use only one gamepad per controller.
  for (auto it = input_system_->GamepadMap().begin();
       it != input_system_->GamepadMap().end(); ++it) {
    int device_id = it->first;
    fpl::Gamepad& gamepad = input_system_->GetGamepad(device_id);
    const fpl::Button& button = gamepad.GetButton(Gamepad::kButtonA);
    buttons_[kFireProjectile].SetValue(button.went_down());
  }
#endif
}

}  // fpl_base
}  // fpl
