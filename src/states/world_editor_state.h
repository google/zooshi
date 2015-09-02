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

#ifndef ZOOSHI_WORLD_EDITOR_STATE_H_
#define ZOOSHI_WORLD_EDITOR_STATE_H_

#include "camera.h"
#include "states/state_machine.h"
#include "world.h"

namespace fpl {

class InputSystem;
class Renderer;

namespace editor {

class WorldEditor;

}  // editor

namespace fpl_project {

struct Config;
struct InputConfig;

class WorldEditorState : public StateNode {
 public:
  WorldEditorState()
      : renderer_(nullptr),
        world_(nullptr),
        input_system_(nullptr),
        camera_(nullptr),
        world_editor_(nullptr) {}
  void Initialize(Renderer* renderer, InputSystem* input_system,
                  editor::WorldEditor* world_editor, World* world);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);

  virtual void OnEnter();
  virtual void OnExit();

 private:
  Renderer* renderer_;
  World* world_;
  InputSystem* input_system_;
  Camera* camera_;
  editor::WorldEditor* world_editor_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_WORLD_EDITOR_STATE_H_
