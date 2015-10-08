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

// This file contains functionality common to multiple game states.

#ifndef ZOOSHI_STATES_STATES_COMMON_H_
#define ZOOSHI_STATES_STATES_COMMON_H_

#include "camera.h"
#include "world.h"

namespace fpl {
namespace zooshi {

// Update the camera to the location of the player in the given world.
void UpdateMainCamera(Camera* camera, World* world);

// Render the world monoscopically or stereoscopically.
void RenderWorld(Renderer& renderer, World* world, Camera& camera,
                 Camera* cardboard_camera, InputSystem* input_system);

// Add a text button.
gui::Event ImageButtonWithLabel(const Texture &tex, float size,
                                const gui::Margin &margin, const char *label);

}  // zooshi
}  // fpl

#endif  // ZOOSHI_STATES_STATES_COMMON_H_
