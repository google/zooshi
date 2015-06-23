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

#ifndef ZOOSHI_GAME_MENU_STATE_H_
#define ZOOSHI_GAME_MENU_STATE_H_

#include "camera.h"
#include "fplbase/input.h"
#include "imgui/imgui.h"
#include "state_machine.h"
#include "gameplay_state.h"

namespace fpl {

class InputSystem;

namespace fpl_project {

enum MenuState {
  kMenuStateStart,
  kMenuStateOptions,
  kMenuStateFinished,
};

class GameMenuState : public GameplayState {
 public:
  void Initialize(Renderer* renderer, InputSystem* input_system, World* world,
                  const InputConfig* input_config,
                  editor::WorldEditor* world_editor,
                  AssetManager* asset_manager, FontManager* font_namager);

  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);

 private:
  MenuState StartMenu(AssetManager& matman, FontManager& fontman,
                      InputSystem& input);
  MenuState OptionMenu(AssetManager& matman, FontManager& fontman,
                       InputSystem& input);
  gui::Event TextButton(const char* text, float size, const char* id);

  // IMGUI uses InputSystem for an input handling for a touch, gamepad,
  // mouse and keyboard.
  InputSystem* input_system_;

  // IMGUI loads resources using AssetManager.
  AssetManager* asset_manager_;

  // IMGUI uses FontManager for a text rendering.
  FontManager* font_manager_;

  // Menu state.
  MenuState menu_state_;

  // In-game menu state.
  bool show_about_;
  bool show_licences_;
  bool show_how_to_play_;
  bool show_audio_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_MENU_STATE_H_
