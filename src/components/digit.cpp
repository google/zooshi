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

#include "components/digit.h"

#include "component_library/rendermesh.h"
#include "components/attributes.h"
#include "components/player.h"
#include "components/services.h"
#include "components_generated.h"
#include "fplbase/asset_manager.h"
#include "fplbase/utilities.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::DigitComponent,
                            fpl::fpl_project::DigitData)

namespace fpl {
namespace fpl_project {

using fpl::component_library::RenderMeshComponent;
using fpl::component_library::RenderMeshData;

static const int kDigitBase = 10;

void DigitComponent::AddFromRawData(entity::EntityRef& entity,
                                    const void* raw_data) {
  auto digit_def = static_cast<const DigitDef*>(raw_data);

  // If digit meshes are specified, all digits need to be declared.
  assert(!digit_def->digit_mesh_list() ||
         digit_def->digit_mesh_list()->size() == kDigitBase);

  DigitData* digit_data = AddEntity(entity);
  if (digit_def->divisor() >= 0) {
    digit_data->divisor = digit_def->divisor();
  }

  // Assign meshes.
  AssetManager* asset_manager =
      entity_manager_->GetComponent<ServicesComponent>()->asset_manager();

  if (digit_def->digit_mesh_list()) {
    for (unsigned int i = 0; i < digit_def->digit_mesh_list()->size(); ++i) {
      digit_data->digits[i] = asset_manager->LoadMesh(
          digit_def->digit_mesh_list()->Get(i)->c_str());
    }
  }

  // Assign player.
  if (digit_def->attrib() != AttributeDef_Unspecified) {
    digit_data->attribute = digit_def->attrib();
  }

  RenderMeshData* render_mesh_data = Data<RenderMeshData>(entity);
  if (digit_def->shader()) {
    render_mesh_data->shader =
        asset_manager->LoadShader(digit_def->shader()->c_str());
  }
  if (digit_def->render_pass() != nullptr) {
    for (size_t i = 0; i < digit_def->render_pass()->size(); i++) {
      int render_pass = digit_def->render_pass()->Get(i);
      assert(render_pass < RenderPass_Count);
      render_mesh_data->pass_mask |= 1 << render_pass;
    }
  }
  // TODO: Load this from a flatbuffer file instead of setting it.
  render_mesh_data->tint = mathfu::kOnes4f;

  // If meshes were loaded, then set the initial one since RenderMeshData should
  // always have a valid mesh.
  if (digit_def->digit_mesh_list()) {
    render_mesh_data->mesh = digit_data->digits[0];
  }
}

void DigitComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<RenderMeshComponent>(entity);
}

void DigitComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  entity::EntityRef player =
      entity_manager_->GetComponent<PlayerComponent>()->begin()->entity;
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    DigitData* digit_data = Data<DigitData>(iter->entity);

    AttributesData* attribute_data = Data<AttributesData>(player);
    float value = attribute_data->attribute(digit_data->attribute);
    int index = static_cast<int>(value) / digit_data->divisor % kDigitBase;

    RenderMeshData* render_mesh_data = Data<RenderMeshData>(iter->entity);
    render_mesh_data->mesh = digit_data->digits[index];
  }
}

}  // fpl_project
}  // fpl
