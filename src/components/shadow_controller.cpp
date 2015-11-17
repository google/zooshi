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
#include "corgi_component_library/transform.h"
#include "fplbase/utilities.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::ShadowControllerComponent,
                       fpl::zooshi::ShadowControllerData)

namespace fpl {
namespace zooshi {

using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;

static const float kShadowHeight = 0.15f;

void ShadowControllerComponent::AddFromRawData(corgi::EntityRef& entity,
                                               const void* raw_data) {
  auto shadow_controller_def =
      static_cast<const ShadowControllerDef*>(raw_data);
  ShadowControllerData* shadow_controller_data = AddEntity(entity);

  (void)(shadow_controller_data);
  (void)shadow_controller_def;
}

void ShadowControllerComponent::UpdateAllEntities(
    corgi::WorldTime /*delta_time*/) {
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

corgi::ComponentInterface::RawDataUniquePtr
ShadowControllerComponent::ExportRawData(const corgi::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateShadowControllerDef(fbb));
  return fbb.ReleaseBufferPointer();
}

}  // zooshi
}  // fpl
