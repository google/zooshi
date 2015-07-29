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

#include <string>
#include <vector>
#include "components_generated.h"
#include "entity/component.h"
#include "event/event_listener.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/motivator.h"
#include "motive/math/compact_spline.h"
#include "railmanager.h"

namespace fpl {
namespace event {

class EventManager;

}  // event
namespace fpl_project {

struct RailDenizenData {
  RailDenizenData()
      : lap(0),
        spline_playback_rate(1.0f),
        previous_time(0),
        on_new_lap(nullptr),
        motivator(),
        rail_offset(mathfu::kZeros3f),
        rail_orientation(mathfu::quat::identity),
        rail_scale(mathfu::kOnes3f),
        enabled(true) {}

  void Initialize(const Rail& rail, float start_time);

  mathfu::vec3 Position() const { return motivator.Value(); }
  mathfu::vec3 Velocity() const { return motivator.Velocity(); }

  int lap;
  float spline_playback_rate;
  float start_time;
  motive::MotiveTime previous_time;
  const ActionDef* on_new_lap;
  std::vector<unsigned char> on_new_lap_flatbuffer;
  motive::Motivator3f motivator;
  std::string rail_name;
  mathfu::vec3 rail_offset;
  mathfu::quat rail_orientation;
  mathfu::vec3 rail_scale;
  bool update_orientation;
  bool enabled;
};

class RailDenizenComponent : public entity::Component<RailDenizenData>,
                             public event::EventListener {
 public:
  RailDenizenComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  virtual void OnEvent(const event::EventPayload& event_payload);

 private:
  void InitializeRail(entity::EntityRef&);

  // A pointer to the MotiveEngine used to spawn motivators.
  motive::MotiveEngine* engine_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RailDenizenComponent,
                              fpl::fpl_project::RailDenizenData)

#endif  // COMPONENTS_RAIL_DENIZEN_H_
