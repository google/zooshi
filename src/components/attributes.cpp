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

#include "components/attributes.h"
#include "components/services.h"
#include "transform.h"
#include "event/event_manager.h"
#include "event/event_payload.h"
#include "events/modify_attribute.h"
#include "events/utilities.h"
#include "event/event_manager.h"
#include "events_generated.h"
#include "fplbase/asset_manager.h"
#include "fplbase/utilities.h"
#include "imgui/imgui.h"

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
        float* attribute =
            &attributes_data->attributes[mod_event->modify_attribute->attrib()];
        ApplyOperation(attribute, mod_event->modify_attribute->op(),
                       mod_event->modify_attribute->value());
      }
      break;
    }
    default: { assert(0); }
  }
}

void AttributesComponent::AddFromRawData(entity::EntityRef& entity,
                                         const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  (void)component_data;
  assert(component_data->data_type() == ComponentDataUnion_AttributesDef);
  AddEntity(entity);
}

entity::ComponentInterface::RawDataUniquePtr AttributesComponent::ExportRawData(
    entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* AttributesComponent::PopulateRawData(entity::EntityRef& entity,
                                           void* helper) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);
  auto component =
      CreateComponentDefInstance(*fbb, ComponentDataUnion_AttributesDef,
                                 CreateAttributesDef(*fbb).Union());
  return reinterpret_cast<void*>(component.o);
}

}  // fpl_project
}  // fpl
