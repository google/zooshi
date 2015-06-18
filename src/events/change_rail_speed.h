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

#ifndef FPL_EVENTS_CHANGE_RAIL_SPEED_EVENT_H_
#define FPL_EVENTS_CHANGE_RAIL_SPEED_EVENT_H_

#include "event/event_registry.h"
#include "events_generated.h"

namespace fpl {
namespace fpl_project {

struct ChangeRailSpeedPayload {
  ChangeRailSpeedPayload(entity::EntityRef entity_,
                         const ChangeRailSpeed* change_rail_speed_)
      : entity(entity_), change_rail_speed(change_rail_speed_) {}

  entity::EntityRef entity;
  const ChangeRailSpeed* change_rail_speed;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_PAYLOAD_ID(fpl::fpl_project::EventSinkUnion_ChangeRailSpeed,
                              fpl::fpl_project::ChangeRailSpeedPayload)

#endif  // FPL_EVENTS_CHANGE_RAIL_SPEED_EVENT_H_

