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

#ifndef FPL_WORLD_EDITOR_H_
#define FPL_WORLD_EDITOR_H_

#include "camera.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "mathfu/vector_3.h"
#include "world_editor_generated.h"

#ifdef __ANDROID__
#include "inputcontrollers/android_cardboard_controller.h"
#else
#include "inputcontrollers/mouse_controller.h"
#endif

namespace fpl {
namespace editor {

class WorldEditor {
 public:
  void Initialize(const WorldEditorConfig* config, InputSystem* input_system);
  void AdvanceFrame(WorldTime delta_time);
  void Render(Renderer* renderer);

  void Activate(const fpl_project::Camera& initial_camera);

  const fpl_project::Camera* GetCamera() const { return &camera_; }

 private:
  enum { kMoving, kEditing } input_mode_;

  // get camera movement via W-A-S-D
  mathfu::vec3 GetMovement();

  const WorldEditorConfig* config_;
  InputSystem* input_system_;
#ifdef __ANDROID__
  fpl_project::AndroidCardboardController input_controller_;
#else
  fpl_project::MouseController input_controller_;
#endif
  fpl_project::Camera camera_;
};

}  // namespace editor
}  // namespace fpl

#endif  // FPL_WORLD_EDITOR_H_

