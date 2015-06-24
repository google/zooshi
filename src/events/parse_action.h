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

#ifndef FPL_EVENTS_PARSE_EVENT_H_
#define FPL_EVENTS_PARSE_EVENT_H_

#include "entity/entity_manager.h"
#include "events_generated.h"

namespace fpl {
namespace entity {
class EntityManager;
}  // entity
namespace event {
class EventManager;
}  // event
namespace fpl_project {

// This structure represents context specific variables used by ParseAction. Not
// all variables are used for all events, and the specific meaning of each
// variable depends on which event is being broadcast. More varaibles can be
// added as needed. This is generally intended to be used for values which are
// dynamic at runtime and thus cannot be directly referenced by static data.
struct EventContext {
  // Generally intended to be the entity that fired the event.
  entity::EntityRef source;

  // Generally intended to be the entity that owns or spawed the entity that
  // fired the event.
  entity::EntityRef source_owner;

  // If this is an event that resulted from the interaction of two entities,
  // this is the target entity, or the entity that the source entity interacted
  // with.
  entity::EntityRef target;

  // If this is an event that resulted from the interaction of two entities,
  // this is the owner of the target entity.
  entity::EntityRef target_owner;

  // The entity representing the raft.
  entity::EntityRef raft;
};

// Evaluate an ActionDef, which is a list of events, and fire each one
// filling in their data based on the EventContext.
void ParseAction(const ActionDef* event_def, EventContext* context,
                 event::EventManager* event_manager,
                 entity::EntityManager* entity_manager);

}  // fpl_project
}  // fpl

#endif  // FPL_EVENTS_PARSE_EVENT_H_

