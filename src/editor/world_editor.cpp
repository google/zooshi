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

WorldEditor::WorldEditor() : is_active_(false) {}

void WorldEditor::Initialize(const WorldEditorConfig* config,
                             InputSystem* input_system) {
  config_ = config;
  input_system_ = input_system;
}

void WorldEditor::Update(WorldTime /*delta_time*/) {
  // not yet implmented
}

void WorldEditor::Render(Renderer* /*renderer*/) {
  // Render any editor-specific things
}

void WorldEditor::Activate(const fpl_project::Camera& initial_camera) {
  is_active_ = true;
  // Set up the initial camera position.
  camera_ = initial_camera;
  camera_.set_viewport_angle(config_->viewport_angle_degrees() *
                             (M_PI / 180.0f));
}

void WorldEditor::Deactivate() { is_active_ = false; }

}  // namespace editor
}  // namespace fpl
