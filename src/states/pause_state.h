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
#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/input.h"
#include "pindrop/pindrop.h"
#include "states/state_machine.h"
#include "states/states.h"
#include "world.h"

namespace fpl {
namespace fpl_project {
struct InputConfig;

class PauseState : public StateNode {
 public:
  virtual ~PauseState() {}
  void Initialize(InputSystem* input_system, World* world, const Config* config,
                  AssetManager* asset_manager, FontManager* font_manager,
                  pindrop::AudioEngine* audio_engine);
  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void RenderPrep(Renderer* renderer);
  virtual void Render(Renderer* renderer);
  virtual void HandleUI(Renderer* renderer);
  virtual void OnEnter(int previous_state);

 protected:
  GameState PauseMenu(AssetManager& assetman, FontManager& fontman,
                      InputSystem& input);

  World* world_;

  // IMGUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  InputSystem* input_system_;

  // IMGUI loads resources using AssetManager.
  AssetManager* asset_manager_;

  // IMGUI uses FontManager for a text rendering.
  FontManager* font_manager_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_continue_;
  pindrop::SoundHandle sound_exit_;

  // Texture used in paused UI .
  Texture* background_paused_;

  // The next menu state.
  GameState next_state_;

  Camera main_camera_;
#ifdef ANDROID_CARDBOARD
  Camera cardboard_camera_;
#endif
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_PAUSE_STATE_H_
