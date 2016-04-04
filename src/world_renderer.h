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

#ifndef ZOOSHI_WORLD_RENDERER_H_
#define ZOOSHI_WORLD_RENDERER_H_

#include "world.h"

namespace fpl {
namespace zooshi {

struct World;

// Class that performs various rendering functions on a world state.
class WorldRenderer {
 public:
  // Initialize the world renderer.  Must be called before any other functions.
  void Initialize(World* world);

  // Load all of the shaders with current rendering options.
  void LoadAllShaders(World* world);

  // Call this before you call RenderWorld - it takes care of clearing
  // the frame, setting up the shadowmap, etc.
  void RenderPrep(const corgi::CameraInterface& camera,
                  World* world);

  // Render the shadowmap from the current camera.
  void RenderShadowMap(const corgi::CameraInterface& camera,
                       fplbase::Renderer& renderer, World* world);

  // Render the world, viewed from the current camera.
  void RenderWorld(const corgi::CameraInterface& camera,
                   fplbase::Renderer& renderer,
                   World* world);

  // Render the shadowmap into the world as a billboard, for debugging.
  void DebugShowShadowMap(const corgi::CameraInterface& camera,
                          fplbase::Renderer& renderer);

  // Sets the position of the light source in the world.  (Where the light is
  // located when generating shdaow maps, etc.)
  void SetLightPosition(const mathfu::vec3& light_pos) {
    light_camera_.set_position(light_pos);
  }

 private:
  fplbase::Shader* bank_shader_;
  fplbase::Shader* depth_shader_;
  fplbase::Shader* depth_skinned_shader_;
  fplbase::Shader* textured_shader_;
  fplbase::Shader* textured_lit_shader_;
  fplbase::Shader* textured_lit_cutout_shader_;
  fplbase::Shader* river_shader_;
  fplbase::Shader* skinned_shader_;
  Camera light_camera_;
  fplbase::RenderTarget shadow_map_;
  bool render_shadows_;
  bool apply_phong_;
  bool apply_specular_;

  // Create the shadowmap for the current worldstate.  Needs to be called
  // before RenderWorld.
  void CreateShadowMap(const corgi::CameraInterface& camera,
                       fplbase::Renderer& renderer, World* world);

  void SetFogUniforms(fplbase::Shader* shader, World* world);

  void SetLightingUniforms(fplbase::Shader* shader, World* world);

  bool ShouldReloadShaders(World* world);
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_WORLD_RENDERER_H_
