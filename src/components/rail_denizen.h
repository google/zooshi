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

class Rail {
 public:
  void Initialize(const RailDef* rail_def, float spline_granularity);

  /// Return vector of `positions` that is the rail evaluated every `delta_time`
  /// for the entire course of the rail. This calculation is much faster than
  /// calling PositionCalculatedSlowly() multiple times.
  void Positions(float delta_time,
                 std::vector<mathfu::vec3_packed>* positions) const;

  /// Return the rail position at `time`. This calculation is fairly slow
  /// so only use outside a loop. If you need a series of positions, consider
  /// calling Positions() above instead.
  mathfu::vec3 PositionCalculatedSlowly(float time) const;

  /// Length of the rail.
  float EndTime() const { return splines_[0].EndX(); }

  /// Internal structure representing the rails.
  const fpl::CompactSpline* splines() const { return splines_; }

 private:
  static const motive::MotiveDimension kDimensions = 3;

  fpl::CompactSpline splines_[kDimensions];
};

struct RailDenizenData {
  RailDenizenData()
      : lap(0),
        spline_playback_rate(1.0f),
        previous_time(0),
        on_new_lap(nullptr),
        motivator() {}

  void Initialize(const Rail& rail, float start_time,
                  motive::MotiveEngine* engine);

  mathfu::vec3 Position() const { return motivator.Value(); }
  mathfu::vec3 Velocity() const { return motivator.Velocity(); }

  int lap;

  float spline_playback_rate;

  motive::MotiveTime previous_time;

  const ActionDef* on_new_lap;

  motive::Motivator3f motivator;
};

class RailDenizenComponent : public entity::Component<RailDenizenData>,
                             public event::EventListener {
 public:
  RailDenizenComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  virtual void OnEvent(const event::EventPayload& event_payload);

  void SetRail(const RailDef* rail_def);

  // TODO - get rid of this once raildenizen is changed to have rail stored
  // in the component data.
  Rail* rail() { return &rail_; }

  entity::EntityRef& river_entity() { return river_entity_; }

 private:
  // A pointer to the MotiveEngine used to spawn motivators.
  motive::MotiveEngine* engine_;

  // The rail that will define the path to follow.
  Rail rail_;

  // The entity that moves along the river.
  entity::EntityRef river_entity_;

  event::EventManager* event_manager_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RailDenizenComponent,
                              fpl::fpl_project::RailDenizenData,
                              ComponentDataUnion_RailDenizenDef)

#endif  // COMPONENTS_RAIL_DENIZEN_H_
