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

#ifndef ZOOSHI_LOADING_STATE_H_
#define ZOOSHI_LOADING_STATE_H_

#include "states/state_machine.h"

namespace fpl {

class AssetManager;
class Material;
class Shader;

namespace fpl_project {

struct AssetManifest;

class LoadingState : public StateNode {
 public:
  LoadingState()
      : loading_complete_(false),
        asset_manager_(nullptr),
        asset_manifest_(nullptr),
        shader_textured_(nullptr) {}
  virtual ~LoadingState() {}
  void Initialize(const AssetManifest& asset_manifest,
                  AssetManager* asset_manager, Shader* shader_textured);
  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(Renderer* renderer);
  virtual void OnEnter() {}
  virtual void OnExit() {}

 protected:
  // Set to true when the render thread detetects that all assets have been
  // loaded. The update thread then transitions to the next state.
  bool loading_complete_;

  // Holds the asynchronous loader thread that we are waiting for.
  // Also holds the loading texture that we display on screen.
  AssetManager* asset_manager_;

  // Holds the name of the loading texture that we display on screen.
  const AssetManifest* asset_manifest_;

  // Simple shader to draw the loading texture on screen.
  Shader* shader_textured_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_LOADING_STATE_H_
