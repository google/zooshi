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

#ifndef ZOOSHI_CONTROLLERINTERFACE_H
#define ZOOSHI_CONTROLLERINTERFACE_H

#include "mathfu/glsl_mappings.h"
#include "camera.h"

namespace fpl {
namespace fpl_project {

template <class T>
class LogicalInput {
 public:
  T Value() const { return current_value_; }
  bool HasChanged() const { return changed_; }
  void SetValue(T new_value) {
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

class BasePlayerController {
 public:
  BasePlayerController() {
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

  LogicalButton& Button(int index) { return buttons_[index]; }

  LogicalVector& facing() { return facing_; }

  LogicalVector& up() { return up_; }

 protected:
  LogicalButton buttons_[kLogicalButtonCount];
  LogicalVector facing_;
  LogicalVector up_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_CONTROLLERINTERFACE_H
