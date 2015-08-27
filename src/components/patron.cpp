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
using fpl::component_library::RigidBodyData;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;
using fpl::entity::EntityRef;
using mathfu::kZeros3f;
using mathfu::quat;
using mathfu::vec3;
using motive::MotiveTime;


// All of these numbers were picked for purely aesthetic reasons:
static const float kSplatterCount = 10;
static const float kLapWaitAmount = 0.5f;
static const float kMaxCatchDist = 5.0f;
static const float kHeightRangeBuffer = 0.05f;
static const Range kCatchTimeRangeInSeconds(0.01f, 10.0f);


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
      patron_data->on_collision_flatbuffer = std::vector<uint8_t>(
          fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize());
      patron_data->on_collision = flatbuffers::GetRoot<TaggedActionDefList>(
          patron_data->on_collision_flatbuffer.data());
    }
  }
  patron_data->anim_object = patron_def->anim_object();
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
    AnimationData* animation_data =
        Data<AnimationData>(patron_data->render_child);
    animation_data->anim_table_object = patron_data->anim_object;

    // Initialize state machine.
    patron_data->state = kPatronStateLayingDown;

    // Cache the index into the physics target body.
    const PhysicsData* physics_data = Data<PhysicsData>(patron);
    const int target_index =
        physics_data->RigidBodyIndex(patron_data->target_tag);
    patron_data->target_rigid_body_index = target_index < 0 ? 0 : target_index;

    // Patrons that are done should not have physics enabled.
    physics_component->DisablePhysics(patron);
    // We don't want patrons moving until they are up.
    RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
    if (rail_denizen_data != nullptr) {
      rail_denizen_data->enabled = false;
    }
  }
}

void PatronComponent::UpdateMovement(const EntityRef& patron) {
  TransformData* transform_data = Data<TransformData>(patron);
  PatronData* patron_data = Data<PatronData>(patron);

  // Add on delta to the position.
  if (patron_data->delta_position.Valid()) {
    const vec3 delta_position = patron_data->delta_position.Value();
    transform_data->position += delta_position -
                                vec3(patron_data->prev_delta_position);
    patron_data->prev_delta_position = delta_position;

    if (patron_data->delta_position.TargetTime() <= 0) {
      patron_data->delta_position.Invalidate();
    }
  }

  // Add on delta to face angle.
  if (patron_data->delta_face_angle.Valid()) {
    const Angle cur_face_angle(transform_data->orientation.ToEulerAngles().z());

    const Angle delta_face_angle(patron_data->delta_face_angle.Value());
    const Angle delta = delta_face_angle - patron_data->prev_delta_face_angle;
    transform_data->orientation = transform_data->orientation *
                                  quat::FromAngleAxis(delta.ToRadians(),
                                                      mathfu::kAxisZ3f);
    patron_data->prev_delta_face_angle = delta_face_angle;

    if (patron_data->delta_face_angle.TargetTime() <= 0) {
      patron_data->delta_face_angle.Invalidate();
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
  float lap = raft_rail_denizen != nullptr ? raft_rail_denizen->lap : 0.0f;
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef patron = iter->entity;
    TransformData* transform_data = Data<TransformData>(patron);
    PatronData* patron_data = Data<PatronData>(patron);
    const PatronState state = patron_data->state;

    // Move patron towards the target.
    UpdateMovement(patron);

    if (state == kPatronStateUpright) {
      if (!patron_data->delta_position.Valid()) {
        CatchProjectile(patron);
      }
    }

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
               lap > patron_data->last_lap_fed + kLapWaitAmount &&
               lap >= patron_data->min_lap && lap <= patron_data->max_lap &&
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
  entity_manager_->GetComponent<AnimationComponent>()->AnimateFromTable(
      patron_data->render_child, action);
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

static inline vec3 ZeroHeight(const vec3& v) {
  vec3 v_copy = v;
  v_copy.z() = 0.0f;
  return v_copy;
}

static float CalculateClosestTimeInHeightRange(
    float target_t, const Range& valid_times, const Range& valid_heights,
    float start_height, float start_speed, float gravity) {

  // Let `h(t)` represent the height at time `t`.
  // Then,
  //   h(0) = start_height
  //   h'(0) = start_speed
  //   h''(t) = gravity
  // So,
  //   h(t) = 0.5*gravity*t^2 + start_speed*t + start_height

  // Create curves with roots at the min and max heights.
  const float half_gravity = 0.5f * gravity;
  const QuadraticCurve below_max_curve(half_gravity, start_speed,
                                       start_height - valid_heights.end());
  const QuadraticCurve above_min_curve(half_gravity, start_speed,
                                       start_height - valid_heights.start());

  // Find time ranges where those curves are in the valid range.
  QuadraticCurve::RangeArray below_max_ranges;
  QuadraticCurve::RangeArray above_min_ranges;
  below_max_curve.RangesBelowZero(valid_times, &below_max_ranges);
  above_min_curve.RangesAboveZero(valid_times, &above_min_ranges);

  // Fine time ranges where both curves are in the valid range.
  Range::RangeArray<4> valid_ranges;
  Range::IntersectRanges(below_max_ranges, above_min_ranges, &valid_ranges);

  // TODO: Prefer later times, given the same distance, since we want more
  //       time to move to the catch point.
  const float closest_t = Range::ClampToClosest(target_t, valid_ranges);
  return closest_t;
}

Range PatronComponent::TargetHeightRange(const EntityRef& patron) const {
  // Get the physics rigid body data for the patron's target.
  const PatronData* patron_data = GetComponentData(patron);
  const PhysicsData* physics_data = Data<PhysicsData>(patron);

  // Get the axis-aligned bounding box of the patron's target.
  vec3 target_min;
  vec3 target_max;
  physics_data->GetAabb(patron_data->target_rigid_body_index, &target_min,
                        &target_max);

  // Return the heights.
  return Range(target_min.z() + kHeightRangeBuffer,
               target_max.z() - kHeightRangeBuffer);
}

const EntityRef* PatronComponent::ClosestProjectile(
    const EntityRef& patron, vec3* closest_position, Angle* closest_face_angle,
    float* closest_time) const {

  // TODO: change projectile_component to const when Component gets a
  //       const_iterator.
  PlayerProjectileComponent* projectile_component =
      entity_manager_->GetComponent<PlayerProjectileComponent>();
  const TransformData* patron_transform = Data<TransformData>(patron);

  // Gather patron details. These are independent of the projectiles.
  const vec3 patron_position_xy = ZeroHeight(patron_transform->position);
  const Range target_height_range = TargetHeightRange(patron);

  // Loop through every projectile. Keep a reference to the closest one.
  const EntityRef* closest_ref = nullptr;
  float closest_dist_sq = kMaxCatchDist * kMaxCatchDist;
  for (auto it = projectile_component->begin(); it != projectile_component->end();
       ++it) {
    // Get movement state of projectile.
    const TransformData* projectile_transform =
        entity_manager_->GetComponentData<TransformData>(it->entity);
    const PhysicsData* projectile_physics =
        entity_manager_->GetComponentData<PhysicsData>(it->entity);
    const vec3 projectile_position = projectile_transform->position;
    const vec3 projectile_velocity = projectile_physics->Velocity(); // In m/s.
    const vec3 projectile_position_xy = ZeroHeight(projectile_position);
    const vec3 projectile_velocity_xy = ZeroHeight(projectile_velocity);

    // Get horizontal unit vector and distance to patron.
    vec3 to_patron_xy = patron_position_xy - projectile_position_xy;
    const float dist_to_patron_xy = to_patron_xy.Normalize();

    // Get time to closest point on horizontal trajectory.
    // Dot product is projectile speet along
    const float projectile_speed_to_patron =
        vec3::DotProduct(projectile_velocity, to_patron_xy);
    if (projectile_speed_to_patron <= 0.0f) continue;
    const float closest_t_ignore_hieght =
        dist_to_patron_xy / projectile_speed_to_patron; // In seconds.

    // Early reject if the distance is already too far.
    const vec3 closest_position_ignore_hieght_xy =
        projectile_position_xy + projectile_velocity_xy * closest_t_ignore_hieght;
    const float dist_sq_ignore_hieght =
        (patron_position_xy - closest_position_ignore_hieght_xy).LengthSquared();
    if (dist_sq_ignore_hieght > closest_dist_sq) continue;

    // Get the closest time at a catchable hieght.
    const float closest_t = CalculateClosestTimeInHeightRange(
        closest_t_ignore_hieght, kCatchTimeRangeInSeconds, target_height_range,
        projectile_position.z(), projectile_velocity.z(), config_->gravity());
    if (closest_t <= 0.0f) continue;

    // Claculate the projectile position at `closest_t`.
    const vec3 closest_position_xy = projectile_position_xy +
                                     projectile_velocity_xy * closest_t;
    const float dist_sq =
        (patron_position_xy - closest_position_xy).LengthSquared();
    if (dist_sq > closest_dist_sq) continue;

    // TODO: prefer projectiles that are slightly farther but with much more
    //       time to close the distance.
    *closest_position = closest_position_xy;
    closest_position->z() = patron_transform->position.z();
    *closest_time = closest_t;
    *closest_face_angle = Angle::FromYXVector(projectile_position_xy -
                                              closest_position_xy);
    closest_ref = &it->entity;
    closest_dist_sq = dist_sq;
  }
  return closest_ref;
}

void PatronComponent::CatchProjectile(const EntityRef& patron) {
  // Find the projectile that's the closest.
  vec3 closest_position;
  Angle closest_face_angle;
  float closest_time;
  const EntityRef* closest_projectile = ClosestProjectile(
      patron, &closest_position, &closest_face_angle, &closest_time);

  // Initialize movement to that spot.
  if (closest_projectile != nullptr) {
    MoveToTarget(patron, closest_position, closest_face_angle, closest_time);
  }
}

void PatronComponent::MoveToTarget(
    const EntityRef& patron, const vec3& target_position,
    Angle target_face_angle, float target_time) {

  // TODO: When patron is moving on tracks, get the future position and
  //       face angle from the tracks instead.
  const TransformData* patron_transform = Data<TransformData>(patron);
  const vec3 future_position = patron_transform->position;
  const Angle future_face_angle(patron_transform->orientation.ToEulerAngles().z());

  // At `target_time` we want to achieve these deltas so that our position and
  // face angle with equal `target_position` and `target_face_angle`.
  const vec3 delta_position = target_position - future_position;
  const Angle delta_face_angle = target_face_angle - future_face_angle;
  const motive::MotiveTime target_time_ms =
      static_cast<motive::MotiveTime>(1000.0f * target_time);

  // Set the delta movement Motivators.
  PatronData* patron_data = Data<PatronData>(patron);
  if (!patron_data->delta_position.Valid()) {
    // Initialize from scratch.
    assert(!patron_data->delta_face_angle.Valid());

    patron_data->prev_delta_position = vec3(kZeros3f);
    patron_data->prev_delta_face_angle = Angle(0.0f);

    motive::MotiveEngine* motive_engine =
        &entity_manager_->GetComponent<AnimationComponent>()->engine();

    patron_data->delta_position.InitializeWithTarget(
        motive::SmoothInit(), motive_engine,
        motive::Tar3f::CurrentToTarget(kZeros3f, kZeros3f, delta_position,
                                       kZeros3f, target_time_ms));

    const Range angle_range(-kPi, kPi);
    patron_data->delta_face_angle.InitializeWithTarget(
        motive::SmoothInit(angle_range, true), motive_engine,
        motive::CurrentToTarget1f(0.0f, 0.0f, delta_face_angle.ToRadians(),
                                  0.0f, target_time_ms));

  } else {
    // Reset the current target to go to the new position.
    patron_data->delta_position.SetTarget(
        motive::Tar3f::Target(delta_position, kZeros3f, target_time_ms));
    patron_data->delta_face_angle.SetTarget(
        motive::Target1f(delta_face_angle.ToRadians(), 0.0f, target_time_ms));
  }
}

}  // fpl_project
}  // fpl

