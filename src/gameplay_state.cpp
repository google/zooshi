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

#include "gameplay_state.h"

#include "components/sound.h"
#include "component_library/transform.h"
#include "config_generated.h"
#include "events/play_sound.h"
#include "events_generated.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "states.h"
#include "world.h"

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

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

void GameplayState::Initialize(Renderer* renderer, InputSystem* input_system,
                               World* world, const InputConfig* input_config,
                               editor::WorldEditor* world_editor) {
  world_ = world;
  renderer_ = renderer;
  input_system_ = input_system;
  input_config_ = input_config;
  world_editor_ = world_editor;
}

static vec2 AdjustedMouseDelta(const vec2i& raw_delta,
                               const InputConfig& input_config) {
  vec2 delta(raw_delta);
  delta *= input_config.mouse_sensitivity();
  if (!input_config.invert_x()) {
    delta.x() *= -1.0f;
  }
  if (!input_config.invert_y()) {
    delta.y() *= -1.0f;
  }
  return delta;
}

void GameplayState::AdvanceFrame(int delta_time, int* next_state) {
  // Update the world.
  world_->motive_engine.AdvanceFrame(delta_time);
  world_->entity_manager.UpdateComponents(delta_time);

  // Calculate raft position and orientation.
  auto player = world_->player_component.begin()->entity;
  auto transform_component = &world_->transform_component;
  main_camera_.set_position(transform_component->WorldPosition(player));
  main_camera_.set_facing(
      transform_component->WorldOrientation(player).Inverse() *
      mathfu::kAxisY3f);
  auto player_data =
      world_->entity_manager.GetComponentData<PlayerData>(player);
  auto raft_orientation = transform_component->WorldOrientation(
      world_->entity_manager.GetComponent<ServicesComponent>()->raft_entity());
  main_camera_.set_up(raft_orientation.Inverse() * player_data->GetUp());

  if (input_system_->GetButton(fpl::FPLK_F9).went_down()) {
    world_->draw_debug_physics = !world_->draw_debug_physics;
  }

  // Switch States if necessary.
  if (world_editor_ && input_system_->GetButton(fpl::FPLK_F10).went_down()) {
    world_editor_->SetInitialCamera(main_camera_);
    *next_state = kGameStateWorldEditor;
  }

  // Exit the game.
  if (input_system_->GetButton(FPLK_ESCAPE).went_down()) {
    *next_state = kGameStateExit;
  }
}

void GameplayState::Render(Renderer* renderer) {
  bool stereoscopic = world_->is_in_cardboard;
  world_->render_mesh_component.RenderPrep(main_camera_);
  if (stereoscopic) {
    RenderStereoscopic(renderer);
  } else {
    RenderMonoscopic(renderer);
  }
}

void GameplayState::RenderMonoscopic(Renderer* renderer) {
  renderer->ClearFrameBuffer(kGreenishColor);
  mat4 camera_transform = main_camera_.GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  world_->render_mesh_component.RenderAllEntities(*renderer, main_camera_);

  if (world_->draw_debug_physics) {
    world_->physics_component.DebugDrawWorld(renderer, camera_transform);
  }
}

void GameplayState::RenderStereoscopic(Renderer* renderer) {
#ifdef ANDROID_CARDBOARD
  auto render_callback = [this, &renderer](const mat4& hmd_viewport_transform) {
    // Update the Cardboard camera with the translation changes from the given
    // transform, which contains the shifts for the eyes.
    const vec3 hmd_translation =
        (hmd_viewport_transform * mathfu::kAxisW4f * hmd_viewport_transform)
            .xyz();
    const vec3 corrected_translation =
        vec3(hmd_translation.x(), -hmd_translation.z(), hmd_translation.y());
    cardboard_camera_.set_position(main_camera_.position() +
                                   corrected_translation);
    cardboard_camera_.set_facing(main_camera_.facing());
    cardboard_camera_.set_up(main_camera_.up());

    auto camera_transform = cardboard_camera_.GetTransformMatrix();
    renderer->model_view_projection() = camera_transform;
    world_->render_mesh_component.RenderAllEntities(*renderer,
                                                    cardboard_camera_);
  };

  HeadMountedDisplayRender(input_system_, renderer, kGreenishColor,
                           render_callback);
#else
  (void)renderer;
#endif  // ANDROID_CARDBOARD
}

void GameplayState::OnEnter() {
  world_->player_component.set_active(true);
  input_system_->SetRelativeMouseMode(true);
#ifdef ANDROID_CARDBOARD
  input_system_->cardboard_input().ResetHeadTracker();
#endif  // ANDROID_CARDBOARD
}

}  // fpl_project
}  // fpl
