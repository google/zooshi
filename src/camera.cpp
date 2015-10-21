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

#include "camera.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;

namespace fpl {
namespace zooshi {

static const float kDefaultViewportAngle = 0.7853975f;  // 45 degrees
static const vec2 kViewportResolution = vec2(640, 480);
static const float kDefaultViewportNearPlane = 1.0f;
static const float kDefaultViewportFarPlane = 500.0f;

Camera::Camera()
    : facing_(mathfu::kAxisY3f),
      up_(mathfu::kAxisZ3f),
      stereo_(false) {
  position_[0] = mathfu::kZeros3f;
  position_[1] = mathfu::kZeros3f;
  Initialize(kDefaultViewportAngle, kViewportResolution,
             kDefaultViewportNearPlane, kDefaultViewportFarPlane);
}

// returns a matrix representing our camera.
// This is the "VP" of the MVP matrix we'll usually want.
// (The M is the world transform of the model.)
mathfu::mat4 Camera::GetTransformMatrix(int32_t index) const {
  assert(index < (stereo_ ? 2 : 1));
  mat4 perspective_matrix_ = mat4::Perspective(
      viewport_angle_, viewport_resolution_.x() / viewport_resolution_.y(),
      viewport_near_plane_, viewport_far_plane_, 1.0f);

  // Subtract the facing vector because we need to be right handed.
  // TODO(amablue): add handedness to LookAt function (b/19229170)
  mat4 camera_matrix = mat4::LookAt(position_[index] - facing_,
                                    position_[index], up_);

  mat4 camera_transform = perspective_matrix_ * camera_matrix;

  return camera_transform;
}

// returns just the View matrix - doesn't do the projection transform.
mathfu::mat4 Camera::GetViewMatrix(int32_t index) const {
  assert(index < (stereo_ ? 2 : 1));
  // Subtract the facing vector because we need to be right handed.
  // TODO(amablue): add handedness to LookAt function (b/19229170)
  return mat4::LookAt(position_[index] - facing_, position_[index], up_);
}


}  // zooshi
}  // fpl
