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
#include "components/attributes.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "corgi/entity_common.h"
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/transform.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "world.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::PlayerComponent, fpl::zooshi::PlayerData)

namespace fpl {
namespace zooshi {

BREADBOARD_DEFINE_EVENT(kOnFireEventId)

using corgi::component_library::CommonServicesComponent;
using corgi::component_library::GraphComponent;
using corgi::component_library::GraphData;
using corgi::component_library::PhysicsComponent;
using corgi::component_library::PhysicsData;
using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;

void PlayerComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
}
void PlayerComponent::UpdateAllEntities(corgi::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    PlayerData* player_data = Data<PlayerData>(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);
    if (state_ != kPlayerState_Disabled) {
      player_data->input_controller()->Update();
    }
    transform_data->orientation =
        mathfu::quat::RotateFromTo(player_data->GetFacing(), mathfu::kAxisY3f);
    if (state_ == kPlayerState_Active &&
        player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      SpawnProjectile(iter->entity);

      GraphData* graph_data = Data<GraphData>(iter->entity);
      if (graph_data) {
        graph_data->broadcaster.BroadcastEvent(kOnFireEventId);
      }
    }
  }
}

void PlayerComponent::AddFromRawData(corgi::EntityRef& entity,
                                     const void* /*raw_data*/) {
  AddEntity(entity);
}

void PlayerComponent::InitEntity(corgi::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

static inline float RandomSign() {
  return mathfu::Random<float>() < 0.5f ? -1.0f : 1.0f;
}

// Return an angle between kMinProjectileAngularVelocity and
// kMaxProjectileAngularVelocity, in degrees. Returned angle has equal
// probibility of being positive and negative.
mathfu::vec3 PlayerComponent::RandomProjectileAngularVelocity() const {
  const mathfu::vec3 random(mathfu::Random<float>(), mathfu::Random<float>(),
                            mathfu::Random<float>());
  auto angle = mathfu::Lerp(
      LoadVec3(config_->projectile_min_angular_velocity()),
      LoadVec3(config_->projectile_max_angular_velocity()), random);
  const mathfu::vec3 sign(RandomSign(), RandomSign(), RandomSign());
  return angle * sign;
}

corgi::EntityRef PlayerComponent::SpawnProjectile(corgi::EntityRef source) {
  const SushiConfig* current_sushi = static_cast<const SushiConfig*>(
      entity_manager_->GetComponent<ServicesComponent>()
          ->world()
          ->SelectedSushi()
          ->data());
  corgi::EntityRef projectile =
      entity_manager_->GetComponent<ServicesComponent>()
          ->entity_factory()
          ->CreateEntityFromPrototype(current_sushi->prototype()->c_str(),
                                      entity_manager_);
  GraphComponent* graph_component =
      entity_manager_->GetComponent<GraphComponent>();
  graph_component->EntityPostLoadFixup(projectile);

  TransformData* transform_data = Data<TransformData>(projectile);
  PhysicsData* physics_data = Data<PhysicsData>(projectile);
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile);

  TransformComponent* transform_component = GetComponent<TransformComponent>();
  transform_data->position =
      transform_component->WorldPosition(source) +
      mathfu::kAxisZ3f * config_->projectile_height_offset();
  auto forward = CalculateProjectileDirection(source);
  auto velocity = current_sushi->speed() * forward +
                  current_sushi->upkick() * mathfu::kAxisZ3f;
  transform_data->position +=
      velocity.Normalized() * config_->projectile_forward_offset();

  // Include the raft's current velocity to the thrown sushi.
  auto raft_entity =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  auto raft_rail = raft_entity ? Data<RailDenizenData>(raft_entity) : nullptr;
  if (raft_rail != nullptr) velocity += raft_rail->Velocity();

  physics_data->SetVelocity(velocity);
  physics_data->SetAngularVelocity(RandomProjectileAngularVelocity());
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  physics_component->UpdatePhysicsFromTransform(projectile);

  projectile_data->owner = source;

  // TODO: Preferably, this should be a step in the entity creation.
  transform_component->UpdateChildLinks(projectile);

  Data<AttributesData>(source)->attributes[AttributeDef_ProjectilesFired]++;

  return corgi::EntityRef();
}

mathfu::vec3 PlayerComponent::CalculateProjectileDirection(
    corgi::EntityRef source) const {
  PlayerData* player_data = Data<PlayerData>(source);
  TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  auto forward = transform_component->WorldOrientation(source).Inverse() *
                 mathfu::kAxisY3f;
  const Camera* camera =
      entity_manager_->GetComponent<ServicesComponent>()->camera();
  // Use the last position from the controller to determine the offset and
  // direction of the projectile. In Cardboard mode this should be ignored,
  // as we always want to fire down the center.
  if (player_data->input_controller()->last_position().x >= 0 &&
      camera != nullptr &&
      entity_manager_->GetComponent<ServicesComponent>()
              ->world()
              ->rendering_mode() == kRenderingMonoscopic) {
    const mathfu::vec2 screen_size(
        entity_manager_->GetComponent<CommonServicesComponent>()
            ->renderer()
            ->window_size());
    // We want to calculate the world ray based on the touch location.
    // We do this by projecting it onto a plane in front of the camera, based
    // on the viewport angle and resolution.
    float fov_y_tan = 2.0f * tan(camera->viewport_angle() * 0.5f);
    float fov_x_tan = fov_y_tan * camera->viewport_resolution().x /
                      camera->viewport_resolution().y;
    const mathfu::vec2 fov_tan(fov_x_tan, -fov_y_tan);
    const mathfu::vec2 touch(player_data->input_controller()->last_position());
    const mathfu::vec2 offset = fov_tan * (touch / screen_size - 0.5f);

    auto far_vec = camera->up() * offset.y + camera->Right() * offset.x;
    forward = (forward + far_vec).Normalized();
  }

  return forward;
}

corgi::ComponentInterface::RawDataUniquePtr PlayerComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  const PlayerData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  PlayerDefBuilder builder(fbb);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

}  // zooshi
}  // fpl
