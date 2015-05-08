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

#include "components/family.h"

namespace fpl {
namespace fpl_project {

void FamilyComponent::AddFromRawData(entity::EntityRef& parent,
                                     const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_FamilyDef);
  auto family_def = static_cast<const FamilyDef*>(component_data->data());
  auto family_data = AddEntity(parent);

  for (size_t i = 0; i < family_def->children()->size(); i++) {
    auto child = entity_factory_->CreateEntityFromData(
        family_def->children()->Get(i), entity_manager_);
    auto child_family_data = AddEntity(child);
    family_data->children.InsertBefore(child_family_data->child_node());
    child_family_data->parent = parent;
  }
}

void FamilyComponent::InitEntity(entity::EntityRef& entity) {
  FamilyData* family_data = Data<FamilyData>(entity);
  family_data->owner = entity;
}

}  // fpl_project
}  // fpl

