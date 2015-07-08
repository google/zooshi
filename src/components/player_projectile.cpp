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

#include "components/player_projectile.h"
#include "components/services.h"
#include "components/transform.h"
#include "events/collision.h"
#include "events/parse_action.h"
#include "pindrop/pindrop.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::PlayerProjectileComponent,
                            fpl::fpl_project::PlayerProjectileData)

namespace fpl {
namespace fpl_project {

void PlayerProjectileComponent::Init() {
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  event_manager_->RegisterListener(EventSinkUnion_Collision, this);
}

void PlayerProjectileComponent::AddFromRawData(entity::EntityRef& entity,
                                               const void* raw_data) {
  auto projectile_def = static_cast<const PlayerProjectileDef*>(raw_data);
  PlayerProjectileData* projectile_data = AddEntity(entity);
  projectile_data->on_collision = projectile_def->on_collision();

  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void PlayerProjectileComponent::OnEvent(
    const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_Collision: {
      auto* collision = event_payload.ToData<CollisionPayload>();
      if (collision->entity_a->IsRegisteredForComponent(GetComponentId())) {
        HandleCollision(collision->entity_a, collision->entity_b);
      } else if (collision->entity_b->IsRegisteredForComponent(
                     GetComponentId())) {
        HandleCollision(collision->entity_b, collision->entity_a);
      }
      break;
    }
  }
}

void PlayerProjectileComponent::HandleCollision(
    const entity::EntityRef& projectile_entity,
    const entity::EntityRef& collided_entity) {
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile_entity);
  if (projectile_data->on_collision) {
    EventContext context;
    context.source_owner = projectile_data->owner;
    context.source = projectile_entity;
    context.target = collided_entity;
    context.raft =
        entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
    ParseAction(projectile_data->on_collision, &context, event_manager_,
                entity_manager_);
  }
}

}  // fpl_project
}  // fpl
