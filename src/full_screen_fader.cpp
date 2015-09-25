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

#include "full_screen_fader.h"

#include "fplbase/material.h"
#include "fplbase/mesh.h"
#include "fplbase/renderer.h"
#include "fplbase/shader.h"

namespace fpl {
namespace fpl_project {

FullScreenFader::FullScreenFader()
    : current_fade_time_(0),
      total_fade_time_(0),
      material_(NULL),
      shader_(NULL),
      opaque_(false) {}

void FullScreenFader::Init(Material* material, Shader* shader) {
  material_ = material;
  shader_ = shader;
}

// Starts the fade.
void FullScreenFader::Start(WorldTime fade_time, const vec4& color) {
  assert(material_ && shader_);
  current_fade_time_ = 0;
  total_fade_time_ = fade_time;
  color_ = color;
  opaque_ = false;
}

// Update the fade color returning true on the frame the overlay
// is opaque.
bool FullScreenFader::AdvanceFrame(int delta_time) {
  if (current_fade_time_ >= total_fade_time_) {
    return false;
  }
  current_fade_time_ += delta_time;
  bool opaque = current_fade_time_ > (total_fade_time_ / 2);
  bool turned_opaque_this_frame = !opaque_ && opaque;
  opaque_ = opaque;
  return turned_opaque_this_frame;
}

// Renders the fade overlay.
void FullScreenFader::Render(Renderer* renderer) {
  float t = std::min(static_cast<float>(current_fade_time_) /
                         static_cast<float>(total_fade_time_),
                     1.0f);
  float alpha = sin(t * M_PI);
  // Render the overlay in front on the screen.
  renderer->color() = vec4(color_.x(), color_.y(), color_.z(), alpha);
  renderer->model_view_projection() =
      mat4::Ortho(-1.0, 1.0, -1.0f, 1.0f, -1.0f, 1.0f);
  material_->Set(*renderer);
  shader_->Set(*renderer);
  // Clear depth buffer to prevent z-fight issues.
  renderer->ClearDepthBuffer();
  Mesh::RenderAAQuadAlongX(vec3(-1.0, 1.0, 0.0f), vec3(1.0, -1.0, 0.0f));
}

// Returns true when the fade is complete (overlay is transparent).
bool FullScreenFader::Finished() const {
  return current_fade_time_ >= total_fade_time_;
}

}  // namespace fpl_project
}  // namespace fpl
