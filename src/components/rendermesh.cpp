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

#include "components/rendermesh.h"
#include "components/transform.h"
#include "components/family.h"

using mathfu::vec3;
using mathfu::mat4;

namespace fpl {
namespace fpl_project {

// Rendermesh depends on transform:
void RenderMeshComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RenderMeshComponent::RenderEntity(entity::EntityRef& entity,
                                       mat4 transform, Renderer& renderer,
                                       const Camera& camera) {
  FamilyData* family_data = Data<FamilyData>(entity);
  TransformData* transform_data = Data<TransformData>(entity);
  RenderMeshData* rendermesh_data = Data<RenderMeshData>(entity);

  transform *= transform_data->GetTransformMatrix();

  const mat4 mvp = camera.GetTransformMatrix() * transform;
  const mat4 world_matrix_inverse = transform.Inverse();

  renderer.camera_pos() = world_matrix_inverse * camera.position();
  renderer.light_pos() = world_matrix_inverse * light_position_;
  renderer.model_view_projection() = mvp;

  rendermesh_data->shader->Set(renderer);
  rendermesh_data->mesh->Render(renderer);

  // Render children, if any exist.
  if (family_data) {
    for (IntrusiveListNode* node = family_data->children.GetNext();
         node != family_data->children.GetTerminator();
         node = node->GetNext()) {
      FamilyData* child_family_data =
          FamilyData::GetInstanceFromChildNode(node);
      RenderEntity(child_family_data->owner, transform, renderer, camera);
    }
  }
}

void RenderMeshComponent::RenderAllEntities(Renderer& renderer,
                                            const Camera& camera) {
  // todo(ccornell) - instead of iterating like this, sort by
  // z depth and alpha blend mode.
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    FamilyData* family_data = Data<FamilyData>(iter->entity);
    // Only draw root level entities. Children (i.e. entities with a parent)
    // will be drawn recursively.
    if (!family_data || !family_data->parent.IsValid()) {
      RenderEntity(iter->entity, mat4::Identity(), renderer, camera);
    }
  }
}

}  // fpl_project
}  // fpl

