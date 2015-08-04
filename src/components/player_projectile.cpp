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

#include "component_library/common_services.h"
#include "component_library/transform.h"
#include "components/services.h"
#include "events/collision.h"
#include "events/parse_action.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "pindrop/pindrop.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::PlayerProjectileComponent,
                            fpl::fpl_project::PlayerProjectileData)

namespace fpl {
namespace fpl_project {

using fpl::component_library::CollisionPayload;
using fpl::component_library::CommonServicesComponent;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;

void PlayerProjectileComponent::Init() {
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  event_manager_->RegisterListener(EventSinkUnion_Collision, this);
}

void PlayerProjectileComponent::AddFromRawData(entity::EntityRef& entity,
                                               const void* raw_data) {
  auto projectile_def = static_cast<const PlayerProjectileDef*>(raw_data);
  PlayerProjectileData* projectile_data = AddEntity(entity);
  if (entity_manager_->GetComponent<CommonServicesComponent>()
          ->entity_factory()
          ->WillBeKeptInMemory(projectile_def->on_collision())) {
    projectile_data->on_collision = projectile_def->on_collision();
  } else {
    // Copy into our own storage.
    // This can be slow! But fortunately it only happens when you are
    // using the world editor to edit nodes.
    flatbuffers::FlatBufferBuilder fbb;
    auto binary_schema = entity_manager_->GetComponent<ServicesComponent>()
                             ->component_def_binary_schema();
    auto schema = reflection::GetSchema(binary_schema);
    auto table_def = schema->objects()->LookupByKey("TaggedActionDefList");
    flatbuffers::Offset<ActionDef> table =
        flatbuffers::CopyTable(
            fbb, *schema, *table_def,
            *(const flatbuffers::Table*)projectile_def->on_collision())
            .o;
    fbb.Finish(table);
    projectile_data->on_collision_flatbuffer = {
        fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize()};
    projectile_data->on_collision = flatbuffers::GetRoot<TaggedActionDefList>(
        projectile_data->on_collision_flatbuffer.data());
  }

  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void PlayerProjectileComponent::OnEvent(
    const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_Collision: {
      auto* collision = event_payload.ToData<CollisionPayload>();
      if (collision->entity_a->IsRegisteredForComponent(GetComponentId())) {
        HandleCollision(collision->entity_a, collision->entity_b,
                        collision->tag_b);
      } else if (collision->entity_b->IsRegisteredForComponent(
                     GetComponentId())) {
        HandleCollision(collision->entity_b, collision->entity_a,
                        collision->tag_a);
      }
      break;
    }
  }
}

// This code is largely duplicated from the Patron component. This is a bad
// thing. We really want this in the Physics component, but that requires
// migrating a bunch of code between different libraries. This is being tracked
// in b/22847175
void PlayerProjectileComponent::HandleCollision(
    const entity::EntityRef& projectile_entity,
    const entity::EntityRef& collided_entity, const std::string& part_tag) {
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(projectile_entity);

  size_t on_collision_size =
      (projectile_data->on_collision &&
       projectile_data->on_collision->action_list())
          ? projectile_data->on_collision->action_list()->size()
          : 0;
  entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  EventContext context;
  context.source_owner = projectile_data->owner;
  context.source = projectile_entity;
  context.target = collided_entity;
  context.raft = raft;
  for (size_t i = 0; i < on_collision_size; ++i) {
    auto* tagged_action = projectile_data->on_collision->action_list()->Get(i);
    const char* tag =
        tagged_action->tag() ? tagged_action->tag()->c_str() : nullptr;
    if (tag && strcmp(tag, part_tag.c_str()) == 0) {
      const ActionDef* action = tagged_action->action();
      ParseAction(action, &context, event_manager_, entity_manager_);
      break;
    }
  }
}

}  // fpl_project
}  // fpl
