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

#ifndef ZOOSHI_CAMERA_H
#define ZOOSHI_CAMERA_H

#include "mathfu/glsl_mappings.h"
#include "mathfu/constants.h"

class Camera {
 public:
  Camera();

  mathfu::mat4 GetTransformMatrix() const;

  void set_position(mathfu::vec3 position) { position_ = position; }
  mathfu::vec3 position() const { return position_; }

  void set_facing(const mathfu::vec3& facing) {
    assert(facing.LengthSquared() != 0);
    facing_ = facing;
  }
  const mathfu::vec3& facing() const { return facing_; }

  void set_up(const mathfu::vec3& up) {
    assert(up.LengthSquared() != 0);
    up_ = up;
  }
  const mathfu::vec3& up() const { return up_; }

  void set_viewport_angle(float viewport_angle) {
    viewport_angle_ = viewport_angle;
  }
  float viewport_angle() const { return viewport_angle_; }

  void set_viewport_resolution(mathfu::vec2 viewport_resolution) {
    viewport_resolution_ = viewport_resolution;
  }
  mathfu::vec2 viewport_resolution() const { return viewport_resolution_; }

  void set_viewport_near_plane(float viewport_near_plane) {
    viewport_near_plane_ = viewport_near_plane;
  }
  float viewport_near_plane() const { return viewport_near_plane_; }

  void set_viewport_far_plane(float viewport_far_plane) {
    viewport_far_plane_ = viewport_far_plane;
  }
  float viewport_far_plane() const { return viewport_far_plane_; }

  void Init(float viewport_angle, mathfu::vec2 viewport_resolution,
            float viewport_near_plane, float viewport_far_plane) {
    viewport_angle_ = viewport_angle;
    viewport_resolution_ = viewport_resolution;
    viewport_near_plane_ = viewport_near_plane;
    viewport_far_plane_ = viewport_far_plane;
  }

 private:
  mathfu::vec3 position_;
  mathfu::vec3 facing_;
  mathfu::vec3 up_;
  float viewport_angle_;
  mathfu::vec2 viewport_resolution_;
  float viewport_near_plane_;
  float viewport_far_plane_;
};

#endif  // ZOOSHI_CAMERA_H
