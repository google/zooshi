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

#ifndef ZOOSHI_PAUSE_STATE_H_
#define ZOOSHI_PAUSE_STATE_H_

#include "camera.h"
#include "fplbase/input.h"
#include "states/state_machine.h"

namespace fpl {

class AssetManager;
class FontManager;
class InputSystem;

struct AssetManager;

namespace fpl_project {
struct InputConfig;
class World;

class PauseState : public StateNode {
 public:
  virtual ~PauseState() {}
  void Initialize(InputSystem* input_system, World* world,
                  AssetManager* asset_manager, FontManager* font_manager);
  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);
  virtual void OnEnter();

 protected:
  World* world_;

  // IMGUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  InputSystem* input_system_;

  // IMGUI loads resources using AssetManager.
  AssetManager* asset_manager_;

  // IMGUI uses FontManager for a text rendering.
  FontManager* font_manager_;

  int next_state_;

  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_PAUSE_STATE_H_
