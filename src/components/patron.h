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

#include "component_library/physics.h"
#include "components/graph.h"
#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "entity/entity_manager.h"
#include "event/event.h"
#include "event/graph.h"
#include "event/graph_state.h"
#include "events_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/angle.h"
#include "motive/motivator.h"

namespace fpl {
namespace fpl_project {

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
};

enum CatchingState {
  // The normal state, when the patron is not reaching to thrown sushi.
  kCatchingStateIdle,

  // Moving to where the patron believes it can intercept sushi.
  kCatchingStateMoveToTarget,

  // Returning to the position it left when it decided to move to the sushi.
  kCatchingStateReturn
};

// Data for scene object components.
struct PatronData {
  PatronData()
      : state(kPatronStateLayingDown),
        catching_state(kCatchingStateIdle),
        anim_object(AnimObject_HungryHippo),
        last_lap_fed(-1.0f) {}

  // Whether the patron is standing up or falling down.
  PatronState state;

  // Describes the behavior of the patron moving to catch sushi.
  CatchingState catching_state;

  // The type of patron being animated. Each patron has its own set of
  // animations.
  AnimObject anim_object;

  // Keep track of the last time this patron was fed so we know when they
  // can pop back up.
  float last_lap_fed;

  // If the raft entity is within the pop_in_range it will stand up. If it is
  // once up, if it's not in the pop out range, it will fall down. As a minor
  // optimization, it's stored here as the square of the distance.
  float pop_in_radius_squared;
  float pop_out_radius_squared;
  float pop_in_radius;
  float pop_out_radius;

  // Each time the raft makes a lap around the river, its lap counter is
  // incremented.  Patrons will only stand up when the lap counter is in the
  // range [min_lap, max_lap].
  float min_lap;
  float max_lap;

  // The tag of the body part that needs to be hit to trigger a fall.
  // Note that an empty name means any collision counts.
  std::string target_tag;

  // The index into physics data's `rigid_bodies` that corresponds to
  // `target_tag`. Cache here so we don't have to loop through all the rigid
  // bodies doing string compares.
  int target_rigid_body_index;

  // The child of the patron entity that has a RenderMeshComponent and
  // an AnimationComponent.
  entity::EntityRef render_child;

  // The maximum distance that the patron will move when trying to catch sushi.
  float max_catch_distance;

  // The maximum angle off of the vector to the raft that the patron will turn
  // to face when trying to catch sushi. Note that 180 fully encompasses it.
  float max_catch_angle;

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
  Angle prev_delta_face_angle;

  // The height above the patron at which to spawn the happy-indicator.
  float point_display_height;
};

class PatronComponent : public entity::Component<PatronData> {
 public:
  PatronComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& parent, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  void UpdateAndEnablePhysics();

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

  static void CollisionHandler(
      fpl::component_library::CollisionData* collision_data, void* user_data);

 private:
  void HandleCollision(const entity::EntityRef& patron_entity,
                       const entity::EntityRef& proj_entity,
                       const std::string& part_tag);
  void UpdateMovement(const entity::EntityRef& patron);
  void SpawnSplatter(const mathfu::vec3& position, int count);
  void SpawnPointDisplay(const entity::EntityRef& patron);
  bool HasAnim(const PatronData* patron_data, PatronAction action) const;
  void Animate(PatronData* patron_data, PatronAction action);
  Range TargetHeightRange(const entity::EntityRef& patron) const;
  bool RaftExists() const;
  mathfu::vec3 RaftPosition() const;
  const entity::EntityRef* ClosestProjectile(const entity::EntityRef& patron,
                                             mathfu::vec3* closest_position,
                                             Angle* closest_face_angle,
                                             float* closest_time) const;
  void FindProjectileAndCatch(const entity::EntityRef& patron);
  void MoveToTarget(const entity::EntityRef& patron,
                    const mathfu::vec3& target_position,
                    Angle target_face_angle, float target_time);

  const Config* config_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PatronComponent,
                              fpl::fpl_project::PatronData)

#endif  // COMPONENTS_PATRON_H_
