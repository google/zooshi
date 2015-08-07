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
#include "pindrop/pindrop.h"
#include "states/state_machine.h"

namespace fpl {

class Renderer;
class InputSystem;

namespace editor {

class WorldEditor;

}  // editor

namespace fpl_project {

struct InputConfig;
class World;
class WorldRenderer;

class GameplayState : public StateNode {
 public:
  void Initialize(InputSystem* input_system, World* world,
                  const InputConfig* input_config,
                  editor::WorldEditor* world_editor,
                  pindrop::AudioEngine* audio_engine);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);
  virtual void OnEnter();
  virtual void OnExit();

 protected:
  World* world_;

  const InputConfig* input_config_;

  InputSystem* input_system_;

  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif

  // This is needed here so that when transitioning into the editor the camera
  // location can be initialized.
  editor::WorldEditor* world_editor_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_pause_;

  // This will eventually be removed when there are events to handle this logic.
  pindrop::SoundHandle music_gameplay_;
  pindrop::Channel music_channel_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAMEPLAY_STATE_H_
