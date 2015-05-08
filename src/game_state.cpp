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
                           MaterialManager* material_manager, Shader* shader) {
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

  for (size_t i = 0; i < config.entity_list()->size(); i++) {
    entity_manager_.CreateEntityFromData(config.entity_list()->Get(i));
  }


  for (int x = -3; x < 3; x++) {
    for (int y = -3; y < 3; y++) {
      // Let's make an entity!
      entity::EntityRef entity = entity_manager_.AllocateNewEntity();
      transform_component_.AddEntity(entity);
      TransformData* transform_data =
          entity_manager_.GetComponentData<TransformData>(entity);

      transform_data->position = vec3(x * 20 + 10, y * 20 + 10, 1);
      transform_data->orientation = mathfu::quat::identity;
      transform_data->scale = vec3(100.0f, 100.0f, 100.0f);

      physics_component_.AddEntity(entity);
      render_mesh_component_.AddEntity(entity);
    }
  }

  for (auto iter = player_component_.begin();
       iter != player_component_.end(); ++iter) {
    active_player_entity_ = iter->entity;
    entity_manager_.GetComponentData<PlayerData>(active_player_entity_)->
        set_input_controller(&input_controller_);
  }



  for (auto iter = render_mesh_component_.begin();
       iter != render_mesh_component_.end(); ++iter) {
    if (iter->entity != active_player_entity_) {
      RenderMeshData* render_data =
          render_mesh_component_.AddEntity(iter->entity);
      render_data->mesh = material_manager_->LoadMesh("meshes/sushi_a.fplmesh");
      render_data->shader = shader;
    }
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
  mat4 camera_transform = main_camera_.GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  render_mesh_component_.RenderAllEntities(*renderer, main_camera_);
}

}  // fpl_project
}  // fpl

