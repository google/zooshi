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

#include "states/scene_lab_state.h"

#include "camera.h"
#include "mathfu/glsl_mappings.h"
#include "states/states.h"
#include "world.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

static const float kEditorViewportAngle =
    static_cast<float>(M_PI) / 3.0f;  // 60 degrees
void SceneLabState::Initialize(fplbase::Renderer* renderer,
                               fplbase::InputSystem* input_system,
                               scene_lab::SceneLab* scene_lab, World* world) {
  renderer_ = renderer;
  input_system_ = input_system;
  scene_lab_ = scene_lab;
  world_ = world;
  camera_ = new Camera();
  camera_->set_viewport_angle(kEditorViewportAngle);
  scene_lab->SetCamera(std::unique_ptr<corgi::CameraInterface>(camera_));
}

void SceneLabState::AdvanceFrame(corgi::WorldTime delta_time, int* next_state){
  scene_lab_->AdvanceFrame(delta_time);
  if (input_system_->GetButton(fplbase::FPLK_F11).went_down()) {
    scene_lab_->SaveScene();
  }

  if (input_system_->GetButton(fplbase::FPLK_F10).went_down() ||
      input_system_->GetButton(fplbase::FPLK_ESCAPE).went_down()) {
    scene_lab_->RequestExit();
  }
  if (scene_lab_->IsReadyToExit()) {
    *next_state = kGameStateGameplay;
  }
  if (input_system_->GetButton(fplbase::FPLK_1).went_down()) {
    *next_state = kGameStateGameplay;
    world_->is_single_stepping = true;
  }
  if (input_system_->GetButton(fplbase::FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }
  if (input_system_->GetButton(fplbase::FPLK_F8).went_down()) {
    world_->skip_rendermesh_rendering = !world_->skip_rendermesh_rendering;
  }
}

void SceneLabState::RenderPrep() {
  const corgi::CameraInterface* camera = scene_lab_->GetCamera();
  world_->world_renderer->RenderPrep(*camera, world_);
}

void SceneLabState::Render(fplbase::Renderer* renderer) {
  camera_->set_viewport_resolution(vec2(renderer->window_size()));

  const auto camera = scene_lab_->GetCamera();

  mat4 camera_transform = camera->GetTransformMatrix();
  renderer->set_color(mathfu::kOnes4f);
  renderer->DepthTest(true);
  renderer->set_model_view_projection(camera_transform);

  world_->river_component.UpdateRiverMeshes();

  if (world_->render_shadows()) {
    world_->world_renderer->RenderShadowMap(*camera, *renderer, world_);
  }
  world_->world_renderer->RenderWorld(*camera, *renderer, world_);
}

void SceneLabState::HandleUI(fplbase::Renderer* renderer) {
  scene_lab_->Render(renderer);
}

void SceneLabState::OnEnter(int /*previous_state*/) {
  scene_lab_->Activate();
}

void SceneLabState::OnExit(int /*next_state*/) { scene_lab_->Deactivate(); }

}  // zooshi
}  // fpl
