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

#ifndef ZOOSHI_ONSCREEN_CONTROLLER_H
#define ZOOSHI_ONSCREEN_CONTROLLER_H

#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/material.h"
#include "inputcontrollers/gamepad_controller.h"
#include "mathfu/vector_2.h"
#include "mathfu/vector_4.h"

namespace fpl {
namespace zooshi {

class OnscreenController : public GamepadController {
 private:
  // OnscreenControllerUI updates delta_.
  friend class OnscreenControllerUI;

 public:
  OnscreenController() : GamepadController(kControllerDefault), delta_(0.0f) {}

  virtual ~OnscreenController() {}

 protected:
  // Calculate the camera delta from onscreen button pushes.
  virtual mathfu::vec2 GetDelta() const { return delta_; }

 private:
  virtual void UpdateButtons();

  // Distance to move the player this frame.
  mathfu::vec2 delta_;
};

// Renders UI for OnscreenController and updating
// OnscreenController::delta_ with the amount to move the player.
//
// The UI rendering is separated from the controller logic so that it can
// be rendered on a different thread to the game simulation.
class OnscreenControllerUI {
 public:
  OnscreenControllerUI()
      : controller_(nullptr),
        location_(mathfu::kZeros2f),
        base_texture_(nullptr),
        top_texture_(nullptr),
        visible_(false) {}

  // Render the UI.
  void Update(AssetManager* asset_manager, FontManager* font_manager,
              const mathfu::vec2i& window_size);

  // Base of the controller (e.g base of the joystick).
  void set_base_texture(Texture* base_texture) { base_texture_ = base_texture; }

  // Top of the controller (e.g top of the joystick).
  void set_top_texture(Texture* top_texture) { top_texture_ = top_texture; }

  // Associate this with an OnscreenController object which is updated by this
  // UI object.
  void set_controller(OnscreenController* controller) {
    controller_ = controller;
  }

 private:
  // Calculate the range of movement given the current magnitude,
  // dead-zone along an axis and sensitivity applied to movement.
  static float CalculateDelta(const float magnitude, const float dead_zone,
                              const float sensitivity);

 protected:
  OnscreenController* controller_;
  mathfu::vec2 location_;
  Texture* base_texture_;
  Texture* top_texture_;
  bool visible_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_ONSCREEN_CONTROLLER_H
