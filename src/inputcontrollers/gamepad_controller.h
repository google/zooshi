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

#ifndef ZOOSHI_GAMEPAD_CONTROLLER_H
#define ZOOSHI_GAMEPAD_CONTROLLER_H

#include "camera.h"
#include "inputcontrollers/base_player_controller.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace zooshi {

class GamepadController : public BasePlayerController {
 public:
  GamepadController(ControllerType controller_type = kControllerGamepad)
      : BasePlayerController(controller_type) {}

  virtual void Update();

 protected:
  // Calculate the camera delta from button pushes.
  virtual mathfu::vec2 GetDelta() const;

 private:
  void UpdateFacing();
  virtual void UpdateButtons();
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_ANDROID_CARDBOARD_CONTROLLER_H
