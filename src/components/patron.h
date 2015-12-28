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

#ifndef COMPONENTS_PATRON_H_
#define COMPONENTS_PATRON_H_

#include "breadboard/event.h"
#include "breadboard/graph.h"
#include "breadboard/graph_state.h"
#include "components/rail_denizen.h"
#include "components_generated.h"
#include "config_generated.h"
#include "corgi/component.h"
#include "corgi/entity_manager.h"
#include "corgi_component_library/graph.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/transform.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/angle.h"
#include "motive/math/range.h"
#include "motive/motivator.h"

namespace fpl {
namespace zooshi {

enum PatronState {
  // Laying down in wait for the raft to come in range. If this patron has been
  // fed this lap, it will not stand up again until the next lap.
  kPatronStateLayingDown,

  // Standing up, ready to be hit by the player.
  kPatronStateUpright,

  // The patron has just been fed, and is enjoying the sushi.
  kPatronStateEating,

  // The patron has finished eating the sushi, and is hanging out being happy.
  kPatronStateSatisfied,

  // The is falling down after being fed, or after going out of range.
  kPatronStateFalling,

  // The patron is within range of the raft, and is standing up.
  kPatronStateGettingUp,

  // The patron is executing a series of PatronEvents.
  kPatronStateInEvent,
};

enum PatronMoveState {
  // The normal state, when the patron is not reaching to thrown sushi.
  kPatronMoveStateIdle,

  // Moving to where the patron believes it can intercept sushi.
  kPatronMoveStateMoveToTarget,

  // Returning to the position it left when it decided to move to the sushi.
  kPatronMoveStateReturn,

  // Turning to face the raft.
  kPatronMoveFaceRaft
};

// An animation that gets triggered sometime after StartEvent() is called.
struct PatronEvent {
  // Animation to play.
  PatronAction action;

  // Time to start playing animation. If -1, start playing immediately after
  // previous animation completes.
  corgi::WorldTime time;

  PatronEvent() : action(PatronAction_GetUp), time(-1) {}
  PatronEvent(PatronAction action, corgi::WorldTime time)
      : action(action), time(time) {}
};

struct Interpolants {
  Interpolants() {}
  Interpolants(const motive::Range& values, const motive::Range& times)
      : values(values), times(times) {
    // Assert times.begin() <= times.end(). Not necessary to check values.
    assert(times.Valid());
  }

  motive::Range values;
  motive::Range times;
};

// Data for scene object components.
struct PatronData {
  PatronData()
      : state(kPatronStateLayingDown),
        move_state(kPatronMoveStateIdle),
        anim_object(AnimObject_HungryHippo),
        last_lap_upright(-1.0f),
        last_lap_fed(-1.0f),
        pop_out_radius(0.0f),
        min_lap(0.0f),
        max_lap(0.0f),
        event_index(0),
        target_rigid_body_index(0),
        prev_delta_position(mathfu::kZeros3f),
        return_position(mathfu::kZeros3f),
        point_display_height(0.0f),
        time_in_state(0.0f),
        time_in_move_state(0.0f),
        time_being_ignored(0.0f),
        max_catch_distance(0.0f),
        max_catch_distance_for_search(0.0f),
        max_catch_angle(0.0f),
        time_between_catch_searches(0.0f),
        return_time(0.0f),
        rail_accelerate_time(0.0f),
        time_to_face_raft(0.0f),
        time_exasperated_before_disappearing(1.0f),
        exasperated_playback_rate(2.0f) {}

  // Whether the patron is standing up or falling down.
  PatronState state;

  // Describes the behavior of the patron moving to catch sushi.
  PatronMoveState move_state;

  // The type of patron being animated. Each patron has its own set of
  // animations.
  AnimObject anim_object;

  // Keep track of the last time this patron was fed so we know when they
  // can pop back up.
  float last_lap_upright;
  float last_lap_fed;

  // If the raft entity is within the pop_in_range it will stand up. If it is
  // once up, if it's not in the pop out range, it will fall down. As a minor
  // optimization, it's stored here as the square of the distance.
  Interpolants pop_in_radius;
  float pop_out_radius;

  // Each time the raft makes a lap around the river, its lap counter is
  // incremented. Patrons will only stand up when the lap counter is in the
  // range [min_lap, max_lap]. A negative value indicates no limits.
  float min_lap;
  float max_lap;

  // Time, in seconds, that patron is willing to wait until sushi is throw
  // within catching distance. Decreases as the lap increases.
  Interpolants patience;

  // Sequence of animations to follow once StartEvent() has been called.
  std::vector<PatronEvent> events;

  // Current index into the `events` array.
  int event_index;

  // The tag of the body part that needs to be hit to trigger a fall.
  // Note that an empty name means any collision counts.
  std::string target_tag;

  // The index into physics data's `rigid_bodies` that corresponds to
  // `target_tag`. Cache here so we don't have to loop through all the rigid
  // bodies doing string compares.
  int target_rigid_body_index;

  // The child of the patron entity that has a RenderMeshComponent and
  // an AnimationComponent.
  corgi::EntityRef render_child;

  // Position to add onto the patron's trajectory.
  // Amount added on = delta_position.Value() - prev_delta_position
  motive::Motivator3f delta_position;

  // Set to delta_position.Value() after movement is updated.
  mathfu::vec3_packed prev_delta_position;

  // The position that the patron left when going to catch the sushi.
  mathfu::vec3 return_position;

  // Face angle to add onto patron's trajectory.
  // Face angle is rotation about z-axis, with y-axis = 0, x-axis = 90 degrees
  // Units are radians.
  // Amount added on = delta_face_angle.Value() - prev_delta_face_angle
  motive::Motivator1f delta_face_angle;

  // Set to delta_face_angle.Value() after movement is updated.
  motive::Angle prev_delta_face_angle;

  // The height above the patron at which to spawn the happy-indicator.
  float point_display_height;

  // The time since `state` was changed, in seconds.
  float time_in_state;

  // The time since `move_state` was changed, in seconds.
  float time_in_move_state;

  // The time since the patron was moving towards a piece of sushi.
  float time_being_ignored;

  // The maximum distance that the patron will move when trying to catch sushi.
  float max_catch_distance;
  float max_catch_distance_for_search;

  // The maximum angle off of the vector to the raft that the patron will turn
  // to face when trying to catch sushi. Note that 180 fully encompasses it.
  float max_catch_angle;

  // When looking at sushi to catch, consider the sushi's trajectory over
  // this time range. In seconds.
  motive::Range catch_time_for_search;

  // When actually moving to catch a sushi, make sure you get to the target
  // position within this time. Ensures we don't react too quickly or slowly.
  // Better to smoothly move to a close position in 200ms than to jerk to it
  // in 10ms (we'll probably catch the sushi either way since our hit region
  // is large). Likewise, better to move to the target position in 2.5s and
  // wait, than to take 5s to move there and look like we're not responding.
  // In seconds.
  motive::Range catch_time;

  // Average speed at which to travel towards the sushi catch position.
  motive::Range catch_speed;

  // The sushi entity trying to be caught.
  corgi::EntityRef catch_sushi;

  // When moving towards a sushi, wait this amount of time before adjusting
  // the search for another sushi.
  float time_between_catch_searches;

  // Time to return to the last idle position, after we're done trying to
  // catch sushi.
  float return_time;

  // Time to accelerate onto the rails, if the patron is traveling along rails.
  float rail_accelerate_time;

  // The angle beyond which the patron should turn to face the raft, in degrees.
  // This is the maximum we allow the patron to face away from the raft.
  motive::Angle max_face_angle_away_from_raft;

  // The time to take turning to face the raft, in seconds.
  float time_to_face_raft;

  // After being ignored for a while, the patron will get exasperated and idle
  // at a faster playback rate. If still no sushi is thrown at the patron,
  // the patron will disappear. This is the amount of time before disappearing
  // that the patron will be exasperated. Time in seconds.
  float time_exasperated_before_disappearing;

  // When exasperated, we play the idle animation faster. Therefore, this
  // number should be > 1.
  float exasperated_playback_rate;

  // If true: when fed play eat, satisfied, disappear animations.
  // If false: when fed play satisfied, disappear animations.
  bool play_eating_animation;
};

class PatronComponent : public corgi::Component<PatronData> {
 public:
  PatronComponent() : config_(nullptr), event_time_(-1) {}
  virtual ~PatronComponent() {}

  virtual void Init();
  virtual void AddFromRawData(corgi::EntityRef& parent, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
  virtual void InitEntity(corgi::EntityRef& entity);
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);

  void UpdateAndEnablePhysics();

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

  // Each patron (optionally) holds a sequence of animations in
  // `PatronData::events`. These events are followed after StartEvent() is
  // called.
  void StartEvent(corgi::WorldTime event_start_time);

  // Stop playback of event timeline, and resume normal operation.
  void StopEvent() { event_time_ = -1; }

  // Current time into the event. Starts from the `start_time` passed into
  // StartEvent().
  corgi::WorldTime event_time() const { return event_time_; }

  static void CollisionHandler(
      corgi::component_library::CollisionData* collision_data, void* user_data);

 private:
  void HandleCollision(const corgi::EntityRef& patron_entity,
                       const corgi::EntityRef& proj_entity,
                       const std::string& part_tag);
  void UpdateMovement(const corgi::EntityRef& patron);
  void SpawnPointDisplay(const corgi::EntityRef& patron);
  bool ShouldAppear(
      const PatronData* patron_data,
      const corgi::component_library::TransformData* transform_data,
      const RailDenizenData* raft_rail_denizen) const;
  bool ShouldDisappear(
      const PatronData* patron_data,
      const corgi::component_library::TransformData* transform_data,
      const RailDenizenData* raft_rail_denizen) const;
  bool AnimationEnding(const PatronData* patron_data,
                       corgi::WorldTime delta_time) const;
  bool HasAnim(const PatronData* patron_data, PatronAction action) const;
  float AnimLength(const PatronData* patron_data, PatronAction action) const;
  void SetAnimPlaybackRate(const PatronData* patron_data, float playback_rate);
  void Animate(PatronData* patron_data, PatronAction action);
  motive::Range TargetHeightRange(const corgi::EntityRef& patron) const;
  bool RaftExists() const;
  mathfu::vec3 RaftPosition() const;
  const corgi::EntityRef* ClosestProjectile(const corgi::EntityRef& patron,
                                            mathfu::vec3* closest_position,
                                            motive::Angle* closest_face_angle,
                                            float* closest_time) const;
  void FindProjectileAndCatch(const corgi::EntityRef& patron);
  void MoveToTarget(const corgi::EntityRef& patron,
                    const mathfu::vec3& target_position,
                    motive::Angle target_face_angle, float target_time);
  bool ShouldReturnToIdle(const corgi::EntityRef& patron) const;
  void FaceRaft(const corgi::EntityRef& patron);
  motive::Angle ReturnAngle(const corgi::EntityRef& patron) const;

  const Config* config_;

  // Current time into the "event". i.e. the set-up sequence of animations.
  corgi::WorldTime event_time_;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::PatronComponent, fpl::zooshi::PatronData)

#endif  // COMPONENTS_PATRON_H_
