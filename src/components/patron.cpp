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
#include "component_library/graph.h"
#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "component_library/transform.h"
#include "components/attributes.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "mathfu/glsl_mappings.h"
#include "motive/anim.h"
#include "motive/anim_table.h"
#include "world.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::PatronComponent,
                            fpl::fpl_project::PatronData)

namespace fpl {
namespace fpl_project {

using fpl::component_library::AnimationComponent;
using fpl::component_library::AnimationData;
using fpl::component_library::CollisionData;
using fpl::component_library::PhysicsComponent;
using fpl::component_library::PhysicsData;
using fpl::component_library::RenderMeshComponent;
using fpl::component_library::RenderMeshData;
using fpl::component_library::RigidBodyData;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;
using fpl::editor::WorldEditor;
using fpl::entity::EntityRef;
using mathfu::kZeros3f;
using mathfu::quat;
using mathfu::vec3;
using motive::MotiveTime;
using motive::kMotiveTimeEndless;

// All of these numbers were picked for purely aesthetic reasons:
static const float kLapWaitAmount = 0.5f;
static const float kMaxCatchTime = 1.5f;
static const float kHeightRangeBuffer = 0.05f;
static const Range kCatchTimeRangeInSeconds(0.01f, kMaxCatchTime);
static const float kCatchReturnTime = 1.0f;

static inline vec3 ZeroHeight(const vec3& v) {
  vec3 v_copy = v;
  v_copy.z() = 0.0f;
  return v_copy;
}

void PatronComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  // World editor is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a world editor.
  WorldEditor* world_editor = services->world_editor();
  if (world_editor) {
    world_editor->AddOnEnterEditorCallback(
        [this]() { UpdateAndEnablePhysics(); });
    world_editor->AddOnExitEditorCallback([this]() { PostLoadFixup(); });
  }
}

static float PopRadius(float desired, float min) {
  return desired < 0.0f ? min : std::min(desired, min);
}

void PatronComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* raw_data) {
  auto patron_def = static_cast<const PatronDef*>(raw_data);
  PatronData* patron_data = AddEntity(entity);
  patron_data->anim_object = patron_def->anim_object();

  // Must pop-in closer than popping-out, which is closer than being culled.
  auto render_config = config_->rendering_config();
  assert(patron_def->pop_out_radius() >= patron_def->pop_in_radius());
  assert(render_config->cull_distance() >= render_config->pop_out_distance() &&
         render_config->pop_out_distance() >= render_config->pop_in_distance());

  patron_data->pop_in_radius =
      PopRadius(patron_def->pop_in_radius(), render_config->pop_in_distance());
  patron_data->pop_out_radius = PopRadius(patron_def->pop_out_radius(),
                                          render_config->pop_out_distance());
  patron_data->pop_in_radius_squared =
      patron_data->pop_in_radius * patron_data->pop_in_radius;
  patron_data->pop_out_radius_squared =
      patron_data->pop_out_radius * patron_data->pop_out_radius;

  patron_data->min_lap = patron_def->min_lap();
  patron_data->max_lap = patron_def->max_lap();

  if (patron_def->target_tag()) {
    patron_data->target_tag = patron_def->target_tag()->str();
  }

  patron_data->max_catch_distance = patron_def->max_catch_distance();
  patron_data->max_catch_angle = patron_def->max_catch_angle();
  patron_data->point_display_height = patron_def->point_display_height();
}

entity::ComponentInterface::RawDataUniquePtr PatronComponent::ExportRawData(
    const entity::EntityRef& entity) const {
  const PatronData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  auto target_tag = fbb.CreateString(data->target_tag);

  // Get all the on_collision events
  PatronDefBuilder builder(fbb);
  builder.add_min_lap(data->min_lap);
  builder.add_max_lap(data->max_lap);
  builder.add_pop_in_radius(data->pop_in_radius);
  builder.add_pop_out_radius(data->pop_out_radius);
  builder.add_target_tag(target_tag);
  builder.add_max_catch_distance(data->max_catch_distance);
  builder.add_max_catch_angle(data->max_catch_angle);
  builder.add_point_display_height(data->point_display_height);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void PatronComponent::InitEntity(entity::EntityRef& entity) { (void)entity; }

void PatronComponent::UpdateAndEnablePhysics() {
  // Make the patrons stand up
  RenderMeshComponent* render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef patron = iter->entity;
    physics_component->UpdatePhysicsFromTransform(patron);
    physics_component->EnablePhysics(patron);

    render_mesh_component->SetHiddenRecursively(
        transform_component->GetRootParent(patron), false);
  }
}

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
      rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
    }
  }
}

void PatronComponent::UpdateMovement(const EntityRef& patron) {
  TransformData* transform_data = Data<TransformData>(patron);
  PatronData* patron_data = Data<PatronData>(patron);

  // Add on delta to the position.
  if (patron_data->delta_position.Valid()) {
    const vec3 delta_position = patron_data->delta_position.Value();
    transform_data->position +=
        delta_position - vec3(patron_data->prev_delta_position);
    patron_data->prev_delta_position = delta_position;

    if (patron_data->delta_position.TargetTime() <= 0) {
      if (patron_data->catching_state == kCatchingStateMoveToTarget) {
        const vec3 raft_position = RaftPosition();
        const Angle return_angle =
            Angle::FromYXVector(raft_position - patron_data->return_position);
        MoveToTarget(patron, patron_data->return_position, return_angle,
                     kCatchReturnTime);
        patron_data->catching_state = kCatchingStateReturn;
      } else {
        patron_data->delta_position.Invalidate();
        patron_data->catching_state = kCatchingStateIdle;
        // Back to idle, so resume moving on rails, if it is on one.
        RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
        if (rail_denizen_data != nullptr &&
            patron_data->state == kPatronStateUpright) {
          rail_denizen_data->enabled = true;
          rail_denizen_data->motivator.SetSplinePlaybackRate(
              rail_denizen_data->spline_playback_rate);
        }
      }
    }
  }

  // Add on delta to face angle.
  if (patron_data->delta_face_angle.Valid()) {
    const Angle delta_face_angle(patron_data->delta_face_angle.Value());
    const Angle delta = delta_face_angle - patron_data->prev_delta_face_angle;
    transform_data->orientation =
        transform_data->orientation *
        quat::FromAngleAxis(delta.ToRadians(), mathfu::kAxisZ3f);
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

    if (state == kPatronStateUpright &&
        patron_data->catching_state != kCatchingStateMoveToTarget) {
      FindProjectileAndCatch(patron);
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
        rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
      }
    } else if (raft_distance_squared <= patron_data->pop_in_radius_squared &&
               lap > patron_data->last_lap_fed + kLapWaitAmount &&
               lap >= patron_data->min_lap &&
               (lap <= patron_data->max_lap || patron_data->max_lap < 0) &&
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
      const MotiveTime anim_delta_time = static_cast<MotiveTime>(delta_time);

      const MotiveTime time_remaining = anim_data->motivator.TimeRemaining();
      const bool anim_ending = time_remaining < anim_delta_time;

      if (anim_ending) {
        switch (patron_data->state) {
          case kPatronStateEating:
            if (HasAnim(patron_data, PatronAction_Satisfied)) {
              patron_data->state = kPatronStateSatisfied;
              Animate(patron_data, PatronAction_Satisfied);
              break;
            }
          // fallthrough
          // Fallthrough to "falling" state if satisfied animation doesn't
          // exist.

          case kPatronStateSatisfied:
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
              rail_denizen_data->motivator.SetSplinePlaybackRate(
                  rail_denizen_data->spline_playback_rate);
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

bool PatronComponent::HasAnim(const PatronData* patron_data,
                              PatronAction action) const {
  return entity_manager_->GetComponent<AnimationComponent>()->HasAnim(
      patron_data->render_child, action);
}

void PatronComponent::Animate(PatronData* patron_data, PatronAction action) {
  entity_manager_->GetComponent<AnimationComponent>()->AnimateFromTable(
      patron_data->render_child, action);
}

void PatronComponent::CollisionHandler(CollisionData* collision_data,
                                       void* user_data) {
  PatronComponent* patron_component = static_cast<PatronComponent*>(user_data);
  entity::ComponentId id = GetComponentId();
  if (collision_data->this_entity->IsRegisteredForComponent(id)) {
    patron_component->HandleCollision(collision_data->this_entity,
                                      collision_data->other_entity,
                                      collision_data->this_tag);
  } else if (collision_data->other_entity->IsRegisteredForComponent(id)) {
    patron_component->HandleCollision(collision_data->other_entity,
                                      collision_data->this_entity,
                                      collision_data->other_tag);
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
      patron_data->state = kPatronStateEating;
      Animate(patron_data, PatronAction_Eat);
      patron_data->last_lap_fed = raft_rail_denizen->lap;

      // Disable physics and rail movement after they have been fed
      auto physics_component =
          entity_manager_->GetComponent<PhysicsComponent>();
      physics_component->DisablePhysics(patron_entity);
      auto rail_denizen_data = Data<RailDenizenData>(patron_entity);
      if (rail_denizen_data != nullptr) {
        rail_denizen_data->enabled = false;
        rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
      }
      SpawnPointDisplay(patron_entity);
      // Delete the projectile, as it has been consumed.
      entity_manager_->DeleteEntity(proj_entity);
    }
  }
}

void PatronComponent::SpawnPointDisplay(const entity::EntityRef& patron) {
  // We need the raft, so we can orient towards it:
  if (!RaftExists()) return;

  // Spawn from prototype:
  entity::EntityRef point_display =
      entity_manager_->GetComponent<ServicesComponent>()
          ->entity_factory()
          ->CreateEntityFromPrototype("FloatingPointDisplay", entity_manager_);

  // Make the point display a child of the patron. We want it to move with
  // the patron.
  // Note--the const_cast is lamentable. I think AddChild should take a
  // const EntityRef&, like most other things.
  auto transform_component = GetComponent<TransformComponent>();
  transform_component->AddChild(point_display, const_cast<EntityRef&>(patron));

  // Set the position offset so the heart displays above the patron.
  const PatronData* patron_data = Data<PatronData>(patron);
  TransformData* points_transform = Data<TransformData>(point_display);
  points_transform->position =
      patron_data->point_display_height * mathfu::kAxisZ3f;
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

bool PatronComponent::RaftExists() const {
  return entity_manager_->GetComponent<ServicesComponent>()
      ->raft_entity()
      .IsValid();
}

vec3 PatronComponent::RaftPosition() const {
  assert(RaftExists());
  const EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  const TransformData* raft_transform = Data<TransformData>(raft);
  return raft_transform->position;
}

const EntityRef* PatronComponent::ClosestProjectile(const EntityRef& patron,
                                                    vec3* closest_position,
                                                    Angle* closest_face_angle,
                                                    float* closest_time) const {
  // TODO: change projectile_component to const when Component gets a
  //       const_iterator.
  PlayerProjectileComponent* projectile_component =
      entity_manager_->GetComponent<PlayerProjectileComponent>();
  const TransformData* patron_transform = Data<TransformData>(patron);
  const PatronData* patron_data = GetComponentData(patron);

  // Gather patron details. These are independent of the projectiles.
  const vec3 patron_position_xy = ZeroHeight(patron_transform->position);
  const Range target_height_range = TargetHeightRange(patron);
  const vec3 return_position_xy = ZeroHeight(patron_data->return_position);

  // Gather data about the raft, which is needed in the calculations.
  const vec3 raft_position_xy = ZeroHeight(RaftPosition());

  // Loop through every projectile. Keep a reference to the closest one.
  const EntityRef* closest_ref = nullptr;
  float max_dist_sq =
      patron_data->max_catch_distance * patron_data->max_catch_distance;
  float closest_dist_sq = max_dist_sq;
  for (auto it = projectile_component->begin();
       it != projectile_component->end(); ++it) {
    // Get movement state of projectile.
    const TransformData* projectile_transform =
        entity_manager_->GetComponentData<TransformData>(it->entity);
    const PhysicsData* projectile_physics =
        entity_manager_->GetComponentData<PhysicsData>(it->entity);
    const vec3 projectile_position = projectile_transform->position;
    const vec3 projectile_velocity = projectile_physics->Velocity();  // In m/s.
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
    const float closest_t_ignore_height =
        dist_to_patron_xy / projectile_speed_to_patron;  // In seconds.

    // Early reject if the distance is already too far.
    const vec3 closest_position_ignore_height_xy =
        projectile_position_xy +
        projectile_velocity_xy * closest_t_ignore_height;
    const float dist_sq_ignore_height =
        (patron_position_xy - closest_position_ignore_height_xy)
            .LengthSquared();
    if (dist_sq_ignore_height > closest_dist_sq) continue;

    // If returning from a previous attempt, limit how far from the initial
    // position to leave from again.
    if (patron_data->catching_state == kCatchingStateReturn) {
      const float dist_sq =
          (return_position_xy - closest_position_ignore_height_xy)
              .LengthSquared();
      if (dist_sq > max_dist_sq) continue;
    }

    // Get the closest time at a catchable height.
    const float closest_t = CalculateClosestTimeInHeightRange(
        closest_t_ignore_height, kCatchTimeRangeInSeconds, target_height_range,
        projectile_position.z(), projectile_velocity.z(), config_->gravity());
    if (closest_t <= 0.0f || closest_t > kMaxCatchTime) continue;

    // Calculate the projectile position at `closest_t`.
    const vec3 closest_position_xy =
        projectile_position_xy + projectile_velocity_xy * closest_t;
    const float dist_sq =
        (patron_position_xy - closest_position_xy).LengthSquared();
    if (dist_sq > closest_dist_sq) continue;

    // Don't face too far away from the player in order to catch thrown sushi.
    Angle angle_to_sushi =
        Angle::FromYXVector(projectile_position_xy - closest_position_xy);
    Angle angle_to_raft =
        Angle::FromYXVector(raft_position_xy - closest_position_xy);
    Angle difference = angle_to_raft - angle_to_sushi;
    if (fabs(difference.ToDegrees()) > patron_data->max_catch_angle) continue;

    // TODO: prefer projectiles that are slightly farther but with much more
    //       time to close the distance.
    *closest_position = closest_position_xy;
    closest_position->z() = patron_transform->position.z();
    *closest_time = closest_t;
    *closest_face_angle =
        Angle::FromYXVector(projectile_position_xy - closest_position_xy);
    closest_ref = &it->entity;
    closest_dist_sq = dist_sq;
  }
  return closest_ref;
}

void PatronComponent::FindProjectileAndCatch(const EntityRef& patron) {
  // Find the projectile that's the closest.
  vec3 closest_position;
  Angle closest_face_angle;
  float closest_time;
  const EntityRef* closest_projectile = ClosestProjectile(
      patron, &closest_position, &closest_face_angle, &closest_time);

  // Initialize movement to that spot.
  if (closest_projectile != nullptr) {
    MoveToTarget(patron, closest_position, closest_face_angle, closest_time);
    PatronData* patron_data = GetComponentData(patron);
    patron_data->catching_state = kCatchingStateMoveToTarget;
    // If this is on a rail, we want to disable it until we are done.
    RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
    if (rail_denizen_data != nullptr) {
      rail_denizen_data->enabled = false;
      rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
    }
  }
}

void PatronComponent::MoveToTarget(const EntityRef& patron,
                                   const vec3& target_position,
                                   Angle target_face_angle, float target_time) {
  const TransformData* patron_transform = Data<TransformData>(patron);
  PatronData* patron_data = Data<PatronData>(patron);
  const vec3 position = patron_transform->position;
  const Angle face_angle(patron_transform->orientation.ToEulerAngles().z());

  // If moving from idle, store the current position to return to later.
  if (patron_data->catching_state == kCatchingStateIdle) {
    patron_data->return_position = position;
  }

  // At `target_time` we want to achieve these deltas so that our position and
  // face angle with equal `target_position` and `target_face_angle`.
  const vec3 delta_position = target_position - position;
  const Angle delta_face_angle = target_face_angle - face_angle;
  const MotiveTime target_time_ms =
      static_cast<MotiveTime>(1000.0f * target_time);

  // Set the delta movement Motivators.
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
      motive::CurrentToTarget1f(0.0f, 0.0f, delta_face_angle.ToRadians(), 0.0f,
                                target_time_ms));
}

}  // fpl_project
}  // fpl
