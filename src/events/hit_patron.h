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

#ifndef FPL_EVENTS_HIT_PATRON_EVENT_H_
#define FPL_EVENTS_HIT_PATRON_EVENT_H_

#include "event_system/event_registry.h"
#include "events/event_ids.h"
#include "events/entity_event.h"
#include "entity/entity_manager.h"

namespace fpl {
namespace fpl_project {

struct HitPatronEventPayload : public EntityEventPayload {
  HitPatronEventPayload(entity::EntityRef& target_,
                        entity::EntityRef& projectile_,
                        entity::EntityRef& patron_)
      : EntityEventPayload(target_), projectile(projectile_), patron(patron_) {}

  // The projectile that hit the patron.
  entity::EntityRef projectile;

  // The patron that was fed.
  entity::EntityRef patron;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_ID(fpl::fpl_project::kEventIdHitPatron,
                      fpl::fpl_project::HitPatronEventPayload)

#endif  // FPL_EVENTS_HIT_PATRON_EVENT_H_

