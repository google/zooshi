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

#include "components/transform.h"
#include "config_generated.h"
#include "rail_def_generated.h"
#include "fplbase/input.h"
#include "game_state.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const float kViewportAngle = M_PI / 4.0f;  // 45 degrees
static const float kViewportNearPlane = 1.0f;
static const float kViewportFarPlane = 500.0f;
static const vec4 kGreenishColor(0.05f, 0.2f, 0.1f, 1.0f);

// The sound effect to play when throwing projectiles.
static const char* kProjectileWhooshSoundName = "whoosh";
// The sound effect to play when a projectile bounces.
static const char* kProjectileBounceSoundName = "paper_bounce";

GameState::GameState()
    : main_camera_(),
      entity_manager_(),
      entity_factory_(),
      motive_engine_(),
      transform_component_(),
      family_component_(&entity_factory_),
      rail_denizen_component_(&motive_engine_),
      player_component_(),
      render_mesh_component_(),
      active_player_entity_() {}

void GameState::Initialize(const vec2i& window_size, const Config& config,
                           const InputConfig& input_config,
                           InputSystem* input_system,
                           MaterialManager* material_manager,
                           pindrop::AudioEngine* audio_engine) {
  main_camera_.Initialize(kViewportAngle, vec2(window_size), kViewportNearPlane,
                          kViewportFarPlane);

  motive::SmoothInit::Register();

  config_ = &config;
  input_config_ = &input_config;

  input_system_ = input_system;
  material_manager_ = material_manager;

  entity_manager_.RegisterComponent<TransformComponent>(&transform_component_);
  entity_manager_.RegisterComponent<FamilyComponent>(&family_component_);
  entity_manager_.RegisterComponent<RailDenizenComponent>(
      &rail_denizen_component_);
  entity_manager_.RegisterComponent<PlayerComponent>(&player_component_);
  entity_manager_.RegisterComponent<PlayerProjectileComponent>(
      &player_projectile_component_);
  entity_manager_.RegisterComponent<RenderMeshComponent>(
      &render_mesh_component_);
  entity_manager_.RegisterComponent<PhysicsComponent>(&physics_component_);

  std::string rail_def_source;
  if (!LoadFile(config.rail_filename()->c_str(), &rail_def_source)) {
    return;
  }
  const RailDef* rail_def = GetRailDef(rail_def_source.c_str());
  rail_denizen_component_.Initialize(rail_def);

  entity_manager_.set_entity_factory(&entity_factory_);
  render_mesh_component_.set_material_manager(material_manager);
  player_component_.set_config(config_);
  input_controller_.set_input_config(input_config_);
  input_controller_.set_input_system(input_system_);
  physics_component_.InitializeAudio(audio_engine, kProjectileBounceSoundName);
  player_projectile_component_.InitializeAudio(audio_engine,
                                               kProjectileWhooshSoundName);

  for (size_t i = 0; i < config.entity_list()->size(); i++) {
    entity_manager_.CreateEntityFromData(config.entity_list()->Get(i));
  }

  for (int x = -3; x < 3; x++) {
    for (int y = -3; y < 3; y++) {
      entity::EntityRef entity = entity_manager_.CreateEntityFromData(
            config.fern_def());

      TransformData* transform_data =
          entity_manager_.GetComponentData<TransformData>(entity);

      transform_data->position = vec3(x * 24 + 12, y * 24 + 12, 1);
    }
  }

  for (auto iter = player_component_.begin(); iter != player_component_.end();
       ++iter) {
    active_player_entity_ = iter->entity;
    entity_manager_.GetComponentData<PlayerData>(active_player_entity_)
        ->set_input_controller(&input_controller_);
  }

  render_mesh_component_.set_light_position(vec3(-10, -20, 20));
}

static vec2 AdjustedMouseDelta(const vec2i& raw_delta,
                               const InputConfig& input_config) {
  vec2 delta(raw_delta);
  delta *= input_config.mouse_sensitivity();
  if (!input_config.invert_x()) {
    delta.x() *= -1.0f;
  }
  if (!input_config.invert_y()) {
    delta.y() *= -1.0f;
  }
  return delta;
}

void GameState::UpdateMainCamera() {
  PlayerData* player = player_component_.GetEntityData(active_player_entity_);
  RailDenizenData* rail_denizen =
      rail_denizen_component_.GetEntityData(active_player_entity_);

  main_camera_.set_position(rail_denizen->Position());
  main_camera_.set_facing(player->GetFacing());
  main_camera_.set_up(player->GetUp());
}

void GameState::Update(WorldTime delta_time) {
  motive_engine_.AdvanceFrame(delta_time);
  entity_manager_.UpdateComponents(delta_time);
  UpdateMainCamera();
}

void GameState::Render(Renderer* renderer) {
  renderer->ClearFrameBuffer(kGreenishColor);
  mat4 camera_transform = main_camera_.GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  render_mesh_component_.RenderAllEntities(*renderer, main_camera_);
}

}  // fpl_project
}  // fpl

