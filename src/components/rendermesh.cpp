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

using mathfu::vec3;
using mathfu::mat4;

namespace fpl {
namespace fpl_project {

// Rendermesh depends on transform:
void RenderMeshComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RenderMeshComponent::RenderAllEntities(Renderer& renderer,
                                            const Camera& camera) {
  // todo(ccornell) - instead of iterating like this, sort by
  // z depth and alpha blend mode.
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    TransformData* transform_data = Data<TransformData>(iter->entity);
    RenderMeshData* rendermesh_data = Data<RenderMeshData>(iter->entity);

    mat4 object_world_matrix = transform_data->GetTransformMatrix();

    const mat4 mvp = camera.GetTransformMatrix() * object_world_matrix;
    const mat4 world_matrix_inverse = object_world_matrix.Inverse();

    rendermesh_data->shader->Set(renderer);

    renderer.camera_pos() = world_matrix_inverse * camera.position();
    renderer.light_pos() = world_matrix_inverse * light_position_;
    renderer.model_view_projection() = mvp;

    rendermesh_data->mesh->Render(renderer);
  }
}

}  // fpl_project
}  // fpl

