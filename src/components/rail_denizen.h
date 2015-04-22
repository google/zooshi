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
#include "entity/component.h"
#include "mathfu/glsl_mappings.h"
#include "motive/motivator.h"
#include "motive/math/compact_spline.h"

namespace fpl {

// TODO(amablue): RailDenizens should have a reference to the track rather than
// a complete copy of it. b/20553810
struct RailDenizenData {
  void Initialize(const motive::MotivatorInit& x_init,
                  const motive::MotivatorInit& y_init,
                  const motive::MotivatorInit& z_init,
                  motive::MotiveEngine* engine);

  mathfu::vec3 Position() const;
  mathfu::vec3 Velocity() const;

  fpl::CompactSpline x_spline;
  fpl::CompactSpline y_spline;
  fpl::CompactSpline z_spline;

  motive::Motivator1f x_motivator;
  motive::Motivator1f y_motivator;
  motive::Motivator1f z_motivator;
};

class RailDenizenComponent : public entity::Component<RailDenizenData> {
 public:
  RailDenizenComponent(motive::MotiveEngine* engine) : engine_(engine) {}

  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

 private:
  // A pointer to the MotiveEngine used to spawn motivators.
  motive::MotiveEngine* engine_;
};

}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::RailDenizenComponent, fpl::RailDenizenData,
                              fpl::ComponentDataUnion_RailDenizenDef)

#endif  // COMPONENTS_RAIL_DENIZEN_H_

