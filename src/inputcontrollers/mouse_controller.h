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

#ifndef ZOOSHI_MOUSECONTROLLER_H
#define ZOOSHI_MOUSECONTROLLER_H

#include "mathfu/glsl_mappings.h"
#include "mathfu/constants.h"
#include "fplbase/input.h"
#include "inputcontrollers/base_player_controller.h"
#include "input_config_generated.h"

namespace fpl {
namespace fpl_project {

class MouseController : public BasePlayerController {
 public:
  MouseController() {}

  virtual void Update();

  void set_input_system(InputSystem* input_system) {
    input_system_ = input_system;
  }
  void set_input_config(const InputConfig* input_config) {
    input_config_ = input_config;
  }

 private:
  InputSystem* input_system_;

  void UpdateFacing();
  void UpdateButtons();

  const InputConfig* input_config_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_MOUSECONTROLLER_H
