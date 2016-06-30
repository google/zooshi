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

#ifndef ZOOSHI_SCENE_LAB_STATE_H_
#define ZOOSHI_SCENE_LAB_STATE_H_

#include "camera.h"
#include "states/state_machine.h"
#include "world.h"

namespace scene_lab {

class SceneLab;

}  // namespace scene_lab

namespace fpl {

class InputSystem;
class Renderer;

namespace zooshi {

struct Config;
struct InputConfig;

class SceneLabState : public StateNode {
 public:
  SceneLabState()
      : renderer_(nullptr),
        world_(nullptr),
        input_system_(nullptr),
        camera_(nullptr),
        scene_lab_(nullptr) {}
  virtual ~SceneLabState() {}

  void Initialize(fplbase::Renderer* renderer,
                  fplbase::InputSystem* input_system,
                  scene_lab_corgi::CorgiAdapter* corgi_adapter, World* world);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void RenderPrep();
  virtual void Render(fplbase::Renderer* renderer);
  virtual void HandleUI(fplbase::Renderer* renderer);
  virtual void OnEnter(int previous_state);
  virtual void OnExit(int next_state);

 private:
  fplbase::Renderer* renderer_;
  World* world_;
  fplbase::InputSystem* input_system_;
  Camera* camera_;
  scene_lab::SceneLab* scene_lab_;
  scene_lab_corgi::CorgiAdapter* corgi_adapter_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_SCENE_LAB_STATE_H_
