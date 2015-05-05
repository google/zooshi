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
static const float kViewportFarPlane = 100.0f;

GameState::GameState()
    : main_camera_(),
      entity_manager_(),
      entity_factory_(),
      motive_engine_(),
      transform_component_(),
      rail_denizen_component_(&motive_engine_),
      player_component_(),
      render_mesh_component_(),
      active_player_entity_() {}

void GameState::Initialize(const vec2i& window_size, const Config& config,
                           Mesh* mesh, Shader* shader) {
  main_camera_.Initialize(kViewportAngle, vec2(window_size), kViewportNearPlane,
                          kViewportFarPlane);

  motive::SmoothInit::Register();

  entity_manager_.RegisterComponent<TransformComponent>(&transform_component_);
  entity_manager_.RegisterComponent<RailDenizenComponent>(
      &rail_denizen_component_);
  entity_manager_.RegisterComponent<PlayerComponent>(&player_component_);
  entity_manager_.RegisterComponent<RenderMeshComponent>(
      &render_mesh_component_);
  entity_manager_.RegisterComponent<PhysicsComponent>(&physics_component_);

  entity_manager_.set_entity_factory(&entity_factory_);

  for (size_t i = 0; i < config.entity_list()->size(); i++) {
    entity::EntityRef ref =
        entity_manager_.CreateEntityFromData(config.entity_list()->Get(i));

    // For now, the camera follows the first entity defined.
    // TODO(amablue): come up with a better solution.
    if (i == 0) {
      active_player_entity_ = ref;
    }
  }

  for (int x = -3; x < 4; x++) {
    for (int y = -3; y < 4; y++) {
      // Let's make an entity!
      entity::EntityRef large_entity = entity_manager_.AllocateNewEntity();
      RenderMeshData* render_data =
          render_mesh_component_.AddEntity(large_entity);
      TransformData* transform_data =
          entity_manager_.GetComponentData<TransformData>(large_entity);

      render_data->mesh = mesh;
      render_data->shader = shader;

      transform_data->position = vec3(x * 20, y * 20, 0);
      transform_data->orientation = mathfu::quat::identity;
      transform_data->scale = vec3(3, 3, 3);

      physics_component_.AddEntity(large_entity);
    }
  }

  render_mesh_component_.set_light_position(vec3(-10, -10, 10));
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

void GameState::UpdateMainCameraAndroid() {
  PlayerData* player = player_component_.GetEntityData(active_player_entity_);
  RailDenizenData* rail_denizen =
      rail_denizen_component_.GetEntityData(active_player_entity_);

#ifdef ANDROID_CARDBOARD
  const vec3 cardboard_forward = input_.cardboard_input().forward();
  const vec3 forward = vec3(cardboard_forward.x(), -cardboard_forward.z(),
                            cardboard_forward.y());
  player->facing = quat::RotateFromTo(mathfu::kAxisX3f, forward);

  const vec3 cardboard_up = input_.cardboard_input().up();
  const vec3 up = vec3(cardboard_up.x(), -cardboard_up.z(), cardboard_up.y());
  main_camera_.set_up(up);
#endif

  main_camera_.set_position(rail_denizen->Position());
  main_camera_.set_facing(player->facing * mathfu::kAxisX3f);
}

void GameState::UpdateMainCameraMouse(const vec2& delta) {
  PlayerData* player = player_component_.GetEntityData(active_player_entity_);
  RailDenizenData* rail_denizen =
      rail_denizen_component_.GetEntityData(active_player_entity_);

  // We assume that the player is looking along the x axis, before
  // camera transformations are applied:
  vec3 player_facing_vector = player->facing * mathfu::kAxisX3f;
  vec3 side_vector =
      quat::FromAngleAxis(-M_PI / 2, mathfu::kAxisZ3f) * player_facing_vector;

  quat pitch_adjustment = quat::FromAngleAxis(delta.y(), side_vector);
  quat yaw_adjustment = quat::FromAngleAxis(delta.x(), mathfu::kAxisZ3f);
  player->facing = pitch_adjustment * yaw_adjustment * player->facing;

  main_camera_.set_position(rail_denizen->Position());
  main_camera_.set_facing(player->facing * mathfu::kAxisX3f);
}

void GameState::Update(WorldTime delta_time, const InputSystem& input,
                       const InputConfig& input_config) {
  motive_engine_.AdvanceFrame(delta_time);
  entity_manager_.UpdateComponents(delta_time);
#ifdef __ANDROID__
  UpdateMainCameraAndroid();
#else
  vec2 delta = AdjustedMouseDelta(input.pointers_[0].mousedelta, input_config);
  UpdateMainCameraMouse(delta);
#endif
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

