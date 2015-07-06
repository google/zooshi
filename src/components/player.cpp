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

#include "btBulletDynamicsCommon.h"
#include "components/physics.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/rendermesh.h"
#include "components/services.h"
#include "components/transform.h"
#include "entity/entity_common.h"
#include "event/event_manager.h"
#include "events/parse_action.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "world.h"

namespace fpl {
namespace fpl_project {

static const float kDegreesToRadians = M_PI / 180.0f;

void PlayerData::SetInitialDirection(const mathfu::vec3& initial_direction) {
  initial_direction_ = mathfu::quat::RotateFromTo(
      mathfu::kAxisY3f, initial_direction.Normalized());
}

void PlayerComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
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
        mathfu::quat::RotateFromTo(player_data->GetFacing(), mathfu::kAxisY3f) *
        player_data->initial_direction();
    if (player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      SpawnProjectile(iter->entity);
      EventContext context;
      context.source = iter->entity;
      context.raft =
          entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
      ParseAction(player_data->on_fire(), &context, event_manager_,
                  entity_manager_);
    }
  }
}

void PlayerComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PlayerDef);
  auto player_def = static_cast<const PlayerDef*>(component_data->data());
  PlayerData* player_data = AddEntity(entity);
  player_data->set_on_fire(player_def->on_fire());
  player_data->SetInitialDirection(
      mathfu::vec3(player_def->initial_direction()->x(),
                   player_def->initial_direction()->y(),
                   player_def->initial_direction()->z()));
}

void PlayerComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

entity::EntityRef PlayerComponent::SpawnProjectile(entity::EntityRef source) {
  entity::EntityRef projectile =
      entity_manager_->GetComponent<ServicesComponent>()
          ->zooshi_entity_factory()
          ->CreateEntityFromPrototype("Projectile", entity_manager_);

  TransformData* transform_data = Data<TransformData>(projectile);
  PhysicsData* physics_data = Data<PhysicsData>(projectile);
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile);

  TransformComponent* transform_component = GetComponent<TransformComponent>();
  transform_data->position = transform_component->WorldPosition(source) +
                             LoadVec3(config_->projectile_offset());
  transform_data->orientation = transform_component->WorldOrientation(source);
  vec3 forward = transform_data->orientation.Inverse() * mathfu::kAxisY3f;

  vec3 velocity = config_->projectile_speed() * forward +
                  config_->projectile_upkick() * mathfu::kAxisZ3f;

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

  return entity::EntityRef();
}

entity::ComponentInterface::RawDataUniquePtr PlayerComponent::ExportRawData(
    entity::EntityRef& entity) const {
  const PlayerData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  // TODO: output the on_fire events.
  mathfu::vec3 euler =
      data->initial_direction().ToEulerAngles() / kDegreesToRadians;
  fpl::Vec3 initial_direction{euler.x(), euler.y(), euler.z()};

  PlayerDefBuilder builder(fbb);
  builder.add_initial_direction(&initial_direction);

  auto component = CreateComponentDefInstance(fbb, ComponentDataUnion_PlayerDef,
                                              builder.Finish().Union());

  fbb.Finish(component);
  return fbb.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
