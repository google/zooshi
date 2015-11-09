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

#ifndef ZOOSHI_BASE_PLAYER_CONTROLLER_H
#define ZOOSHI_BASE_PLAYER_CONTROLLER_H

#include "camera.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace zooshi {

template <class T>
class LogicalInput {
 public:
  T Value() const { return current_value_; }
  bool HasChanged() const { return changed_; }
  void SetValue(const T& new_value) {
    current_value_ = new_value;
    changed_ = true;
  }

  void Update() {
    previous_value_ = current_value_;
    changed_ = false;
  }

 private:
  bool changed_;
  T current_value_;
  T previous_value_;
};

class LogicalButton : public LogicalInput<bool> {};
class LogicalVector : public LogicalInput<mathfu::vec3> {};

enum LogicalButtonTypes {
  kFireProjectile = 0,

  kLogicalButtonCount  // This needs to be last.
};

enum ControllerType { kControllerDefault, kControllerGamepad };

class BasePlayerController {
 public:
  BasePlayerController(ControllerType controller_type = kControllerDefault)
      : last_position_(-1), controller_type_(controller_type), enabled_(true) {
    facing_.SetValue(kCameraForward);
    facing_.Update();
    up_.SetValue(kCameraUp);
    up_.Update();
    for (int i = 0; i < kLogicalButtonCount; i++) {
      buttons_[i].SetValue(false);
      buttons_[i].Update();
    }
  }

  virtual void Update() = 0;

  void ResetFacing() {
    facing_.SetValue(kCameraForward);
    up_.SetValue(kCameraUp);
  }

  LogicalButton& Button(int index) { return buttons_[index]; }

  LogicalVector& facing() { return facing_; }

  LogicalVector& up() { return up_; }

  const mathfu::vec2i& last_position() const { return last_position_; }

  ControllerType controller_type() const { return controller_type_; }

  void set_input_system(InputSystem* input_system) {
    input_system_ = input_system;
  }
  void set_input_config(const InputConfig* input_config) {
    input_config_ = input_config;
  }

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

 protected:
  LogicalButton buttons_[kLogicalButtonCount];
  LogicalVector facing_;
  LogicalVector up_;

  // The last position that the controller handled, in screen space, with 0,0
  // as top-left. Note that having negative values indicates not having a
  // position defined.
  mathfu::vec2i last_position_;

  ControllerType controller_type_;

  InputSystem* input_system_;
  const InputConfig* input_config_;

  bool enabled_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_BASE_PLAYER_CONTROLLER_H
