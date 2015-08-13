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

#include "components/lap_dependent.h"

#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "world_editor/editor_event.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::LapDependentComponent,
                            fpl::fpl_project::LapDependentData)

namespace fpl {
namespace fpl_project {

using fpl::component_library::PhysicsComponent;
using fpl::component_library::RenderMeshComponent;

void LapDependentComponent::Init() {
  auto event_manager =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  event_manager->RegisterListener(EventSinkUnion_EditorEvent, this);
}

void LapDependentComponent::AddFromRawData(entity::EntityRef& entity,
                                           const void* raw_data) {
  auto lap_dependent_def = static_cast<const LapDependentDef*>(raw_data);
  LapDependentData* lap_dependent_data = AddEntity(entity);
  lap_dependent_data->min_lap = lap_dependent_def->min_lap();
  lap_dependent_data->max_lap = lap_dependent_def->max_lap();
}

entity::ComponentInterface::RawDataUniquePtr
LapDependentComponent::ExportRawData(const entity::EntityRef& entity) const {
  const LapDependentData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  LapDependentDefBuilder builder(fbb);
  builder.add_min_lap(data->min_lap);
  builder.add_max_lap(data->max_lap);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void LapDependentComponent::InitEntity(entity::EntityRef& /*entity*/) {}

void LapDependentComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  if (!raft) return;
  RailDenizenData* raft_rail_denizen = Data<RailDenizenData>(raft);
  float lap = raft_rail_denizen != nullptr ? raft_rail_denizen->lap : 0.0f;
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    LapDependentData* data = GetComponentData(iter->entity);
    if (lap >= data->min_lap && lap <= data->max_lap) {
      if (data->currently_active) continue;
      data->currently_active = true;
      auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
      if (rm_component) {
        rm_component->SetHiddenRecursively(iter->entity, false);
      }
      auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
      if (phys_component) {
        phys_component->EnablePhysics(iter->entity);
      }
    } else if (data->currently_active) {
      data->currently_active = false;
      auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
      if (rm_component) {
        rm_component->SetHiddenRecursively(iter->entity, true);
      }
      auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
      if (phys_component) {
        phys_component->DisablePhysics(iter->entity);
      }
    }
  }
}

void LapDependentComponent::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_EditorEvent: {
      auto* event = event_payload.ToData<editor::EditorEventPayload>();
      if (event->action == EditorEventAction_Enter) {
        // Make sure all entities are activated and visible.
        for (auto iter = component_data_.begin(); iter != component_data_.end();
             ++iter) {
          ActivateEntity(iter->entity);
        }
      } else if (event->action == EditorEventAction_Exit) {
        // Deactivate them all, as they reactivate during update.
        for (auto iter = component_data_.begin(); iter != component_data_.end();
             ++iter) {
          DeactivateEntity(iter->entity);
        }
      }
      break;
    }
    default: { assert(0); }
  }
}

void LapDependentComponent::ActivateEntity(entity::EntityRef& entity) {
  auto data = GetComponentData(entity);
  if (!data) return;

  data->currently_active = true;
  auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
  if (rm_component) {
    rm_component->SetHiddenRecursively(entity, false);
  }
  auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (phys_component) {
    phys_component->EnablePhysics(entity);
  }
}

void LapDependentComponent::DeactivateEntity(entity::EntityRef& entity) {
  auto data = GetComponentData(entity);
  if (!data) return;

  data->currently_active = false;
  auto rm_component = entity_manager_->GetComponent<RenderMeshComponent>();
  if (rm_component) {
    rm_component->SetHiddenRecursively(entity, true);
  }
  auto phys_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (phys_component) {
    phys_component->DisablePhysics(entity);
  }
}

}  // fpl_project
}  // fpl
