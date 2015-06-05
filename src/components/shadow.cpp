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

#include "components/shadow.h"
#include "components/transform.h"
#include "components/services.h"

namespace fpl {
namespace fpl_project {

void ShadowComponent::InitEntity(entity::EntityRef &entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}


void ShadowComponent::AddFromRawData(entity::EntityRef &entity,
                                     const void *raw_data) {
  auto component_data = static_cast<const ComponentDefInstance *>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_ShadowDef);
  auto shadow_data = AddEntity(entity);
  shadow_data->shadowdef =
    reinterpret_cast<const ShadowDef *>(component_data->data());
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void ShadowComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
}

void ShadowComponent::RenderAllEntities(const mat4 &cam) {
  auto &matman =
    *entity_manager_->GetComponent<ServicesComponent>()->material_manager();
  auto &renderer = matman.renderer();
  auto blobmat = matman.FindMaterial("materials/blob.fplmat");
  auto sh = matman.FindShader("shaders/textured");
  const float shadowlevel = 0.5f;
  renderer.color() = vec4(shadowlevel);
  auto quad_bottom_left = vec3(-1.0f, -1.0f, 0.1f);
  auto quad_top_right = vec3(1.0f, 1.0f, 0.1f);
  // TODO: these shadows are our only alpha items so likely should be
  // rendered last anyway, which is most efficiently done as below. If
  // we need to Z-sort with other alpha textures however, this may need to be
  // moved into a rendermesh or similar.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    auto transform_data = Data<TransformData>(iter->entity);
    auto shadow_data = Data<ShadowData>(iter->entity);
    renderer.model_view_projection() =
      cam * mat4::FromTranslationVector(transform_data->position);
    sh->Set(renderer);
    blobmat->Set(renderer);
    auto r = shadow_data->shadowdef->radius();
    Mesh::RenderAAQuadAlongX(quad_bottom_left * r, quad_top_right * r);
  }
}

}  // fpl_project
}  // fpl

