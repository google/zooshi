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
#include "entity/entity_common.h"
#include "events/projectile_fired_event.h"
#include "event_system/event_manager.h"
#include "components/physics.h"
#include "components/rendermesh.h"
#include "components/transform.h"
#include "components/player_projectile.h"
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
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    PlayerData* player_data = Data<PlayerData>(iter->entity);
    player_data->input_controller()->Update();
    if (player_data->input_controller()->Button(kFireProjectile).Value() &&
        player_data->input_controller()->Button(kFireProjectile).HasChanged()) {
      entity::EntityRef projectile = SpawnProjectile(iter->entity);
      event_manager_->BroadcastEvent(
          kEventIdProjectileFired,
          ProjectileFiredEventPayload(iter->entity, projectile));
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
  entity::EntityRef projectile =
      entity_manager_->CreateEntityFromData(config_->projectile_def());

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

  physics_data->velocity =
      config_->projectile_speed() * source_player_data->GetFacing();
  physics_data->angular_velocity = mathfu::quat::FromEulerAngles(mathfu::vec3(
      mathfu::RandomInRange(0.05f, 0.1f), mathfu::RandomInRange(0.05f, 0.1f),
      mathfu::RandomInRange(0.05f, 0.1f)));
  projectile_data->owner = source;

  return entity::EntityRef();
}

}  // fpl_project
}  // fpl
