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

#ifndef FPL_EVENTS_PROJECTILE_FIRED_EVENT_H_
#define FPL_EVENTS_PROJECTILE_FIRED_EVENT_H_

#include "entity/entity_manager.h"
#include "event_system/event_registry.h"
#include "events/entity.h"
#include "events/event_ids.h"

namespace fpl {
namespace fpl_project {

struct ProjectileFiredEvent : public EntityEvent {
  ProjectileFiredEvent(entity::EntityRef& target_,
                       entity::EntityRef& projectile_)
      : EntityEvent(target_), projectile(projectile_) {}

  // The projectile that was fired;
  entity::EntityRef projectile;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_ID(fpl::fpl_project::kEventIdProjectileFired,
                      fpl::fpl_project::ProjectileFiredEvent)

#endif  // FPL_EVENTS_PROJECTILE_FIRED_EVENT_H_

