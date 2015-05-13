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

#include "transform.h"
#include <math.h>

namespace fpl {
namespace fpl_project {

static const float kDegreesToRadians = M_PI / 180.0f;

void TransformComponent::AddFromRawData(entity::EntityRef& entity,
                                        const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_TransformDef);
  auto transform_def = static_cast<const TransformDef*>(component_data->data());
  auto pos = transform_def->position();
  auto orientation = transform_def->orientation();
  auto scale = transform_def->scale();
  auto transform_data = AddEntity(entity);
  // TODO: Move vector loading into a function in fplbase.
  if (pos != nullptr) {
    transform_data->position = mathfu::vec3(pos->x(), pos->y(), pos->z());
  }
  if (orientation != nullptr) {
    transform_data->orientation = mathfu::quat::FromEulerAngles(
        mathfu::vec3(orientation->x(), orientation->y(), orientation->z()) *
        kDegreesToRadians);
  }
  if (scale != nullptr) {
    transform_data->scale = mathfu::vec3(scale->x(), scale->y(), scale->z());
  }
}

entity::ComponentInterface::RawDataUniquePtr TransformComponent::ExportRawData(
    entity::EntityRef& entity) const {
  flatbuffers::FlatBufferBuilder builder;
  const TransformData* data = GetEntityData(entity);
  mathfu::vec3 euler = data->orientation.ToEulerAngles();
  fpl::Vec3 position{data->position.x(), data->position.y(),
                     data->position.z()};
  fpl::Vec3 scale{data->scale.x(), data->scale.y(), data->scale.z()};
  fpl::Vec3 orientation{euler.x(), euler.y(), euler.z()};
  auto transform_def =
      CreateTransformDef(builder, &position, &scale, &orientation);
  builder.Finish(transform_def);

  return builder.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
