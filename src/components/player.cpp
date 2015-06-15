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
#include "components/services.h"
#include "components/transform.h"
#include "entity/entity_common.h"
#include "event_system/event_manager.h"
#include "events/parse_action.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

void PlayerComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
}

void PlayerComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    PlayerData* player_data = Data<PlayerData>(iter->entity);
    player_data->input_controller()->Update();
    if (player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      SpawnProjectile(iter->entity);
      EventContext context;
      context.source = iter->entity;
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
}

void PlayerComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

entity::EntityRef PlayerComponent::SpawnProjectile(entity::EntityRef source) {
  entity::EntityRef projectile = entity_manager_->CreateEntityFromData(
      config_->entity_defs()->Get(EntityDefs_kProjectile));

  TransformData* transform_data = Data<TransformData>(projectile);
  PhysicsData* physics_data = Data<PhysicsData>(projectile);
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile);

  TransformData* source_transform_data = Data<TransformData>(source);
  transform_data->position = source_transform_data->position;
  transform_data->orientation = source_transform_data->orientation;

  PlayerData* source_player_data = Data<PlayerData>(source);

  auto velocity = config_->projectile_speed() * source_player_data->GetFacing();
  physics_data->SetVelocity(velocity);
  physics_data->SetAngularVelocity(vec3(mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f),
                                        mathfu::RandomInRange(3.0f, 6.0f)));
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  physics_component->UpdatePhysicsFromTransform(projectile);

  projectile_data->owner = source;

  return entity::EntityRef();
}

}  // fpl_project
}  // fpl
