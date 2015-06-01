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
#include "components/rendermesh.h"
#include "components/transform.h"
#include "entity/entity_common.h"
#include "event_system/event_manager.h"
#include "events/projectile_fired.h"
#include "fplbase/utilities.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

void PlayerComponent::Initialize(event::EventManager* event_manager,
                                 const Config* config) {
  event_manager_ = event_manager;
  config_ = config;
}

void PlayerComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    PlayerData* player_data = Data<PlayerData>(iter->entity);
    player_data->input_controller()->Update();
    if (player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      entity::EntityRef projectile = SpawnProjectile(iter->entity);
      event_manager_->BroadcastEvent(
          ProjectileFiredEvent(iter->entity, projectile));
    }
    if (player_data->listener().Valid()) {
      TransformData* transform_data = Data<TransformData>(iter->entity);
      player_data->listener().SetLocation(transform_data->position);
    }
  }
}

void PlayerComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PlayerDef);
  (void)component_data;
  AddEntity(entity);
}

void PlayerComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

entity::EntityRef PlayerComponent::SpawnProjectile(entity::EntityRef source) {
  entity::EntityRef projectile = entity_manager_->CreateEntityFromData(
      config_->entity_defs()->Get(EntityDefs_kProjectile));

  TransformData* transform_data =
      entity_manager_->GetComponentData<TransformData>(projectile);
  PhysicsData* physics_data =
      entity_manager_->GetComponentData<PhysicsData>(projectile);
  PlayerProjectileData* projectile_data =
      entity_manager_->GetComponentData<PlayerProjectileData>(projectile);

  TransformData* source_transform_data = Data<TransformData>(source);
  transform_data->position = source_transform_data->position;
  transform_data->orientation = source_transform_data->orientation;

  PlayerData* source_player_data = Data<PlayerData>(source);

  // TODO: Instantiate physics from raw data (b/21502254)
  physics_data->shape.reset(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));
  physics_data->motion_state.reset(
      new btDefaultMotionState(btTransform(btQuaternion(), btVector3())));
  btScalar mass = 1;
  btVector3 inertia(0, 0, 0);
  physics_data->shape->calculateLocalInertia(mass, inertia);
  btRigidBody::btRigidBodyConstructionInfo rigid_body_builder(
      mass, physics_data->motion_state.get(), physics_data->shape.get(),
      inertia);
  rigid_body_builder.m_restitution = 1.0f;
  physics_data->rigid_body.reset(new btRigidBody(rigid_body_builder));

  auto velocity = config_->projectile_speed() * source_player_data->GetFacing();
  physics_data->SetVelocity(velocity);
  physics_data->SetAngularVelocity(vec3(mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f)));

  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  physics_component->UpdatePhysicsFromTransform(projectile);
  physics_component->bullet_world()->addRigidBody(
      physics_data->rigid_body.get());

  projectile_data->owner = source;

  return entity::EntityRef();
}

}  // fpl_project
}  // fpl
