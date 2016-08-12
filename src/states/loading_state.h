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

#include "config_generated.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"  // For ANDROID_HMD definition.
#include "states/state_machine.h"

namespace pindrop {
class AudioEngine;
}

namespace fpl {

namespace zooshi {

struct AssetManifest;
class FullScreenFader;
struct World;

class LoadingState : public StateNode {
 public:
  LoadingState()
      : loading_complete_(false),
        asset_manager_(nullptr),
        asset_manifest_(nullptr),
        shader_textured_(nullptr),
        world_(nullptr),
        banner_rotation_(0.0f){}
  virtual ~LoadingState() {}
  void Initialize(fplbase::InputSystem* input_system, World* world,
                  const AssetManifest& asset_manifest,
                  fplbase::AssetManager* asset_manager,
                  pindrop::AudioEngine* audio_engine,
                  fplbase::Shader* shader_textured, FullScreenFader* fader);
  virtual void AdvanceFrame(int delta_time, int* next_state);
  virtual void Render(fplbase::Renderer* renderer);
  virtual void OnEnter(int previous_state);

 protected:
  // Set to true when the render thread detetects that all assets have been
  // loaded. The update thread then transitions to the next state.
  bool loading_complete_;

  // Holds the texture asynchronous loader thread that we are waiting for.
  // Also holds the loading texture that we display on screen.
  fplbase::AssetManager* asset_manager_;

  // Holds the audio asynchronous loader thread that we are waiting for.
  pindrop::AudioEngine* audio_engine_;

  // Holds the name of the loading texture that we display on screen.
  const AssetManifest* asset_manifest_;

  // Simple shader to draw the loading texture on screen.
  fplbase::Shader* shader_textured_;

  FullScreenFader* fader_;

  World *world_;

  // The input system so that we can get input.
  fplbase::InputSystem* input_system_;

  // Rotation around Y of the banner when a VR loading screen is in use.
  float banner_rotation_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_LOADING_STATE_H_
