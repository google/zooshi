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
namespace zooshi {
struct InputConfig;

class PauseState : public StateNode {
 public:
  virtual ~PauseState() {}
  void Initialize(fplbase::InputSystem* input_system, World* world,
                  const Config* config,
                  fplbase::AssetManager* asset_manager,
                  flatui::FontManager* font_manager,
                  pindrop::AudioEngine* audio_engine);
  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void RenderPrep();
  virtual void Render(fplbase::Renderer* renderer);
  virtual void HandleUI(fplbase::Renderer* renderer);
  virtual void OnEnter(int previous_state);

 protected:
  GameState PauseMenu(fplbase::AssetManager& assetman,
                      flatui::FontManager& fontman,
                      fplbase::InputSystem& input);

  World* world_;

  // IMGUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  fplbase::InputSystem* input_system_;

  // IMGUI loads resources using AssetManager.
  fplbase::AssetManager* asset_manager_;

  // IMGUI uses FontManager for a text rendering.
  flatui::FontManager* font_manager_;

  // The audio engine, so that sound effects can be played.
  pindrop::AudioEngine* audio_engine_;

  // Cache the common sounds that are going to be played.
  pindrop::SoundHandle sound_continue_;
  pindrop::SoundHandle sound_exit_;

  // Texture used in paused UI .
  fplbase::Texture* background_paused_;

  // Asset manifest used to retrieve configurations.
  const Config* config_;

  // The next menu state.
  GameState next_state_;

  Camera main_camera_;
#if FPLBASE_ANDROID_VR
  Camera cardboard_camera_;
#endif
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_PAUSE_STATE_H_
