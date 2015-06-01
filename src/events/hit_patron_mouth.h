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

#ifndef FPL_EVENTS_HIT_PATRON_MOUTH_EVENT_H_
#define FPL_EVENTS_HIT_PATRON_MOUTH_EVENT_H_

#include "entity/entity_manager.h"
#include "event_system/event_registry.h"
#include "events/entity.h"
#include "events/event_ids.h"

namespace fpl {
namespace fpl_project {

struct HitPatronMouthEvent : public EntityEvent {
  HitPatronMouthEvent(entity::EntityRef& target_,
                      entity::EntityRef& projectile_,
                      entity::EntityRef& patron_)
      : EntityEvent(target_), projectile(projectile_), patron(patron_) {}

  // The projectile that fed the patron.
  entity::EntityRef projectile;

  // The patron that was fed.
  entity::EntityRef patron;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_ID(fpl::fpl_project::kEventIdHitPatronMouth,
                      fpl::fpl_project::HitPatronMouthEvent)

#endif  // FPL_EVENTS_HIT_PATRON_MOUTH_EVENT_H_

