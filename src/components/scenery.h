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

#include "config_generated.h"
#include "entity/component.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace fpl_project {

enum SceneryState {
  kSceneryHide,       // Can be culled without visual glitches.
  kSceneryAppear,     // Transitioning from hide to show.
  kSceneryShow,       // Fully on-screen. Should not be culled.
  kSceneryDisappear,  // Transitioning from show to hide.
};

// Data for scene object components.
struct SceneryData {
  SceneryData() : state(kSceneryHide) {}

  // The child of the scenery entity that has a RenderMeshComponent and
  // an AnimationComponent.
  entity::EntityRef render_child;

  // Current state of the scenery. See NextState() for state machine.
  SceneryState state;

  // The type of scenery being animated. Each type has its own animations.
  AnimObject anim_object;
};

class SceneryComponent : public entity::Component<SceneryData> {
 public:
  SceneryComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& parent, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

 private:
  mathfu::vec3 CenterPosition() const;
  float PopInDistSq() const;
  float PopOutDistSq() const;
  float DistSq(const entity::EntityRef& scenery,
               const mathfu::vec3& center) const;
  float AnimTimeRemaining(const entity::EntityRef& scenery) const;
  bool HasAnim(const SceneryData* scenery_data, SceneryState state) const;
  SceneryState NextState(const entity::EntityRef& scenery,
                         const mathfu::vec3& center) const;
  void Animate(const entity::EntityRef& scenery, SceneryState state);
  void StopAnimating(const entity::EntityRef& scenery);
  void Show(const entity::EntityRef& scenery, bool show);
  void ShowAll(bool show);
  void TransitionState(const entity::EntityRef& scenery,
                       SceneryState next_state);

  const Config* config_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::SceneryComponent,
                              fpl::fpl_project::SceneryData)

#endif  // COMPONENTS_SCENERY_H_
