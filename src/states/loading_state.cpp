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

#include "states/loading_state.h"

#include "assets_generated.h"
#include "fplbase/asset_manager.h"
#include "fplbase/material.h"
#include "fplbase/shader.h"
#include "fplbase/utilities.h"
#include "full_screen_fader.h"
#include "pindrop/pindrop.h"
#include "states/states.h"

namespace fpl {
namespace fpl_project {

using mathfu::kOnes3f;
using mathfu::kZeros3f;
using mathfu::kOnes4f;

static const WorldTime kLoadingScreenFadeInTime = 400;
static const WorldTime kLoadingScreenFadeOutTime = 200;

void LoadingState::Initialize(const AssetManifest& asset_manifest,
                              AssetManager* asset_manager,
                              pindrop::AudioEngine* audio_engine,
                              Shader* shader_textured, FullScreenFader* fader) {
  asset_manager_ = asset_manager;
  audio_engine_ = audio_engine;
  asset_manifest_ = &asset_manifest;
  shader_textured_ = shader_textured;
  loading_complete_ = false;
  fader_ = fader;
}

void LoadingState::AdvanceFrame(int delta_time, int* next_state) {
  const bool fade_out_complete = fader_->AdvanceFrame(delta_time);

  // Exit state when everything has finished loading.
  if (loading_complete_ && fade_out_complete) {
    *next_state = kGameStateGameMenu;
  }
}

void LoadingState::Render(Renderer* renderer) {
  // Ensure assets are instantiated after they've been loaded.
  // This must be called from the render thread.
  loading_complete_ =
      asset_manager_->TryFinalize() && audio_engine_->TryFinalize();

  // Get a handle to the loading material.
  const char* loading_material_name =
      asset_manifest_->loading_material()->c_str();
  Material* loading_material =
      asset_manager_->FindMaterial(loading_material_name);

  // Always clear the background.
  renderer->ClearFrameBuffer(kOnes4f);

  // Render nothing until the loading material itself has loaded.
  const Texture* texture = loading_material->textures()[0];
  if (!texture->id()) return;

  // In ortho space:
  //  - screen centered at (0,0)
  //  - unit of 1 is distance from middle to top of screen
  //  - bottom left is (-aspect_ratio, -1)
  //  - top right is (aspect_ratio, +1)
  //  - horizontal and vertical units are same distance on screen
  const vec2 res(renderer->window_size());
  const float aspect_ratio = res.x() / res.y();
  renderer->model_view_projection() =
      mat4::Ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);

  // Set up rendering context.
  renderer->color() = kOnes4f;
  loading_material->Set(*renderer);
  shader_textured_->Set(*renderer);

  // Render the overlay in front on the screen.
  // Don't scale the image so that the text remains crisp.
  const vec2 size = vec2(texture->size()) / res.y();
  const vec3 bottom_left(-size.x(), size.y(), 0.0f);
  const vec3 top_right(size.x(), -size.y(), 0.0f);
  Mesh::RenderAAQuadAlongX(bottom_left, top_right);

  const vec3 fade_bottom_left(-aspect_ratio, 1.0f, 0.0f);
  const vec3 fade_top_right(aspect_ratio, -1.0f, 0.0f);
  if (fader_->current_fade_time() == 0) {
    // If this is the first frame the textures have been loaded, start fade-in.
    fader_->Start(kLoadingScreenFadeInTime, kZeros3f, kFadeIn, fade_bottom_left,
                  fade_top_right);
  } else if (loading_complete_ && fader_->Finished()) {
    // If this is the first frame the textures have been loaded, start fade-out.
    fader_->Start(kLoadingScreenFadeOutTime, kZeros3f, kFadeOutThenIn,
                  fade_bottom_left, fade_top_right);
  }

  // Draw fader on top of everything.
  if (!fader_->Finished()) {
    fader_->Render(renderer);
  }
}

}  // fpl_project
}  // fpl
