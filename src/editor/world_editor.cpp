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

#include "world_editor.h"
#include "mathfu/utilities.h"

namespace fpl {
namespace editor {

void WorldEditor::Initialize(const WorldEditorConfig* config,
                             InputSystem* input_system) {
  config_ = config;
  input_system_ = input_system;
  input_controller_.set_input_system(input_system_);
  input_controller_.set_input_config(config_->input_config());
}

void WorldEditor::AdvanceFrame(WorldTime delta_time) {
  if (input_mode_ == kMoving) {
    // Allow the camera to look around and move.
    input_controller_.Update();
    camera_.set_facing(input_controller_.facing().Value());
    camera_.set_up(input_controller_.up().Value());

    mathfu::vec3 movement = GetMovement();
    camera_.set_position(camera_.position() + movement * (float)delta_time);
  } else if (input_mode_ == kEditing) {
    // We have an object selected to edit, so input now moves the object.
  }
}

void WorldEditor::Render(Renderer* /*renderer*/) {
  // Render any editor-specific things
}

void WorldEditor::Activate(const fpl_project::Camera& initial_camera) {
  // Set up the initial camera position.
  camera_ = initial_camera;
  camera_.set_viewport_angle(config_->viewport_angle_degrees() *
                             (M_PI / 180.0f));
  input_controller_.facing().SetValue(camera_.facing());
  input_controller_.up().SetValue(camera_.up());

  input_mode_ = kMoving;
}

mathfu::vec3 WorldEditor::GetMovement() {
  // Get a movement vector to move the user forward, up, or right.
  // Movement is always relative to the camera facing, but parallel to ground.
  float forward_speed = 0;
  float up_speed = 0;
  float right_speed = 0;
  // TODO(jsimantov): make the specific keys configurable?
  if (input_system_->GetButton(FPLK_w).is_down()) {
    forward_speed += config_->movement_speed();
  }
  if (input_system_->GetButton(FPLK_s).is_down()) {
    forward_speed -= config_->movement_speed();
  }
  if (input_system_->GetButton(FPLK_d).is_down()) {
    right_speed += config_->movement_speed();
  }
  if (input_system_->GetButton(FPLK_a).is_down()) {
    right_speed -= config_->movement_speed();
  }
  if (input_system_->GetButton(FPLK_r).is_down()) {
    up_speed += config_->movement_speed();
  }
  if (input_system_->GetButton(FPLK_f).is_down()) {
    up_speed -= config_->movement_speed();
  }

  // Translate the keypresses into movement parallel to the ground plane.
  mathfu::vec3 movement = mathfu::kZeros3f;
  // TODO(jsimantov): Currently assumes Z is up. This should be configurable.
  const float epsilon = 0.00001;
  mathfu::vec3 forward = {camera_.facing().xy(), 0};

  // If we're in gimbal lock, don't allow movement.
  if (forward.LengthSquared() > epsilon) {
    forward.Normalize();
    mathfu::vec3 up = mathfu::kAxisZ3f;
    mathfu::vec3 right =
        mathfu::vec3::CrossProduct(camera_.facing(), camera_.up());
    if (right.LengthSquared() > epsilon) {
      right.Normalize();
      movement += forward * forward_speed;
      movement += up * up_speed;
      movement += right * right_speed;
    }
  }
  return movement;
}

}  // namespace editor
}  // namespace fpl
