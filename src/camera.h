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

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "component_library/camera_interface.h"

namespace fpl {
namespace fpl_project {
static const mathfu::vec3 kCameraForward = mathfu::kAxisY3f;
static const mathfu::vec3 kCameraSide = mathfu::kAxisX3f;
static const mathfu::vec3 kCameraUp = mathfu::kAxisZ3f;

class Camera : public fpl::CameraInterface {
 public:
  Camera();

  // returns the View/Projection matrix:
  virtual mathfu::mat4 GetTransformMatrix() const;

  // returns just the V matrix:
  virtual mathfu::mat4 GetViewMatrix() const;

  virtual void set_position(mathfu::vec3 position) { position_ = position; }
  virtual mathfu::vec3 position() const { return position_; }

  virtual void set_facing(const mathfu::vec3& facing) {
    assert(facing.LengthSquared() != 0);
    facing_ = facing;
  }
  virtual const mathfu::vec3& facing() const { return facing_; }

  virtual void set_up(const mathfu::vec3& up) {
    assert(up.LengthSquared() != 0);
    up_ = up;
  }
  virtual const mathfu::vec3& up() const { return up_; }

  void set_viewport_angle(float viewport_angle) {
    viewport_angle_ = viewport_angle;
  }
  virtual float viewport_angle() const { return viewport_angle_; }

  virtual void set_viewport_resolution(mathfu::vec2 viewport_resolution) {
    viewport_resolution_ = viewport_resolution;
  }
  virtual mathfu::vec2 viewport_resolution() const {
    return viewport_resolution_;
  }

  virtual void set_viewport_near_plane(float viewport_near_plane) {
    viewport_near_plane_ = viewport_near_plane;
  }
  virtual float viewport_near_plane() const { return viewport_near_plane_; }

  virtual void set_viewport_far_plane(float viewport_far_plane) {
    viewport_far_plane_ = viewport_far_plane;
  }
  virtual float viewport_far_plane() const { return viewport_far_plane_; }

  void Initialize(float viewport_angle, mathfu::vec2 viewport_resolution,
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

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_CAMERA_H
