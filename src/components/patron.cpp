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

#include "components/patron.h"

#include <vector>
#include "component_library/animation.h"
#include "component_library/common_services.h"
#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "component_library/transform.h"
#include "components/attributes.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "events/collision.h"
#include "events/parse_action.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "mathfu/glsl_mappings.h"
#include "motive/anim.h"
#include "motive/anim_table.h"
#include "world.h"
#include "world_editor/editor_event.h"

using mathfu::vec3;
using mathfu::quat;

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::PatronComponent,
                            fpl::fpl_project::PatronData)

namespace fpl {
namespace fpl_project {

using fpl::component_library::AnimationComponent;
using fpl::component_library::AnimationData;
using fpl::component_library::CollisionPayload;
using fpl::component_library::CommonServicesComponent;
using fpl::component_library::PhysicsComponent;
using fpl::component_library::PhysicsData;
using fpl::component_library::RenderMeshComponent;
using fpl::component_library::RenderMeshData;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;
using motive::MotiveTime;

// All of these numbers were picked for purely aesthetic reasons:
static const float kSplatterCount = 10;

void PatronComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  event_manager_->RegisterListener(EventSinkUnion_Collision, this);
  event_manager_->RegisterListener(EventSinkUnion_EditorEvent, this);
}

void PatronComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* raw_data) {
  auto patron_def = static_cast<const PatronDef*>(raw_data);
  PatronData* patron_data = AddEntity(entity);
  if (patron_def->on_collision() != nullptr) {
    if (entity_manager_->GetComponent<CommonServicesComponent>()
            ->entity_factory()
            ->WillBeKeptInMemory(patron_def->on_collision())) {
      patron_data->on_collision = patron_def->on_collision();
    } else {
      // Copy the on_collision into a new flatbuffer that we own.
      flatbuffers::FlatBufferBuilder fbb;
      auto binary_schema = entity_manager_->GetComponent<ServicesComponent>()
                               ->component_def_binary_schema();
      auto schema = reflection::GetSchema(binary_schema);
      auto table_def = schema->objects()->LookupByKey("ActionDef");
      flatbuffers::Offset<ActionDef> table =
          flatbuffers::CopyTable(
              fbb, *schema, *table_def,
              (const flatbuffers::Table&)*patron_def->on_collision())
              .o;
      fbb.Finish(table);
      patron_data->on_collision_flatbuffer = {
          fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize()};
      patron_data->on_collision = flatbuffers::GetRoot<TaggedActionDefList>(
          patron_data->on_collision_flatbuffer.data());
    }
  }
  assert(patron_def->pop_out_radius() >= patron_def->pop_in_radius());
  if (patron_def->pop_in_radius()) {
    patron_data->pop_in_radius = patron_def->pop_in_radius();
    patron_data->pop_in_radius_squared =
        patron_data->pop_in_radius * patron_data->pop_in_radius;
  }
  if (patron_def->pop_out_radius()) {
    patron_data->pop_out_radius = patron_def->pop_out_radius();
    patron_data->pop_out_radius_squared =
        patron_data->pop_out_radius * patron_data->pop_out_radius;
  }

  if (patron_def->min_lap() >= 0) {
    patron_data->min_lap = patron_def->min_lap();
  }

  if (patron_def->max_lap() >= 0) {
    patron_data->max_lap = patron_def->max_lap();
  }

  if (patron_def->target_tag()) {
    patron_data->target_tag = patron_def->target_tag()->str();
  }
}

entity::ComponentInterface::RawDataUniquePtr PatronComponent::ExportRawData(
    const entity::EntityRef& entity) const {
  const PatronData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  auto schema_file = entity_manager_->GetComponent<ServicesComponent>()
                         ->component_def_binary_schema();
  auto schema =
      schema_file != nullptr ? reflection::GetSchema(schema_file) : nullptr;
  auto table_def = schema != nullptr
                       ? schema->objects()->LookupByKey("TaggedActionDefList")
                       : nullptr;
  auto on_collision =
      data->on_collision != nullptr && table_def != nullptr
          ? flatbuffers::Offset<TaggedActionDefList>(
                flatbuffers::CopyTable(
                    fbb, *schema, *table_def,
                    (const flatbuffers::Table&)(*data->on_collision))
                    .o)
          : 0;
  auto target_tag = fbb.CreateString(data->target_tag);

  // Get all the on_collision events
  PatronDefBuilder builder(fbb);
  if (on_collision.o != 0) {
    builder.add_on_collision(on_collision);
  }
  builder.add_min_lap(data->min_lap);
  builder.add_max_lap(data->max_lap);
  builder.add_pop_in_radius(data->pop_in_radius);
  builder.add_pop_out_radius(data->pop_out_radius);
  builder.add_target_tag(target_tag);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void PatronComponent::InitEntity(entity::EntityRef& entity) { (void)entity; }

void PatronComponent::PostLoadFixup() {
  const TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();

  // Initialize each patron.
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef patron = iter->entity;
    PatronData* patron_data = Data<PatronData>(patron);

    // Get reference to the first child with a rendermesh. We assume there will
    // only be one such child.
    patron_data->render_child = transform_component->ChildWithComponent(
        patron, RenderMeshComponent::GetComponentId());
    assert(patron_data->render_child);

    // Animate the entity with the rendermesh.
    entity_manager_->AddEntityToComponent<AnimationComponent>(
        patron_data->render_child);

    // Initialize state machine.
    patron_data->state = kPatronStateLayingDown;

    // Patrons that are done should not have physics enabled.
    physics_component->DisablePhysics(patron);
    // We don't want patrons moving until they are up.
    RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
    if (rail_denizen_data != nullptr) {
      rail_denizen_data->enabled = false;
    }
  }
}

void PatronComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  if (!raft) return;
  TransformData* raft_transform = raft ? Data<TransformData>(raft) : nullptr;
  RailDenizenData* raft_rail_denizen =
      raft ? Data<RailDenizenData>(raft) : nullptr;
  int lap = raft_rail_denizen != nullptr ? raft_rail_denizen->lap : 0;
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef patron = iter->entity;
    TransformData* transform_data = Data<TransformData>(patron);
    PatronData* patron_data = Data<PatronData>(patron);
    const PatronState state = patron_data->state;

    RenderMeshComponent* rm_component =
        entity_manager_->GetComponent<RenderMeshComponent>();
    TransformComponent* tf_component =
        entity_manager_->GetComponent<TransformComponent>();

    rm_component->SetHiddenRecursively(tf_component->GetRootParent(patron),
                                       state == kPatronStateLayingDown);

    // Determine the patron's distance from the raft.
    float raft_distance_squared =
        (transform_data->position - raft_transform->position).LengthSquared();
    if (raft_distance_squared > patron_data->pop_out_radius_squared &&
        (state == kPatronStateUpright || state == kPatronStateGettingUp)) {
      // If you are too far away, and the patron is standing up (or getting up)
      // make them fall back down.
      patron_data->state = kPatronStateFalling;
      Animate(patron_data, PatronAction_Fall);

      auto physics_component =
          entity_manager_->GetComponent<PhysicsComponent>();
      physics_component->DisablePhysics(patron);
      auto rail_denizen_data = Data<RailDenizenData>(patron);
      if (rail_denizen_data != nullptr) {
        rail_denizen_data->enabled = false;
      }
    } else if (raft_distance_squared <= patron_data->pop_in_radius_squared &&
               lap > patron_data->last_lap_fed && lap >= patron_data->min_lap &&
               lap <= patron_data->max_lap &&
               (state == kPatronStateLayingDown ||
                state == kPatronStateFalling)) {
      // If you are in range, and the patron is standing laying down (or falling
      // down) and they have not been fed this lap, and they are in the range of
      // laps in which they should appear, make them stand back up.
      patron_data->state = kPatronStateGettingUp;
      Animate(patron_data, PatronAction_GetUp);
    }

    // Transition to the next state if we're at the end of the current
    // animation.
    AnimationData* anim_data = Data<AnimationData>(patron_data->render_child);
    if (anim_data->motivator.Valid()) {
      const MotiveTime time_remaining = anim_data->motivator.TimeRemaining();
      const bool anim_ending =
          time_remaining < static_cast<MotiveTime>(delta_time);
      if (anim_ending) {
        switch (patron_data->state) {
          case kPatronStateFed:
            patron_data->state = kPatronStateFalling;
            Animate(patron_data, PatronAction_Fall);
            break;

          case kPatronStateFalling:
            patron_data->state = kPatronStateLayingDown;
            break;

          case kPatronStateGettingUp: {
            patron_data->state = kPatronStateUpright;
            auto physics_component =
                entity_manager_->GetComponent<PhysicsComponent>();
            physics_component->EnablePhysics(patron);
            auto rail_denizen_data = Data<RailDenizenData>(patron);
            if (rail_denizen_data != nullptr) {
              rail_denizen_data->enabled = true;
            }
          }  // fallthrough

          case kPatronStateUpright:
            Animate(patron_data, PatronAction_Idle);
            break;

          default:
            break;
        }
      }
    }
  }
}

void PatronComponent::Animate(const PatronData* patron_data,
                              PatronAction action) {
  auto services_component = entity_manager_->GetComponent<ServicesComponent>();
  const motive::AnimTable* anim_table = services_component->anim_table();
  const motive::MatrixAnim& anim = anim_table->Query(AnimObject_Patron, action);

  entity_manager_->GetComponent<AnimationComponent>()->Animate(
      patron_data->render_child, anim);
}

void PatronComponent::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_Collision: {
      auto* collision = event_payload.ToData<CollisionPayload>();
      if (collision->entity_a->IsRegisteredForComponent(GetComponentId())) {
        HandleCollision(collision->entity_a, collision->entity_b,
                        collision->tag_a);
      } else if (collision->entity_b->IsRegisteredForComponent(
                     GetComponentId())) {
        HandleCollision(collision->entity_b, collision->entity_a,
                        collision->tag_b);
      }
      break;
    }
    case EventSinkUnion_EditorEvent: {
      auto* event = event_payload.ToData<editor::EditorEventPayload>();
      auto physics_component =
          entity_manager_->GetComponent<PhysicsComponent>();
      if (event->action == EditorEventAction_Enter) {
        // Make the patrons stand up
        for (auto iter = component_data_.begin(); iter != component_data_.end();
             ++iter) {
          entity::EntityRef patron = iter->entity;
          physics_component->UpdatePhysicsFromTransform(patron);
          physics_component->EnablePhysics(patron);
        }
      } else if (event->action == EditorEventAction_Exit) {
        PostLoadFixup();
      }
      break;
    }
    default: { assert(0); }
  }
}

void PatronComponent::HandleCollision(const entity::EntityRef& patron_entity,
                                      const entity::EntityRef& proj_entity,
                                      const std::string& part_tag) {
  // We only care about collisions with projectiles that haven't been deleted.
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(proj_entity);
  if (projectile_data == nullptr || proj_entity->marked_for_deletion()) {
    return;
  }
  entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  PatronData* patron_data = Data<PatronData>(patron_entity);
  if (patron_data->state == kPatronStateUpright) {
    // If the target tag was hit, consider it being fed
    if (patron_data->target_tag == "" || patron_data->target_tag == part_tag) {
      // TODO: Make state change an action.
      patron_data->state = kPatronStateFed;
      Animate(patron_data, PatronAction_Fed);
      patron_data->last_lap_fed = raft_rail_denizen->lap;

      // Disable physics and rail movement after they have been fed
      auto physics_component =
          entity_manager_->GetComponent<PhysicsComponent>();
      physics_component->DisablePhysics(patron_entity);
      auto rail_denizen_data = Data<RailDenizenData>(patron_entity);
      if (rail_denizen_data != nullptr) {
        rail_denizen_data->enabled = false;
      }

      SpawnPointDisplay(Data<TransformData>(proj_entity)->position);
    }

    // Send events to every on_collision listener with the correct tag.
    size_t on_collision_size =
        (patron_data->on_collision && patron_data->on_collision->action_list())
            ? patron_data->on_collision->action_list()->size()
            : 0;
    EventContext context;
    context.source_owner = projectile_data->owner;
    context.source = proj_entity;
    context.target = patron_entity;
    context.raft = raft;
    for (size_t i = 0; i < on_collision_size; ++i) {
      auto* tagged_action = patron_data->on_collision->action_list()->Get(i);
      const char* tag =
          tagged_action->tag() ? tagged_action->tag()->c_str() : nullptr;
      if (tag && strcmp(tag, part_tag.c_str()) == 0) {
        const ActionDef* action = tagged_action->action();
        ParseAction(action, &context, event_manager_, entity_manager_);
        break;
      }
    }

    // Even if you didn't hit the top, if got here, you got some
    // kind of collision, so you get a splatter.
    TransformData* proj_transform = Data<TransformData>(proj_entity);
    SpawnSplatter(proj_transform->position, kSplatterCount);
    entity_manager_->DeleteEntity(proj_entity);
  }
}

void PatronComponent::SpawnPointDisplay(const vec3& position) {
  // We need the raft, so we can orient towards it:
  entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  if (!raft) return;

  // Spawn from prototype:
  entity::EntityRef point_display =
      entity_manager_->GetComponent<ServicesComponent>()
          ->entity_factory()
          ->CreateEntityFromPrototype("FloatingPointDisplay", entity_manager_);

  TransformData* points_transform = Data<TransformData>(point_display);
  TransformData* raft_transform = Data<TransformData>(raft);
  points_transform->position = position;
  vec3 facing_vector = points_transform->position - raft_transform->position;
  facing_vector.z() = 0;
  points_transform->orientation =
      mathfu::quat::RotateFromTo(facing_vector, vec3(0, 1, 0));
}

void PatronComponent::SpawnSplatter(const mathfu::vec3& position, int count) {
  // Save the position off, as the CreateEntity call can cause the reference to
  // become invalid.
  mathfu::vec3 pos = position;

  for (int i = 0; i < count; i++) {
    entity::EntityRef particle =
        entity_manager_->GetComponent<ServicesComponent>()
            ->entity_factory()
            ->CreateEntityFromPrototype("SplatterParticle", entity_manager_);

    TransformData* transform_data =
        entity_manager_->GetComponentData<TransformData>(particle);
    PhysicsData* physics_data =
        entity_manager_->GetComponentData<PhysicsData>(particle);

    transform_data->position = pos;

    physics_data->SetVelocity(vec3(mathfu::RandomInRange(-3.0f, 3.0f),
                                   mathfu::RandomInRange(-3.0f, 3.0f),
                                   mathfu::RandomInRange(0.0f, 6.0f)));
    physics_data->SetAngularVelocity(vec3(mathfu::RandomInRange(1.0f, 2.0f),
                                          mathfu::RandomInRange(1.0f, 2.0f),
                                          mathfu::RandomInRange(1.0f, 2.0f)));

    auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
    physics_component->UpdatePhysicsFromTransform(particle);
  }
}

}  // fpl_project
}  // fpl
