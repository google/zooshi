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

#include "components/score.h"
#include "event_system/event_manager.h"
#include "event_system/event_payload.h"
#include "events/event_ids.h"
#include "events/hit_patron.h"
#include "events/hit_patron_body.h"
#include "events/hit_patron_mouth.h"
#include "events/projectile_fired_event.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

void ScoreData::OnEvent(int event_id,
                        const event::EventPayload& event_payload) {
  (void)event_payload;
  switch (event_id) {
    case kEventIdProjectileFired: {
      LogInfo("Fired projectile");
      ++projectiles_fired;
      break;
    }
    case kEventIdHitPatronMouth: {
      LogInfo("Hit patron mouth");
      ++patrons_fed;
      break;
    }
    case kEventIdHitPatronBody: {
      LogInfo("Hit patron body");
      ++patrons_hit;
      break;
    }
    case kEventIdHitPatron: {
      LogInfo("Hit patron");
      // Do nothing here right now...
      break;
    }
    default: { assert(0); }
  }
}

void ScoreComponent::Initialize(event::EventManager* event_manager) {
  event_manager->RegisterListener(kEventIdProjectileFired, this);
  event_manager->RegisterListener(kEventIdHitPatronMouth, this);
  event_manager->RegisterListener(kEventIdHitPatronBody, this);
  event_manager->RegisterListener(kEventIdHitPatron, this);
}

void ScoreComponent::OnEvent(int event_id,
                             const event::EventPayload& event_payload) {
  const EntityEventPayload* entity_event;
  switch (event_id) {
    case kEventIdProjectileFired: {
      entity_event = event_payload.ToData<ProjectileFiredEventPayload>();
      break;
    }
    case kEventIdHitPatronMouth: {
      entity_event = event_payload.ToData<HitPatronMouthEventPayload>();
      break;
    }
    case kEventIdHitPatronBody: {
      entity_event = event_payload.ToData<HitPatronBodyEventPayload>();
      break;
    }
    case kEventIdHitPatron: {
      entity_event = event_payload.ToData<HitPatronEventPayload>();
      break;
    }
    default: { assert(0); }
  }
  if (entity_event) {
    ScoreData* score_data = Data<ScoreData>(entity_event->target);
    if (score_data) {
      score_data->OnEvent(event_id, event_payload);
    }
  }
}

void ScoreComponent::AddFromRawData(entity::EntityRef& entity,
                                    const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_ScoreDef);
  AddEntity(entity);
}

}  // fpl_project
}  // fpl

