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

#include <cstdio>

#include "components/score.h"
#include "event_system/event_manager.h"
#include "event_system/event_payload.h"
#include "events/event_ids.h"
#include "events/hit_patron.h"
#include "events/hit_patron_body.h"
#include "events/hit_patron_mouth.h"
#include "events/projectile_fired.h"
#include "fplbase/material_manager.h"
#include "fplbase/utilities.h"
#include "imgui/imgui.h"

namespace fpl {
namespace fpl_project {

void ScoreData::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.event_id()) {
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

void ScoreComponent::Initialize(InputSystem* input_system,
                                MaterialManager* material_manager,
                                FontManager* font_manager,
                                event::EventManager* event_manager) {
  input_system_ = input_system;
  font_manager_ = font_manager;
  material_manager_ = material_manager;
  event_manager->RegisterListener(kEventIdProjectileFired, this);
  event_manager->RegisterListener(kEventIdHitPatronMouth, this);
  event_manager->RegisterListener(kEventIdHitPatronBody, this);
  event_manager->RegisterListener(kEventIdHitPatron, this);
}

void ScoreComponent::OnEvent(const event::EventPayload& event_payload) {
  const EntityEvent* entity_event = nullptr;
  switch (event_payload.event_id()) {
    case kEventIdProjectileFired: {
      entity_event = event_payload.ToData<ProjectileFiredEvent>();
      break;
    }
    case kEventIdHitPatronMouth: {
      entity_event = event_payload.ToData<HitPatronMouthEvent>();
      break;
    }
    case kEventIdHitPatronBody: {
      entity_event = event_payload.ToData<HitPatronBodyEvent>();
      break;
    }
    case kEventIdHitPatron: {
      entity_event = event_payload.ToData<HitPatronEvent>();
      break;
    }
    default: { assert(0); }
  }
  if (entity_event) {
    ScoreData* score_data = Data<ScoreData>(entity_event->target);
    if (score_data) {
      score_data->OnEvent(event_payload);
    }
  }
}

void ScoreComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  auto component_data = &component_data_;
  gui::Run(*material_manager_, *font_manager_, *input_system_,
           [component_data]() {
             for (auto iter = component_data->begin();
                  iter != component_data->end(); ++iter) {
               ScoreData* score_data = &iter->data;
               char label[16] = {0};
               std::sprintf(label, "%i", score_data->patrons_fed);
               gui::StartGroup(gui::LAYOUT_HORIZONTAL_TOP, 10);
               gui::Margin margin(8);
               gui::SetMargin(margin);
               gui::Label(label, 100);
               gui::EndGroup();
             }
           });
}

void ScoreComponent::AddFromRawData(entity::EntityRef& entity,
                                    const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  (void)component_data;
  assert(component_data->data_type() == ComponentDataUnion_ScoreDef);
  AddEntity(entity);
}

}  // fpl_project
}  // fpl

