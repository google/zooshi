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

#include "components/player.h"
#include "camera.h"
#include "component_library/common_services.h"
#include "component_library/physics.h"
#include "component_library/transform.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "entity/entity_common.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "world.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::PlayerComponent,
                            fpl::fpl_project::PlayerData)

namespace fpl {
namespace fpl_project {

BREADBOARD_DEFINE_EVENT(kOnFireEventId)

using fpl::component_library::CommonServicesComponent;
using fpl::component_library::PhysicsComponent;
using fpl::component_library::PhysicsData;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;

void PlayerComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
}
void PlayerComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    PlayerData* player_data = Data<PlayerData>(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);
    if (active_) {
      player_data->input_controller()->Update();
    }
    transform_data->orientation =
        mathfu::quat::RotateFromTo(player_data->GetFacing(), mathfu::kAxisY3f);
    if (player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      SpawnProjectile(iter->entity);

      if (player_data->on_fire()) {
        GraphData* graph_data = Data<GraphData>(iter->entity);
        if (graph_data) {
          graph_data->broadcaster.BroadcastEvent(kOnFireEventId);
        }
      }
    }
  }
}

void PlayerComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* /*raw_data*/) {
  AddEntity(entity);
}

void PlayerComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

entity::EntityRef PlayerComponent::SpawnProjectile(entity::EntityRef source) {
  entity::EntityRef projectile =
      entity_manager_->GetComponent<ServicesComponent>()
          ->entity_factory()
          ->CreateEntityFromPrototype("Projectile", entity_manager_);

  TransformData* transform_data = Data<TransformData>(projectile);
  PhysicsData* physics_data = Data<PhysicsData>(projectile);
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile);

  TransformComponent* transform_component = GetComponent<TransformComponent>();
  transform_data->position =
      transform_component->WorldPosition(source) +
      mathfu::kAxisZ3f * config_->projectile_height_offset();
  transform_data->orientation = transform_component->WorldOrientation(source);
  const vec3 forward = CalculateProjectileDirection(source);
  vec3 velocity = config_->projectile_speed() * forward +
                  config_->projectile_upkick() * mathfu::kAxisZ3f;
  transform_data->position +=
      velocity.Normalized() * config_->projectile_forward_offset();

  // Include the raft's current velocity to the thrown sushi.
  auto raft_entity =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  auto raft_rail = raft_entity ? Data<RailDenizenData>(raft_entity) : nullptr;
  if (raft_rail != nullptr) velocity += raft_rail->Velocity();

  physics_data->SetVelocity(velocity);
  physics_data->SetAngularVelocity(vec3(mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f)));
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  physics_component->UpdatePhysicsFromTransform(projectile);

  projectile_data->owner = source;

  // TODO: Preferably, this should be a step in the entity creation.
  transform_component->UpdateChildLinks(projectile);

  return entity::EntityRef();
}

vec3 PlayerComponent::CalculateProjectileDirection(
    entity::EntityRef source) const {
  PlayerData* player_data = Data<PlayerData>(source);
  TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  vec3 forward = transform_component->WorldOrientation(source).Inverse() *
                 mathfu::kAxisY3f;
  const Camera* camera =
      entity_manager_->GetComponent<ServicesComponent>()->camera();
  // Use the last position from the controller to determine the offset and
  // direction of the projectile. In Cardboard mode this should be ignored,
  // as we always want to fire down the center.
  if (player_data->input_controller()->last_position().x() >= 0 &&
      camera != nullptr &&
      !entity_manager_->GetComponent<ServicesComponent>()
           ->world()
           ->is_in_cardboard) {
    const vec2 screen_size(
        entity_manager_->GetComponent<CommonServicesComponent>()
            ->renderer()
            ->window_size());
    // We want to calculate the world ray based on the touch location.
    // We do this by projecting it onto a plane in front of the camera, based
    // on the viewport angle and resolution.
    float fov_y_tan = 2.0f * tan(camera->viewport_angle() * 0.5f);
    float fov_x_tan = fov_y_tan * camera->viewport_resolution().x() /
                      camera->viewport_resolution().y();
    const vec2 fov_tan(fov_x_tan, -fov_y_tan);
    const vec2 touch(player_data->input_controller()->last_position());
    const vec2 offset = fov_tan * (touch / screen_size - 0.5f);

    const vec3 far_vec = camera->up() * offset.y() + camera->Right() * offset.x();
    forward = (forward + far_vec).Normalized();
  }

  return forward;
}

entity::ComponentInterface::RawDataUniquePtr PlayerComponent::ExportRawData(
    const entity::EntityRef& entity) const {
  const PlayerData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  PlayerDefBuilder builder(fbb);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
