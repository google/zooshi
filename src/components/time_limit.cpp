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

#include "components/time_limit.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

void TimeLimitComponent::AddFromRawData(entity::EntityRef& entity,
                                        const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_TimeLimitDef);
  auto time_limit_def =
      static_cast<const TimeLimitDef*>(component_data->data());
  TimeLimitData* time_limit_data = AddEntity(entity);
  // Time limit is specified in seconds in the data files.
  time_limit_data->time_limit =
      time_limit_def->timelimit() * kMillisecondsPerSecond;
}

void TimeLimitComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    TimeLimitData* time_limit_data = Data<TimeLimitData>(iter->entity);
    time_limit_data->time_elapsed += delta_time;
    if (time_limit_data->time_elapsed >= time_limit_data->time_limit) {
      entity_manager_->DeleteEntity(iter->entity);
    }
  }
}

entity::ComponentInterface::RawDataUniquePtr TimeLimitComponent::ExportRawData(
    entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* TimeLimitComponent::PopulateRawData(entity::EntityRef& entity,
                                          void* helper) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  const TimeLimitData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);
  auto component = CreateComponentDefInstance(
      *fbb, ComponentDataUnion_TimeLimitDef,
      CreateTimeLimitDef(*fbb, data->time_limit).Union());
  return reinterpret_cast<void*>(component.o);
}

}  // fpl_project
}  // fpl
