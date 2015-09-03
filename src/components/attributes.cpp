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

#include "components/attributes.h"

#include "components/graph.h"
#include "components/services.h"
#include "event/event_manager.h"
#include "event/event_payload.h"
#include "events/modify_attribute.h"
#include "events/utilities.h"
#include "event/event_manager.h"
#include "event/event.h"
#include "events_generated.h"
#include "fplbase/asset_manager.h"
#include "fplbase/utilities.h"
#include "flatui/flatui.h"
#include "modules/event_ids.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::AttributesComponent,
                            fpl::fpl_project::AttributesData)

namespace fpl {
namespace fpl_project {

void AttributesComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();
  input_system_ = services->input_system();
  font_manager_ = services->font_manager();
  asset_manager_ = services->asset_manager();
  event::EventManager* event_manager = services->event_manager();
  event_manager->RegisterListener(EventSinkUnion_ModifyAttribute, this);
}

void AttributesComponent::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_ModifyAttribute: {
      auto* mod_event = event_payload.ToData<ModifyAttributePayload>();
      AttributesData* attributes_data = Data<AttributesData>(mod_event->target);
      if (attributes_data) {
        int index = mod_event->modify_attribute->attrib();
        float value = attributes_data->attribute(index);
        ApplyOperation(&value, mod_event->modify_attribute->op(),
                       mod_event->modify_attribute->value());
        attributes_data->set_attribute(index, value);
      }
      break;
    }
    default: { assert(0); }
  }
}

void AttributesComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef& entity = iter->entity;
    AttributesData* attributes_data = Data<AttributesData>(entity);
    GraphData* graph_data = Data<GraphData>(entity);
    if (attributes_data->dirty_bit()) {
      attributes_data->clear_dirty_bit();
      if (graph_data) {
        graph_data->broadcaster.BroadcastEvent(
            kEventIdEntityAttributesChanged);
      }
    }
  }
}

void AttributesComponent::AddFromRawData(entity::EntityRef& entity,
                                         const void* /*raw_data*/) {
  AddEntity(entity);
}

entity::ComponentInterface::RawDataUniquePtr AttributesComponent::ExportRawData(
    const entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  fbb.Finish(CreateAttributesDef(fbb));
  return fbb.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
