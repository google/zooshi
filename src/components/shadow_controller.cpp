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

#include "components/shadow_controller.h"
#include "components/transform.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

static const float kShadowHeight = 0.15f;

void ShadowControllerComponent::AddFromRawData(entity::EntityRef& entity,
                                               const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_ShadowControllerDef);
  auto shadow_controller_def =
      static_cast<const ShadowControllerDef*>(component_data->data());
  ShadowControllerData* shadow_controller_data = AddEntity(entity);

  (void)(shadow_controller_data);
  (void)shadow_controller_def;
}

void ShadowControllerComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    ShadowControllerData* shadow_data =
        Data<ShadowControllerData>(iter->entity);

    TransformData* transform_data = Data<TransformData>(iter->entity);
    if (!transform_data->parent) {
      // No parent yet, don't do anything.
      continue;
    }

    if (!shadow_data->shadow_caster.IsValid()) {
      shadow_data->shadow_caster = transform_data->parent;

      entity_manager_->GetComponent<TransformComponent>()->RemoveChild(
          iter->entity);
    }

    TransformData* parent_transform_data =
        Data<TransformData>(shadow_data->shadow_caster);

    transform_data->position =
        mathfu::vec3(parent_transform_data->position.x(),
                     parent_transform_data->position.y(), kShadowHeight);
  }
}

entity::ComponentInterface::RawDataUniquePtr
ShadowControllerComponent::ExportRawData(entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  auto component =
      CreateComponentDefInstance(fbb, ComponentDataUnion_ShadowControllerDef,
                                 CreateShadowControllerDef(fbb).Union());
  fbb.Finish(component);
  return fbb.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
