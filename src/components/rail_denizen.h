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

#ifndef COMPONENTS_RAIL_DENIZEN_H_
#define COMPONENTS_RAIL_DENIZEN_H_

#include <map>
#include <limits>

#include "components_generated.h"
#include "rail_def_generated.h"
#include "entity/component.h"
#include "event_system/event_listener.h"
#include "mathfu/glsl_mappings.h"
#include "motive/motivator.h"
#include "motive/math/compact_spline.h"

namespace fpl {
namespace event {

class EventManager;

}  // event
namespace fpl_project {

struct Rail {
  static const motive::MotiveDimension kDimensions = 3;

  fpl::CompactSpline splines[kDimensions];

  void Initialize(const RailDef* rail_def, float spline_granularity);
};

struct RailDenizenData {
  void Initialize(const Rail& rail, float start_time,
                  motive::MotiveEngine* engine);

  mathfu::vec3 Position() const { return motivator.Value(); }
  mathfu::vec3 Velocity() const { return motivator.Velocity(); }

  float speed_coefficient;

  motive::Motivator3f motivator;
};

class RailDenizenComponent : public entity::Component<RailDenizenData>,
                             public event::EventListener {
 public:
  RailDenizenComponent() {}

  void Initialize(motive::MotiveEngine* engine, const RailDef* rail_def,
                  event::EventManager* event_manager);

  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  virtual void OnEvent(const event::EventPayload& event_payload);

 private:
  // A pointer to the MotiveEngine used to spawn motivators.
  motive::MotiveEngine* engine_;

  // The rail that will define the path to follow.
  Rail rail_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RailDenizenComponent,
                              fpl::fpl_project::RailDenizenData,
                              ComponentDataUnion_RailDenizenDef)

#endif  // COMPONENTS_RAIL_DENIZEN_H_
