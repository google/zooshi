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
#include "component_library/transform.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::zooshi::TimeLimitComponent,
                            fpl::zooshi::TimeLimitData)

namespace fpl {
namespace zooshi {

// Time (in ms) taken for objects to shrink away before they're actually
// removed.
const entity::WorldTime kShrinkTime = 1000;

void TimeLimitComponent::AddFromRawData(entity::EntityRef& entity,
                                        const void* raw_data) {
  auto time_limit_def = static_cast<const TimeLimitDef*>(raw_data);
  TimeLimitData* time_limit_data = AddEntity(entity);
  // Time limit is specified in seconds in the data files.
  time_limit_data->time_limit = static_cast<entity::WorldTime>(
      time_limit_def->timelimit() * 1000);
}

void TimeLimitComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    TimeLimitData* time_limit_data = Data<TimeLimitData>(iter->entity);
    time_limit_data->time_elapsed += delta_time;
    if (time_limit_data->time_elapsed >=
        time_limit_data->time_limit - kShrinkTime) {
      component_library::TransformData* transform_data =
          Data<component_library::TransformData>(iter->entity);
      if (transform_data) {
        float scale_factor = (time_limit_data->time_limit -
            time_limit_data->time_elapsed) / static_cast<float>(kShrinkTime);
        transform_data->scale = time_limit_data->original_scale * scale_factor;
      }
    }
    if (time_limit_data->time_elapsed >= time_limit_data->time_limit) {
      entity_manager_->DeleteEntity(iter->entity);
    }
  }
}

entity::ComponentInterface::RawDataUniquePtr TimeLimitComponent::ExportRawData(
    const entity::EntityRef& entity) const {
  const TimeLimitData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  fbb.Finish(CreateTimeLimitDef(fbb, static_cast<float>(data->time_limit)));
  return fbb.ReleaseBufferPointer();
}

void TimeLimitComponent::InitEntity(entity::EntityRef& entity) {
  TimeLimitData* time_limit_data = Data<TimeLimitData>(entity);
  component_library::TransformData* transform_data =
      Data<component_library::TransformData>(entity);
  if (transform_data) {
    time_limit_data->original_scale = transform_data->scale;
  }
}

}  // zooshi
}  // fpl
