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

#include "states/world_editor_state.h"

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
namespace fpl_project {

static const float kEditorViewportAngle = M_PI / 3.0;  // 60 degrees
void WorldEditorState::Initialize(Renderer* renderer, InputSystem* input_system,
                                  editor::WorldEditor* world_editor,
                                  World* world) {
  renderer_ = renderer;
  input_system_ = input_system;
  world_editor_ = world_editor;
  world_ = world;
  camera_ = new Camera();
  camera_->set_viewport_angle(kEditorViewportAngle);
  world_editor->SetCamera(std::unique_ptr<CameraInterface>(camera_));
}

void WorldEditorState::AdvanceFrame(WorldTime delta_time, int* next_state) {
  world_editor_->AdvanceFrame(delta_time);
  if (input_system_->GetButton(FPLK_F11).went_down()) {
    world_editor_->SaveWorld();
  }

  if (input_system_->GetButton(FPLK_F10).went_down() ||
      input_system_->GetButton(FPLK_ESCAPE).went_down()) {
    *next_state = kGameStateGameplay;
  }
  if (input_system_->GetButton(FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }
  if (input_system_->GetButton(FPLK_F8).went_down()) {
    world_->skip_rendermesh_rendering = !world_->skip_rendermesh_rendering;
  }
}

void WorldEditorState::Render(Renderer* renderer) {
  camera_->set_viewport_resolution(vec2(renderer->window_size()));

  const CameraInterface* camera = world_editor_->GetCamera();

  mat4 camera_transform = camera->GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  world_->world_renderer->RenderPrep(*camera, *renderer, world_);
  world_->world_renderer->RenderWorld(*camera, *renderer, world_);

  world_editor_->Render(renderer);
}

void WorldEditorState::OnEnter() { world_editor_->Activate(); }

void WorldEditorState::OnExit() { world_editor_->Deactivate(); }

}  // fpl_project
}  // fpl
