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

#ifndef GAME_H
#define GAME_H

#include "camera.h"
#include "input.h"
#include "material_manager.h"
#include "pindrop/pindrop.h"
#include "renderer.h"
#include "flatbuffers/flatbuffers.h"
#include "config_generated.h"

namespace fpl {
namespace fpl_project {

struct Config;

class Game {
 public:
  Game();
  ~Game();
  bool Initialize(const char* const binary_directory);
  void Run();

 private:
  bool InitializeConfig();
  bool InitializeRenderer();
  bool InitializeAssets();
  Mesh* CreateVerticalQuadMesh(const char* material_name, const vec3& offset,
                               const vec2& pixel_bounds,
                               float pixel_to_world_scale);
  void Render(const Camera& camera);
  void Render2DElements(mathfu::vec2i resolution);
  void Update(WorldTime delta_time);
  const Config& GetConfig() const;
  Mesh* GetCardboardFront(int renderable_id);

  // Hold configuration binary data.
  std::string config_source_;

  // Report touches, button presses, keyboard presses.
  InputSystem input_;

  // Hold rendering context.
  Renderer renderer_;

  // Load and own rendering resources.
  MaterialManager matman_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

  // Shaders we use.
  Shader* shader_cardboard;
  Shader* shader_lit_textured_normal_;
  Shader* shader_textured_;

  // World time of previous update. We use this to calculate the delta_time
  // of the current update. This value is tied to the real-world clock.
  // Note that it is distict from game_state_.time_, which is *not* tied to the
  // real-world clock. If the game is paused, game_state.time_ will pause, but
  // prev_world_time_ will keep chugging.
  WorldTime prev_world_time_;

  // Channel used to play the ambience sound effect.
  pindrop::Channel ambience_channel_;

  // String version number of the game.
  const char* version_;
  Mesh* billboard_;
  pindrop::AudioConfig* audioConfig_;
  Camera main_camera_;


  float model_angle_;
};

}  // game
}  // fpl

#endif  // GAME_H
