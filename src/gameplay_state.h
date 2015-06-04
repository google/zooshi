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

#ifndef ZOOSHI_GAMEPLAY_STATE_H_
#define ZOOSHI_GAMEPLAY_STATE_H_

#include "camera.h"
#include "fplbase/input.h"
#include "state_machine.h"

namespace fpl {

class Renderer;
class InputSystem;

namespace editor {

class WorldEditor;

}  // editor

namespace fpl_project {

struct InputConfig;
class World;

class GameplayState : public StateNode {
 public:
  void Initialize(Renderer* renderer, InputSystem* input_system, World* world,
                  const InputConfig* input_config,
                  editor::WorldEditor* world_editor);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);
  virtual void OnEnter();

 protected:
  void RenderMonoscopic(Renderer* renderer);
  void RenderStereoscopic(Renderer* renderer);

  World* world_;

  Renderer* renderer_;

  const InputConfig* input_config_;

  InputSystem* input_system_;

  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif

  // This is needed here so that when transitioning into the editor the camera
  // location can be initialized.
  editor::WorldEditor* world_editor_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAMEPLAY_STATE_H_
