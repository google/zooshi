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

#include "world_editor_state.h"

#include "mathfu/glsl_mappings.h"
#include "states.h"
#include "world.h"
#include "world_editor_generated.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const vec4 kGreenishColor(0.05f, 0.2f, 0.1f, 1.0f);

void WorldEditorState::Initialize(Renderer* renderer, InputSystem* input_system,
                                  editor::WorldEditor* world_editor,
                                  World* world) {
  renderer_ = renderer;
  input_system_ = input_system;
  world_editor_ = world_editor;
  world_ = world;
}

void WorldEditorState::AdvanceFrame(WorldTime delta_time, int* next_state) {
  world_editor_->AdvanceFrame(delta_time);
  if (input_system_->GetButton(FPLK_F10).went_down() ||
      input_system_->GetButton(FPLK_ESCAPE).went_down()) {
    *next_state = kGameStateGameplay;
  }
  if (input_system_->GetButton(FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }
}

void WorldEditorState::Render(Renderer* renderer) {
  const Camera* camera = world_editor_->GetCamera();

  renderer->ClearFrameBuffer(kGreenishColor);
  mat4 camera_transform = camera->GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  world_->render_mesh_component.RenderAllEntities(*renderer, *camera);
  world_->shadow_component_.RenderAllEntities(camera_transform);

  world_editor_->Render(renderer);

  if (world_->draw_debug_physics) {
    world_->physics_component.DebugDrawWorld(renderer, camera_transform);
  }
}

}  // fpl_project
}  // fpl

