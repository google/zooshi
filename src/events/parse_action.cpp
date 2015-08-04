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

#include "component_library/transform.h"
#include "entity/entity_manager.h"
#include "event/event_manager.h"
#include "events/change_rail_speed.h"
#include "events/modify_attribute.h"
#include "events/parse_action.h"
#include "events/play_sound.h"
#include "events_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector_3.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;

static entity::EntityRef* GetEntity(EntityAffected entity_affected,
                                    EventContext* context) {
  switch (entity_affected) {
    case EntityAffected_Source: {
      return &context->source;
    }
    case EntityAffected_SourceOwner: {
      return &context->source_owner;
    }
    case EntityAffected_Target: {
      return &context->target;
    }
    case EntityAffected_TargetOwner: {
      return &context->target_owner;
    }
    case EntityAffected_Raft: {
      return &context->raft;
    }
    default: { assert(0); }
  }
  return nullptr;
}

void ParseAction(const ActionDef* event_def, EventContext* context,
                 event::EventManager* event_manager,
                 entity::EntityManager* entity_manager) {
  if (!event_def || !event_def->event_list()) return;
  for (size_t i = 0; i < event_def->event_list()->size(); i++) {
    const ActionDefInstance* instance = event_def->event_list()->Get(i);
    switch (instance->event_type()) {
      case EventSinkUnion_PlaySound: {
        auto* event = static_cast<const PlaySound*>(instance->event());
        entity::EntityRef* ent = GetEntity(event->entity(), context);
        mathfu::vec3 location =
            entity_manager->GetComponent<TransformComponent>()->WorldPosition(
                *ent);
        event_manager->BroadcastEvent(
            PlaySoundPayload(event->sound_name()->c_str(), location));
        break;
      }
      case EventSinkUnion_ModifyAttribute: {
        auto* event = static_cast<const ModifyAttribute*>(instance->event());
        entity::EntityRef* ent = GetEntity(event->entity(), context);
        event_manager->BroadcastEvent(ModifyAttributePayload(*ent, event));
        break;
      }
      case EventSinkUnion_ChangeRailSpeed: {
        auto* event = static_cast<const ChangeRailSpeed*>(instance->event());
        entity::EntityRef* ent = GetEntity(event->entity(), context);
        event_manager->BroadcastEvent(ChangeRailSpeedPayload(*ent, event));
        break;
      }
      default: { assert(0); }
    }
  }
}

}  // fpl_project
}  // fpl
