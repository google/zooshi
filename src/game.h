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

#ifndef ZOOSHI_GAME_H
#define ZOOSHI_GAME_H

#include "camera.h"
#include "game_state.h"
#include "entity/entity_manager.h"
#include "flatbuffers/flatbuffers.h"
#include "fplbase/input.h"
#include "fplbase/material_manager.h"
#include "fplbase/renderer.h"
#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"
#include "rail_def_generated.h"
#include "config_generated.h"

namespace fpl {
namespace fpl_project {

struct Config;
struct InputConfig;

class Game {
 public:
  Game();
  ~Game();
  bool Initialize(const char* const binary_directory);
  void Run();

 private:
  enum MeshModel {
    kMeshFernA,
    kMeshFernB,
    kMeshFernC,
    kMeshSushiA,
    kMeshSushiB,
    kMeshSushiC,
    kMeshTreeA,
    kNumMeshes
  };

  bool InitializeRenderer();
  bool InitializeAssets();

  Mesh* CreateVerticalQuadMesh(const char* material_name,
                               const mathfu::vec3& offset,
                               const mathfu::vec2& pixel_bounds,
                               float pixel_to_world_scale);
  void Render();
  void Render2DElements(mathfu::vec2i resolution);
  void Update(WorldTime delta_time);
  void UpdateMainCamera();
  void UpdateMainCameraAndroid();
  void UpdateMainCameraMouse();
  const Config& GetConfig() const;
  const InputConfig& GetInputConfig() const;
  const RailDef& GetRailDef() const;
  Mesh* GetCardboardFront(int renderable_id);

  // Hold configuration binary data.
  std::string config_source_;

  // Hold the configuration for the input system data.
  std::string input_config_source_;

  // Report touches, button presses, keyboard presses.
  InputSystem input_;

  // Hold rendering context.
  Renderer renderer_;

  // Load and own rendering resources.
  MaterialManager matman_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

  // Shaders we use.
  Shader* shader_cardboard_;
  Shader* shader_lit_textured_normal_;
  Shader* shader_textured_;

  // World time of previous update. We use this to calculate the delta_time of
  // the current update. This value is tied to the real-world clock.  Note that
  // it is distict from game_state_.time_, which is *not* tied to the real-world
  // clock. If the game is paused, game_state.time_ will pause, but
  // prev_world_time_ will keep chugging.
  WorldTime prev_world_time_;

  std::string rail_source_;

  pindrop::AudioConfig* audio_config_;

  Mesh* billboard_;
  Mesh* meshes_[kNumMeshes];
  GameState game_state_;

  // String version number of the game.
  const char* version_;
};

}  // fpl_project
}  // fpl

#endif  // ZOOSHI_GAME_H
