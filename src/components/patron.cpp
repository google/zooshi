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
#include "components/attributes.h"
#include "components/player.h"
#include "components/player_projectile.h"
#include "components/services.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "mathfu/glsl_mappings.h"
#include "motive/anim.h"
#include "motive/anim_table.h"
#include "world.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::zooshi::PatronComponent,
                            fpl::zooshi::PatronData)

namespace fpl {
namespace zooshi {

using corgi::component_library::AnimationComponent;
using corgi::component_library::AnimationData;
using corgi::component_library::CollisionData;
using corgi::component_library::PhysicsComponent;
using corgi::component_library::PhysicsData;
using corgi::component_library::RenderMeshComponent;
using corgi::component_library::RenderMeshData;
using corgi::component_library::RigidBodyData;
using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;
using scene_lab::SceneLab;
using corgi::EntityRef;
using mathfu::kZeros3f;
using mathfu::quat;
using mathfu::vec3;
using motive::MotiveTime;
using motive::kMotiveTimeEndless;

// All of these numbers were picked for purely aesthetic reasons:
static const float kLapWaitAmount = 0.5f;
static const float kHeightRangeBuffer = 0.05f;

static inline vec3 ZeroHeight(const vec3& v) {
  vec3 v_copy = v;
  v_copy.z() = 0.0f;
  return v_copy;
}

static inline float Interpolate(const Interpolants& in, float time) {
  assert(in.times.Valid());
  if (in.times.Length() == 0.0f) {
    return time <= in.times.start() ? in.values.start() : in.values.end();
  }
  return in.values.Lerp(in.times.PercentClamped(time));
}

static inline bool IgnoredMoveState(PatronMoveState move_state) {
  return move_state != kPatronMoveStateMoveToTarget;
}

static inline void SetState(PatronState state, PatronData* patron_data) {
  patron_data->state = state;
  patron_data->time_in_state = 0.0f;
  patron_data->time_being_ignored = 0.0f;
}

static inline void SetMoveState(PatronMoveState move_state,
                                PatronData* patron_data) {
  patron_data->move_state = move_state;
  patron_data->time_in_move_state = 0.0f;
  if (!IgnoredMoveState(move_state)) {
    patron_data->time_being_ignored = 0.0f;
  }
}

void PatronComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  // Scene Lab is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a Scene Lab.
  SceneLab* scene_lab = services->scene_lab();
  if (scene_lab) {
    scene_lab->AddOnEnterEditorCallback([this]() { UpdateAndEnablePhysics(); });
    scene_lab->AddOnExitEditorCallback([this]() { PostLoadFixup(); });
  }
}

static Interpolants LoadInterpolants(const InterpolantsDef* def) {
  return def == nullptr
             ? Interpolants()
             : Interpolants(motive::Range(def->start_value(), def->end_value()),
                            motive::Range(def->start_time(), def->end_time()));
}

void PatronComponent::AddFromRawData(corgi::EntityRef& entity,
                                     const void* raw_data) {
  auto patron_def = static_cast<const PatronDef*>(raw_data);
  PatronData* patron_data = AddEntity(entity);
  patron_data->anim_object = patron_def->anim_object();

  patron_data->pop_in_radius = LoadInterpolants(patron_def->pop_in_radius());
  patron_data->pop_out_radius = patron_def->pop_out_radius();
  assert(patron_data->pop_out_radius >=
         patron_data->pop_in_radius.values.end());

  patron_data->min_lap = patron_def->min_lap();
  patron_data->max_lap = patron_def->max_lap();
  patron_data->patience = LoadInterpolants(patron_def->patience());

  if (patron_def->events()) {
    patron_data->events.resize(patron_def->events()->size());
    for (size_t i = 0; i < patron_def->events()->size(); ++i) {
      auto event = patron_def->events()->Get(i);
      patron_data->events[i] = PatronEvent(event->action(), event->time());
    }
  }

  if (patron_def->target_tag()) {
    patron_data->target_tag = patron_def->target_tag()->str();
  }

  patron_data->max_catch_distance = patron_def->max_catch_distance();
  patron_data->max_catch_distance_for_search =
      patron_def->max_catch_distance_for_search();
  patron_data->max_catch_angle = patron_def->max_catch_angle();
  patron_data->point_display_height = patron_def->point_display_height();
  patron_data->max_face_angle_away_from_raft =
      motive::Angle::FromDegrees(patron_def->max_face_angle_away_from_raft());
  patron_data->time_to_face_raft = patron_def->time_to_face_raft();
  patron_data->play_eating_animation = patron_def->play_eating_animation() != 0;

  patron_data->catch_time_for_search =
      motive::Range(patron_def->min_catch_time_for_search(),
                    patron_def->max_catch_time_for_search());
  patron_data->catch_time =
      motive::Range(patron_def->min_catch_time(), patron_def->max_catch_time());
  patron_data->catch_speed = motive::Range(patron_def->min_catch_speed(),
                                           patron_def->max_catch_speed());
  patron_data->time_between_catch_searches =
      patron_def->time_between_catch_searches();
  patron_data->return_time = patron_def->return_time();
  patron_data->rail_accelerate_time = patron_def->rail_accelerate_time();

  patron_data->time_exasperated_before_disappearing =
      patron_def->time_exasperated_before_disappearing();
  patron_data->exasperated_playback_rate =
      patron_def->exasperated_playback_rate();
}

static inline flatbuffers::Offset<InterpolantsDef> SaveInterpolants(
    flatbuffers::FlatBufferBuilder& fbb, const Interpolants& in) {
  return CreateInterpolantsDef(fbb, in.values.start(), in.times.start(),
                               in.values.end(), in.times.end());
}

corgi::ComponentInterface::RawDataUniquePtr PatronComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  const PatronData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  auto target_tag = fbb.CreateString(data->target_tag);

  auto patience_fb = SaveInterpolants(fbb, data->patience);
  auto pop_in_radius_fb = SaveInterpolants(fbb, data->pop_in_radius);

  // Get all the on_collision events
  PatronDefBuilder builder(fbb);
  builder.add_min_lap(data->min_lap);
  builder.add_max_lap(data->max_lap);
  builder.add_patience(patience_fb);
  if (data->events.size() > 0) {
    std::vector<flatbuffers::Offset<fpl::PatronEvent>> events_flat(
        data->events.size());
    for (size_t i = 0; i < data->events.size(); ++i) {
      const PatronEvent& event = data->events[i];
      events_flat[i] = CreatePatronEvent(fbb, event.action, event.time);
    }
    builder.add_events(fbb.CreateVector(events_flat));
  }
  builder.add_pop_in_radius(pop_in_radius_fb);
  builder.add_pop_out_radius(data->pop_out_radius);
  builder.add_target_tag(target_tag);
  builder.add_max_catch_distance(data->max_catch_distance);
  builder.add_max_catch_angle(data->max_catch_angle);
  builder.add_point_display_height(data->point_display_height);
  builder.add_max_face_angle_away_from_raft(
      data->max_face_angle_away_from_raft.ToDegrees());
  builder.add_time_to_face_raft(data->time_to_face_raft);
  builder.add_play_eating_animation(data->play_eating_animation);
  builder.add_max_catch_distance_for_search(
      data->max_catch_distance_for_search);
  builder.add_min_catch_time_for_search(data->catch_time_for_search.start());
  builder.add_max_catch_time_for_search(data->catch_time_for_search.end());
  builder.add_min_catch_time(data->catch_time.start());
  builder.add_max_catch_time(data->catch_time.end());
  builder.add_min_catch_speed(data->catch_speed.start());
  builder.add_max_catch_speed(data->catch_speed.end());
  builder.add_time_between_catch_searches(data->time_between_catch_searches);
  builder.add_return_time(data->return_time);
  builder.add_rail_accelerate_time(data->rail_accelerate_time);
  builder.add_time_exasperated_before_disappearing(
      data->time_exasperated_before_disappearing);
  builder.add_exasperated_playback_rate(
      data->exasperated_playback_rate);
  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void PatronComponent::InitEntity(corgi::EntityRef& entity) { (void)entity; }

void PatronComponent::UpdateAndEnablePhysics() {
  // Make the patrons stand up
  RenderMeshComponent* render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef patron = iter->entity;
    physics_component->UpdatePhysicsFromTransform(patron);
    physics_component->EnablePhysics(patron);

    render_mesh_component->SetVisibilityRecursively(patron, true);
  }
}

void PatronComponent::PostLoadFixup() {
  const TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();

  // Initialize each patron.
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef patron = iter->entity;
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
    SetState(kPatronStateLayingDown, patron_data);

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

// Return time until the patron's patience has expired.
// That is, returns (time patron is willing to be ignored) -
//                  (time patron has been ignored)
static inline float TimeUntilExasperated(
    const PatronData* patron_data, const RailDenizenData* raft_rail_denizen) {
  const float lap = raft_rail_denizen->total_lap_progress;
  const float patience = Interpolate(patron_data->patience, lap);
  return patience - patron_data->time_being_ignored;
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
      patron_data->delta_position.Invalidate();
      if (patron_data->move_state == kPatronMoveStateReturn ||
          patron_data->move_state == kPatronMoveFaceRaft) {
        SetMoveState(kPatronMoveStateIdle, patron_data);
        // Back to idle, so resume moving on rails, if it is on one.
        RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
        if (rail_denizen_data != nullptr &&
            patron_data->state == kPatronStateUpright) {
          rail_denizen_data->enabled = true;
          rail_denizen_data->SetPlaybackRate(
              rail_denizen_data->initial_playback_rate,
              corgi::kMillisecondsPerSecond *
                  patron_data->rail_accelerate_time);
        }
      }
    }
  }

  // Add on delta to face angle.
  if (patron_data->delta_face_angle.Valid()) {
    const motive::Angle delta_face_angle(patron_data->delta_face_angle.Value());
    const motive::Angle delta =
        delta_face_angle - patron_data->prev_delta_face_angle;
    transform_data->orientation =
        transform_data->orientation *
        quat::FromAngleAxis(delta.ToRadians(), mathfu::kAxisZ3f);
    patron_data->prev_delta_face_angle = delta_face_angle;

    if (patron_data->delta_face_angle.TargetTime() <= 0) {
      patron_data->delta_face_angle.Invalidate();
    }
  }

  if (patron_data->move_state == kPatronMoveStateMoveToTarget) {
    if (ShouldReturnToIdle(patron)) {
      const vec3 raft_position = RaftPosition();
      const motive::Angle return_angle = motive::Angle::FromYXVector(
          raft_position - patron_data->return_position);
      MoveToTarget(patron, patron_data->return_position, return_angle,
                   patron_data->return_time);
      SetMoveState(kPatronMoveStateReturn, patron_data);
    }
  }

  // Start moving them faster if right before they disappear.
  const corgi::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  const RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  const bool agitated =
      patron_data->state == kPatronStateUpright &&
      event_time_ < 0 &&
      TimeUntilExasperated(patron_data, raft_rail_denizen) <=
          patron_data->time_exasperated_before_disappearing;
  SetAnimPlaybackRate(patron_data,
                      agitated ? patron_data->exasperated_playback_rate : 1.0f);
}

void PatronComponent::FaceRaft(const corgi::EntityRef& patron) {
  const vec3 raft = RaftPosition();
  const TransformData* transform_data = Data<TransformData>(patron);
  const motive::Angle to_raft(
      motive::Angle::FromYXVector(raft - transform_data->position));
  const motive::Angle face(transform_data->orientation.ToEulerAngles().z());
  const motive::Angle error(to_raft - face);
  PatronData* patron_data = Data<PatronData>(patron);
  if (error.Abs() > patron_data->max_face_angle_away_from_raft) {
    MoveToTarget(patron, transform_data->position, to_raft,
                 patron_data->time_to_face_raft);
    SetMoveState(kPatronMoveFaceRaft, patron_data);
  }
}

bool PatronComponent::AnimationEnding(const PatronData* patron_data,
                                      corgi::WorldTime delta_time) const {
  const AnimationData* anim_data =
      Data<AnimationData>(patron_data->render_child);
  if (!anim_data->motivator.Valid()) return false;

  const MotiveTime anim_delta_time = static_cast<MotiveTime>(delta_time);
  const MotiveTime time_remaining = anim_data->motivator.TimeRemaining();
  const bool anim_ending = time_remaining < anim_delta_time;
  return anim_ending;
}

void PatronComponent::StartEvent(corgi::WorldTime event_start_time) {
  event_time_ = event_start_time;

  // Reset the event index for all patrons.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef patron = iter->entity;
    PatronData* patron_data = Data<PatronData>(patron);
    patron_data->event_index = 0;
  }
}

bool PatronComponent::ShouldAppear(
    const PatronData* patron_data, const TransformData* transform_data,
    const RailDenizenData* raft_rail_denizen) const {
  if (patron_data->state != kPatronStateLayingDown) return false;

  // Only appear once per lap.
  const float lap = raft_rail_denizen->total_lap_progress;
  if (lap < patron_data->last_lap_upright + kLapWaitAmount) return false;

  // Only appear within min/max lap regions.
  if (lap < patron_data->min_lap ||
      (lap > patron_data->max_lap && patron_data->max_lap >= 0))
    return false;

  // Determine the patron's distance from the raft.
  const vec3 raft_position = raft_rail_denizen->Position();
  const vec3 raft_to_patron = transform_data->position - raft_position;
  const float dist_from_raft = raft_to_patron.Length();
  const float pop_in_radius = Interpolate(patron_data->pop_in_radius, lap);
  return dist_from_raft <= pop_in_radius;
}

bool PatronComponent::ShouldDisappear(
    const PatronData* patron_data, const TransformData* transform_data,
    const RailDenizenData* raft_rail_denizen) const {
  if (patron_data->state != kPatronStateUpright) return false;

  // When we're in an event, make non-event patrons disappear.
  if (event_time_ >= 0) return true;

  // Start disappear animation such that the patron will be disappeared
  // when it reaches the pop_out_radius.
  const float disappear_time = AnimLength(patron_data, PatronAction_Fall);
  const vec3 raft_future_position =
      raft_rail_denizen->Position() +
      disappear_time * raft_rail_denizen->Velocity();
  float raft_distance =
      (transform_data->position - raft_future_position).Length();
  if (raft_distance > patron_data->pop_out_radius) return true;

  // Patron tolerance decreases as we progress around the laps.
  const float time_until_exasperated = TimeUntilExasperated(patron_data,
                                                            raft_rail_denizen);
  return time_until_exasperated <= 0.0f;
}

void PatronComponent::UpdateAllEntities(corgi::WorldTime delta_time) {
  corgi::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  if (!raft) return;
  const RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef patron = iter->entity;
    TransformData* transform_data = Data<TransformData>(patron);
    PatronData* patron_data = Data<PatronData>(patron);
    RenderMeshComponent* rm_component =
        entity_manager_->GetComponent<RenderMeshComponent>();
    PhysicsComponent* physics_component =
        entity_manager_->GetComponent<PhysicsComponent>();

    // Animate patrons in the event.
    const int num_events = static_cast<int>(patron_data->events.size());
    if (event_time_ >= 0 && num_events > 0) {
      const bool anim_ending = AnimationEnding(patron_data, delta_time);
      if (patron_data->event_index < num_events) {
        const PatronEvent& event =
            patron_data->events[patron_data->event_index];
        if ((event.time >= 0 && event.time <= event_time_) ||
            (event.time < 0 && anim_ending)) {
          // Start new animation.
          Animate(patron_data, event.action);
          patron_data->event_index++;
          SetState(kPatronStateInEvent, patron_data);
        }
      } else if (anim_ending) {
        // Disable event patron since we've played the last event.
        SetState(kPatronStateLayingDown, patron_data);
      }
    }

    const PatronState state = patron_data->state;
    rm_component->SetVisibilityRecursively(patron,
                                           state != kPatronStateLayingDown);
    if (num_events > 0) continue;

    // Remember the last idle position so we can return to later.
    if (patron_data->move_state == kPatronMoveStateIdle) {
      patron_data->return_position = transform_data->position;
    }

    // Move patron towards the target.
    UpdateMovement(patron);

    // Set the patron's movement target.
    if (state == kPatronStateUpright &&
        (patron_data->move_state != kPatronMoveStateMoveToTarget ||
         patron_data->time_in_move_state >
             patron_data->time_between_catch_searches)) {
      FindProjectileAndCatch(patron);
    }
    if ((state == kPatronStateUpright || state == kPatronStateGettingUp) &&
        patron_data->move_state == kPatronMoveStateIdle) {
      FaceRaft(patron);
    }

    if (ShouldAppear(patron_data, transform_data, raft_rail_denizen)) {
      SetState(kPatronStateGettingUp, patron_data);
      Animate(patron_data, PatronAction_GetUp);
      patron_data->last_lap_upright = raft_rail_denizen->total_lap_progress;

    } else if (ShouldDisappear(patron_data, transform_data,
                               raft_rail_denizen)) {
      SetState(kPatronStateFalling, patron_data);
      Animate(patron_data, PatronAction_Fall);

      auto physics_component =
          entity_manager_->GetComponent<PhysicsComponent>();
      physics_component->DisablePhysics(patron);
      auto rail_denizen_data = Data<RailDenizenData>(patron);
      if (rail_denizen_data != nullptr) {
        rail_denizen_data->enabled = false;
        rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
      }
    }

    // Transition to the next state if we're at the end of the current
    // animation.
    const bool anim_ending = AnimationEnding(patron_data, delta_time);
    if (anim_ending) {
      switch (patron_data->state) {
        case kPatronStateEating:
          SetState(kPatronStateSatisfied, patron_data);
          Animate(patron_data, PatronAction_Satisfied);
          break;

        case kPatronStateSatisfied:
          SetState(kPatronStateFalling, patron_data);
          Animate(patron_data, PatronAction_Fall);
          break;

        case kPatronStateFalling:
          // After the patron has finished their falling animation, turn off
          // the physics, as they are no longer in the world.
          physics_component->DisablePhysics(patron);
          SetState(kPatronStateLayingDown, patron_data);
          break;

        case kPatronStateGettingUp: {
          SetState(kPatronStateUpright, patron_data);
          physics_component->EnablePhysics(patron);
          auto rail_denizen_data = Data<RailDenizenData>(patron);
          if (rail_denizen_data != nullptr) {
            rail_denizen_data->enabled = true;
            rail_denizen_data->SetPlaybackRate(
                rail_denizen_data->initial_playback_rate,
                corgi::kMillisecondsPerSecond *
                    patron_data->rail_accelerate_time);
          }
        }  // fallthrough

        case kPatronStateUpright:
          Animate(patron_data, PatronAction_Idle);
          break;

        default:
          break;
      }
    }

    // Update timers.
    const float delta_seconds =
        static_cast<float>(delta_time) / corgi::kMillisecondsPerSecond;
    patron_data->time_in_move_state += delta_seconds;
    patron_data->time_in_state += delta_seconds;
    if (IgnoredMoveState(patron_data->move_state)) {
      patron_data->time_being_ignored += delta_seconds;
    }
  }
  if (event_time_ >= 0) {
    event_time_ += delta_time;
  }
}

bool PatronComponent::HasAnim(const PatronData* patron_data,
                              PatronAction action) const {
  return entity_manager_->GetComponent<AnimationComponent>()->HasAnim(
      patron_data->render_child, action);
}

float PatronComponent::AnimLength(const PatronData* patron_data,
                                  PatronAction action) const {
  return static_cast<float>(
      entity_manager_->GetComponent<AnimationComponent>()->AnimLength(
          patron_data->render_child, action));
}

void PatronComponent::SetAnimPlaybackRate(const PatronData* patron_data,
                                          float playback_rate) {
  AnimationData* anim = Data<AnimationData>(patron_data->render_child);
  if (anim->motivator.Valid()) {
    anim->motivator.SetPlaybackRate(playback_rate);
  }
}

void PatronComponent::Animate(PatronData* patron_data, PatronAction action) {
  entity_manager_->GetComponent<AnimationComponent>()->AnimateFromTable(
      patron_data->render_child, action);
}

void PatronComponent::CollisionHandler(CollisionData* collision_data,
                                       void* user_data) {
  PatronComponent* patron_component = static_cast<PatronComponent*>(user_data);
  corgi::ComponentId id = GetComponentId();
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

void PatronComponent::HandleCollision(const corgi::EntityRef& patron_entity,
                                      const corgi::EntityRef& proj_entity,
                                      const std::string& part_tag) {
  // We only care about collisions with projectiles that haven't been deleted.
  PlayerProjectileData* projectile_data =
      Data<PlayerProjectileData>(proj_entity);
  if (projectile_data == nullptr || proj_entity->marked_for_deletion()) {
    return;
  }
  corgi::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  PatronData* patron_data = Data<PatronData>(patron_entity);
  if (patron_data->state == kPatronStateUpright) {
    // If the target tag was hit, consider it being fed
    if (patron_data->target_tag == "" || patron_data->target_tag == part_tag) {
      SetState(patron_data->play_eating_animation ? kPatronStateEating
                                                  : kPatronStateSatisfied,
               patron_data);
      Animate(patron_data, patron_data->play_eating_animation
                               ? PatronAction_Eat
                               : PatronAction_Satisfied);
      patron_data->last_lap_fed = raft_rail_denizen->total_lap_progress;

      // Disable rail movement after they have been fed
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

void PatronComponent::SpawnPointDisplay(const corgi::EntityRef& patron) {
  // We need the raft, so we can orient towards it:
  if (!RaftExists()) return;

  // Spawn from prototype:
  corgi::EntityRef point_display =
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
    float target_t, const motive::Range& valid_times,
    const motive::Range& valid_heights, float start_height, float start_speed,
    float gravity) {
  // Let `h(t)` represent the height at time `t`.
  // Then,
  //   h(0) = start_height
  //   h'(0) = start_speed
  //   h''(t) = gravity
  // So,
  //   h(t) = 0.5*gravity*t^2 + start_speed*t + start_height

  // Create curves with roots at the min and max heights.
  const float half_gravity = 0.5f * gravity;
  const motive::QuadraticCurve below_max_curve(
      half_gravity, start_speed, start_height - valid_heights.end());
  const motive::QuadraticCurve above_min_curve(
      half_gravity, start_speed, start_height - valid_heights.start());

  // Find time ranges where those curves are in the valid range.
  motive::QuadraticCurve::RangeArray below_max_ranges;
  motive::QuadraticCurve::RangeArray above_min_ranges;
  below_max_curve.RangesBelowZero(valid_times, &below_max_ranges);
  above_min_curve.RangesAboveZero(valid_times, &above_min_ranges);

  // Fine time ranges where both curves are in the valid range.
  motive::Range::RangeArray<4> valid_ranges;
  motive::Range::IntersectRanges(below_max_ranges, above_min_ranges,
                                 &valid_ranges);

  // TODO: Prefer later times, given the same distance, since we want more
  //       time to move to the catch point.
  const float closest_t = motive::Range::ClampToClosest(target_t, valid_ranges);
  return closest_t;
}

motive::Range PatronComponent::TargetHeightRange(const EntityRef& patron)
    const {
  // Get the physics rigid body data for the patron's target.
  const PatronData* patron_data = GetComponentData(patron);
  const PhysicsData* physics_data = Data<PhysicsData>(patron);

  // Get the axis-aligned bounding box of the patron's target.
  vec3 target_min;
  vec3 target_max;
  physics_data->GetAabb(patron_data->target_rigid_body_index, &target_min,
                        &target_max);

  // Return the heights.
  return motive::Range(target_min.z() + kHeightRangeBuffer,
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

const EntityRef* PatronComponent::ClosestProjectile(
    const EntityRef& patron, vec3* closest_position,
    motive::Angle* closest_face_angle, float* closest_time) const {
  // TODO: change projectile_component to const when Component gets a
  //       const_iterator.
  PlayerProjectileComponent* projectile_component =
      entity_manager_->GetComponent<PlayerProjectileComponent>();
  const TransformData* patron_transform = Data<TransformData>(patron);
  const PatronData* patron_data = GetComponentData(patron);

  // Gather patron details. These are independent of the projectiles.
  const vec3 patron_position_xy = ZeroHeight(patron_transform->position);
  const motive::Range target_height_range = TargetHeightRange(patron);
  const vec3 return_position_xy = ZeroHeight(patron_data->return_position);

  // Gather data about the raft, which is needed in the calculations.
  const vec3 raft_position_xy = ZeroHeight(RaftPosition());

  // Loop through every projectile. Keep a reference to the closest one.
  const EntityRef* closest_ref = nullptr;
  float max_dist_sq = patron_data->max_catch_distance_for_search *
                      patron_data->max_catch_distance_for_search;
  float closest_dist_sq = max_dist_sq;
  vec3 closest_position_xy = mathfu::kZeros3f;
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
    if (patron_data->move_state == kPatronMoveStateReturn) {
      const float dist_sq = (return_position_xy -
                             closest_position_ignore_height_xy).LengthSquared();
      if (dist_sq > max_dist_sq) continue;
    }

    // Get the closest time at a catchable height.
    const float closest_t = CalculateClosestTimeInHeightRange(
        closest_t_ignore_height, patron_data->catch_time_for_search,
        target_height_range, projectile_position.z(), projectile_velocity.z(),
        config_->gravity());
    if (!patron_data->catch_time_for_search.Contains(closest_t)) continue;

    // Calculate the projectile position at `closest_t`.
    const vec3 intercept_position_xy =
        projectile_position_xy + projectile_velocity_xy * closest_t;
    const float dist_sq =
        (patron_position_xy - intercept_position_xy).LengthSquared();
    if (dist_sq > closest_dist_sq) continue;

    // Don't face too far away from the player in order to catch thrown sushi.
    motive::Angle angle_to_sushi = motive::Angle::FromYXVector(
        projectile_position_xy - intercept_position_xy);
    motive::Angle angle_to_raft =
        motive::Angle::FromYXVector(raft_position_xy - intercept_position_xy);
    motive::Angle difference = angle_to_raft - angle_to_sushi;
    if (fabs(difference.ToDegrees()) > patron_data->max_catch_angle) continue;

    // TODO: prefer projectiles that are slightly farther but with much more
    //       time to close the distance.
    closest_position_xy = intercept_position_xy;
    *closest_time = closest_t;
    *closest_face_angle = motive::Angle::FromYXVector(projectile_position_xy -
                                                      intercept_position_xy);
    closest_ref = &it->entity;
    closest_dist_sq = dist_sq;
  }

  // Clamp the movement time so the patron doesn't move to quickly or slowly.
  if (closest_ref != nullptr) {
    const float clamped_dist =
        std::min(std::sqrt(closest_dist_sq), patron_data->max_catch_distance);
    const float avg_speed = clamped_dist / *closest_time;
    const float clamped_speed = patron_data->catch_speed.Clamp(avg_speed);
    *closest_time = patron_data->catch_time.Clamp(clamped_dist / clamped_speed);

    // Ensure the returned `closest_position` is not farther than
    // max_catch_distance from the last idle position.
    const vec3 direction =
        (closest_position_xy - return_position_xy).Normalized();
    *closest_position = return_position_xy + clamped_dist * direction;
    closest_position->z() = patron_transform->position.z();
  }
  return closest_ref;
}

void PatronComponent::FindProjectileAndCatch(const EntityRef& patron) {
  // Find the projectile that's the closest.
  vec3 closest_position;
  motive::Angle closest_face_angle;
  float closest_time;
  const EntityRef* closest_projectile = ClosestProjectile(
      patron, &closest_position, &closest_face_angle, &closest_time);

  // Initialize movement to that spot.
  if (closest_projectile != nullptr) {
    PatronData* patron_data = GetComponentData(patron);
    // If already moving to catch that projectile, do nothing.
    if (*closest_projectile == patron_data->catch_sushi) {
      return;
    }
    MoveToTarget(patron, closest_position, closest_face_angle, closest_time);
    patron_data->catch_sushi = *closest_projectile;
    SetMoveState(kPatronMoveStateMoveToTarget, patron_data);

    // If patron is on a rail, we want to disable it until we are done.
    RailDenizenData* rail_denizen_data = Data<RailDenizenData>(patron);
    if (rail_denizen_data != nullptr) {
      rail_denizen_data->enabled = false;
      rail_denizen_data->motivator.SetSplinePlaybackRate(0.0f);
    }
  }
}

void PatronComponent::MoveToTarget(const EntityRef& patron,
                                   const vec3& target_position,
                                   motive::Angle target_face_angle,
                                   float target_time) {
  const TransformData* patron_transform = Data<TransformData>(patron);
  PatronData* patron_data = Data<PatronData>(patron);
  const vec3 position = patron_transform->position;
  const motive::Angle face_angle(
      patron_transform->orientation.ToEulerAngles().z());

  // At `target_time` we want to achieve these deltas so that our position and
  // face angle with equal `target_position` and `target_face_angle`.
  const vec3 delta_position = target_position - position;
  const motive::Angle delta_face_angle = target_face_angle - face_angle;
  const MotiveTime target_time_ms = std::max(
      1, static_cast<MotiveTime>(corgi::kMillisecondsPerSecond * target_time));

  // Set the delta movement Motivators.
  patron_data->prev_delta_position = vec3(kZeros3f);
  patron_data->prev_delta_face_angle = motive::Angle(0.0f);

  motive::MotiveEngine* motive_engine =
      &entity_manager_->GetComponent<AnimationComponent>()->engine();

  patron_data->delta_position.InitializeWithTarget(
      motive::SmoothInit(), motive_engine,
      motive::Tar3f::CurrentToTarget(kZeros3f, kZeros3f, delta_position,
                                     kZeros3f, target_time_ms));

  const motive::Range angle_range(-motive::kPi, motive::kPi);
  patron_data->delta_face_angle.InitializeWithTarget(
      motive::SmoothInit(angle_range, true), motive_engine,
      motive::CurrentToTarget1f(0.0f, 0.0f, delta_face_angle.ToRadians(), 0.0f,
                                target_time_ms));
}

bool PatronComponent::ShouldReturnToIdle(const corgi::EntityRef& patron) const {
  const PatronData* patron_data = Data<PatronData>(patron);
  const TransformData* transform_data = Data<TransformData>(patron);
  if (patron_data->move_state == kPatronMoveStateMoveToTarget &&
      patron_data->catch_sushi.IsValid()) {
    const PhysicsData* sushi_physics =
        Data<PhysicsData>(patron_data->catch_sushi);
    const TransformData* sushi_transform =
        Data<TransformData>(patron_data->catch_sushi);
    if (sushi_physics != nullptr && sushi_transform != nullptr) {
      vec3 sushi_to_patron =
          transform_data->position - sushi_transform->position;
      float dot = vec3::DotProduct(ZeroHeight(sushi_physics->Velocity()),
                                   ZeroHeight(sushi_to_patron.Normalized()));
      // If the sushi is moving towards the patron, we want the patron to stay.
      if (dot >= 0.0f) {
        return false;
      }
    }
  }
  // If all the checks fail, assume something is wrong, and return to idle.
  return true;
}

}  // zooshi
}  // fpl
