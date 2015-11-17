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
      : lap_number(0),
        lap_progress(0.0f),
        total_lap_progress(0.0f),
        initial_playback_rate(0.0f),
        start_time(0.0f),
        motivator(),
        orientation_motivator(),
        playback_rate(),
        rail_offset(mathfu::kZeros3f),
        rail_orientation(mathfu::kQuatIdentityf),
        rail_scale(mathfu::kOnes3f),
        interpolated_orientation(mathfu::kQuatIdentityf),
        internal_rail_offset(mathfu::kZeros3f),
        internal_rail_orientation(mathfu::kQuatIdentityf),
        internal_rail_scale(mathfu::kZeros3f),
        orientation_convergence_rate(0.0f),
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
  mathfu::vec3 Direction() const { return motivator.Direction(); }
  float PlaybackRate() const { return playback_rate.Value(); }

  // The total number of laps completed so far.
  int lap_number;
  // The current progress in the current lap in the range of [0,1].
  float lap_progress;
  // The total distance traveled so far in laps (e.g. a value of 1.75 would mean
  // you've traveled 1 and three quarters laps). This is the sum of the
  // lap_number and the lap_progress.
  float total_lap_progress;
  float initial_playback_rate;
  float start_time;
  motive::Motivator3f motivator;
  // Look ahead used to calculate the interpolated orientation of the denizen.
  motive::Motivator3f orientation_motivator;
  motive::Motivator1f playback_rate;
  std::string rail_name;
  // Additional transform to apply from the rail to the entity being driven.
  mathfu::vec3 rail_offset;
  mathfu::quat rail_orientation;
  mathfu::vec3 rail_scale;
  // Interpolated orientation.
  mathfu::quat interpolated_orientation;

  // Additional transform that does not take into account any inherited
  // transform data. Should be used by editor and parsing data, while the above
  // ones should be used when driving motion.
  mathfu::vec3 internal_rail_offset;
  mathfu::quat internal_rail_orientation;
  mathfu::vec3 internal_rail_scale;
  float orientation_convergence_rate;
  bool update_orientation;
  bool inherit_transform_data;
  bool enabled;
};

class RailDenizenComponent : public corgi::Component<RailDenizenData> {
 public:
  RailDenizenComponent() {}

  virtual void Init();
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);
  virtual void InitEntity(corgi::EntityRef& entity);

  void UpdateRailNodeData(corgi::EntityRef entity);

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

 private:
  void InitializeRail(corgi::EntityRef&);
  void OnEnterEditor();
};

}  // zooshi
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::zooshi::RailDenizenComponent,
                              fpl::zooshi::RailDenizenData)

#endif  // COMPONENTS_RAIL_DENIZEN_H_
