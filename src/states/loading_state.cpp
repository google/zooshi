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
#include "states/states.h"

namespace fpl {
namespace fpl_project {

void LoadingState::Initialize(const AssetManifest& asset_manifest,
                              AssetManager* asset_manager,
                              Shader* shader_textured) {
  asset_manager_ = asset_manager;
  asset_manifest_ = &asset_manifest;
  shader_textured_ = shader_textured;
  loading_complete_ = false;
}

void LoadingState::AdvanceFrame(int /*delta_time*/, int* next_state) {
  // Exit state when everything has finished loading.
  if (loading_complete_) {
    *next_state = kGameStateGameMenu;
  }
}

void LoadingState::Render(Renderer* renderer) {
  // Ensure assets are instantiated after they've been loaded.
  // This must be called from the render thread.
  if (asset_manager_->TryFinalize()) {
    loading_complete_ = true;
  }

  // Get a handle to the loading material.
  const char* loading_material_name =
      asset_manifest_->loading_material()->c_str();
  Material* loading_material =
      asset_manager_->FindMaterial(loading_material_name);

  // Render nothing until the loading material itself has loaded.
  if (!loading_material->textures()[0]->id()) return;

  // In ortho space:
  //  - screen centered at (0,0)
  //  - unit of 1 is distance from middle to top of screen
  //  - bottom left is (-aspect_ratio, -1)
  //  - top right is (aspect_ratio, +1)
  //  - horizontal and vertical units are same distance on screen
  const vec2 res = vec2(renderer->window_size());
  const float aspect_ratio = res.x() / res.y();
  renderer->model_view_projection() =
      mat4::Ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);

  // Set up rendering context.
  renderer->color() = mathfu::kOnes4f;
  loading_material->Set(*renderer);
  shader_textured_->Set(*renderer);

  // Render the overlay in front on the screen.
  Mesh::RenderAAQuadAlongX(vec3(-0.5f, 0.5f, 0.0f), vec3(0.5f, -0.5f, 0.0f));
}

}  // fpl_project
}  // fpl
