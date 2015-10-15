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
#include "breadboard/event.h"
#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/compact_spline.h"
#include "motive/motivator.h"
#include "railmanager.h"

namespace fpl {
namespace zooshi {

BREADBOARD_DECLARE_EVENT(kNewLapEventId)

struct RailDenizenData {
  RailDenizenData()
      : lap(0.0f),
        initial_playback_rate(0.0f),
        start_time(0.0f),
        motivator(),
        playback_rate(),
        rail_offset(mathfu::kZeros3f),
        rail_orientation(mathfu::quat::identity),
        rail_scale(mathfu::kOnes3f),
        internal_rail_offset(mathfu::kZeros3f),
        internal_rail_orientation(mathfu::kQuatIdentityf),
        internal_rail_scale(mathfu::kZeros3f),
        update_orientation(false),
        inherit_transform_data(false),
        enabled(true) {}

  void Initialize(const Rail& rail, motive::MotiveEngine& engine);

  // Set speed at which the entity traverses the rail.
  // playback_rate 0 ==> paused
  // playback_rate 0.5 ==> half speed of authored rail (slow)
  // playback_rate 1 ==> authored speed
  // playback_rate 2 ==> double speed of authored rail (fast)
  void SetPlaybackRate(float playback_rate, float transition_time);

  mathfu::vec3 Position() const { return motivator.Value(); }
  mathfu::vec3 Velocity() const { return motivator.Velocity(); }
  float PlaybackRate() const { return playback_rate.Value(); }

  float lap;
  float initial_playback_rate;
  float start_time;
  motive::Motivator3f motivator;
  motive::Motivator1f playback_rate;
  std::string rail_name;
  // Additional transform to apply from the rail to the entity being driven.
  mathfu::vec3 rail_offset;
  mathfu::quat rail_orientation;
  mathfu::vec3 rail_scale;
  // Additional transform that does not take into account any inherited
  // transform data. Should be used by editor and parsing data, while the above
  // ones should be used when driving motion.
  mathfu::vec3 internal_rail_offset;
  mathfu::quat internal_rail_orientation;
  mathfu::vec3 internal_rail_scale;
  bool update_orientation;
  bool inherit_transform_data;
  bool enabled;
};

class RailDenizenComponent : public entity::Component<RailDenizenData> {
 public:
  RailDenizenComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  void UpdateRailNodeData(entity::EntityRef entity);

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

 private:
  void InitializeRail(entity::EntityRef&);
  void OnEnterEditor();
};

}  // zooshi
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::zooshi::RailDenizenComponent,
                              fpl::zooshi::RailDenizenData)

#endif  // COMPONENTS_RAIL_DENIZEN_H_
