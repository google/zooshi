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

#ifndef COMPONENTS_SCENERY_H_
#define COMPONENTS_SCENERY_H_

#include "components/rail_denizen.h"
#include "config_generated.h"
#include "corgi/component.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/angle.h"
#include "motive/math/range.h"
#include "motive/motivator.h"

namespace fpl {
namespace zooshi {

enum SceneryState {
  kSceneryInvalid = -1,  // An invalid scenery state.
  kSceneryHide,          // Can be culled without visual glitches.
  kSceneryAppear,        // Transitioning from hide to show.
  kSceneryShow,          // Fully on-screen. Should not be culled.
  kSceneryDisappear,     // Transitioning from show to hide.
};

enum SceneryMoveState {
  kSceneryMoveStateStatic,  // Should not move.
  kSceneryMoveFaceRaft,     // Turn to face raft.
};

// Data for scene object components.
struct SceneryData {
  SceneryData()
    : state(kSceneryHide),
      move_state(kSceneryMoveStateStatic),
      show_override(kSceneryInvalid) {}

  // The child of the scenery entity that has a RenderMeshComponent and
  // an AnimationComponent.
  corgi::EntityRef render_child;

  // Current state of the scenery. See NextState() for state machine.
  SceneryState state;

  // Move behavior for scenery as raft moves.
  SceneryMoveState move_state;

  // Angle to add onto scenery's trajectory.
  motive::Motivator1f delta_face_angle;

  // Previous delta_angle.Value().
  motive::Angle prev_delta_face_angle;

  // The type of scenery being animated. Each type has its own animations.
  AnimObject anim_object;

  // A Scenery object can specify an override animation that will play when in
  // the show state. The scenery override is reset when the scenery object
  // disappears.
  SceneryState show_override;
};

class SceneryComponent : public corgi::Component<SceneryData> {
 public:
  SceneryComponent() {}
  virtual ~SceneryComponent() {}

  virtual void Init();
  virtual void AddFromRawData(corgi::EntityRef& parent, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
  virtual void InitEntity(corgi::EntityRef& entity);
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

  // Apply an override animation to an entity that only applies in the `Show`
  // state.
  void ApplyShowOverride(const corgi::EntityRef& scenery,
                         SceneryState show_override);

 private:
  const RailDenizenData& Raft() const;
  float PopInDistSq() const;
  float PopOutDistSq() const;
  float DistSq(const corgi::EntityRef& scenery,
               const RailDenizenData& raft) const;
  float AnimTimeRemaining(const corgi::EntityRef& scenery) const;
  bool HasAnim(const SceneryData* scenery_data, SceneryState state) const;
  float AnimLength(const SceneryData* scenery_data, SceneryState state) const;
  SceneryState NextState(const corgi::EntityRef& scenery,
                         const RailDenizenData& raft) const;
  void Animate(const corgi::EntityRef& scenery, SceneryState state);
  void StopAnimating(const corgi::EntityRef& scenery);
  void Show(const corgi::EntityRef& scenery, bool show);
  void ShowAll(bool show);
  void TransitionState(const corgi::EntityRef& scenery,
                       SceneryState next_state);
  void AnimateScenery(const corgi::EntityRef& scenery,
                      SceneryData* scenery_data, SceneryState state);
  void SetVisibilityOnOtherChildren(const corgi::EntityRef& scenery,
                                    bool visible);
  void FaceRaft(const corgi::EntityRef& scenery);
  void UpdateMovement(const corgi::EntityRef& scenery);

  const Config* config_;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::SceneryComponent,
                         fpl::zooshi::SceneryData)

#endif  // COMPONENTS_SCENERY_H_
