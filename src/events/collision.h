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

#ifndef FPL_EVENTS_COLLISION_H_
#define FPL_EVENTS_COLLISION_H_

#include "event_system/event_registry.h"
#include "events_generated.h"
#include "mathfu/vector.h"

namespace fpl {
namespace fpl_project {

struct CollisionPayload {
  CollisionPayload(entity::EntityRef entity_a_, mathfu::vec3 position_a_,
                   entity::EntityRef entity_b_, mathfu::vec3 position_b_)
      : entity_a(entity_a_),
        position_a(position_a_),
        entity_b(entity_b_),
        position_b(position_b_) {}

  // The first entity in the collision.
  entity::EntityRef entity_a;
  // The position on the first entity the collision occurred.
  mathfu::vec3 position_a;
  // The second entity in the collision.
  entity::EntityRef entity_b;
  // The position on the second entity the collision occurred.
  mathfu::vec3 position_b;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_PAYLOAD_ID(fpl::fpl_project::EventSinkUnion_Collision,
                              fpl::fpl_project::CollisionPayload)

#endif  // FPL_EVENTS_COLLISION_H_
