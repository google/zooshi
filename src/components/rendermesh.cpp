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
#include "components_generated.h"
#include "fplbase/mesh.h"

using mathfu::vec3;
using mathfu::mat4;

namespace fpl {
namespace fpl_project {

void RenderMeshComponent::Initialize(mathfu::vec3 light_position,
                                     MaterialManager* material_manager) {
  light_position_ = light_position;
  material_manager_ = material_manager;
}

// Rendermesh depends on transform:
void RenderMeshComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RenderMeshComponent::RenderEntity(entity::EntityRef& entity,
                                       Renderer& renderer,
                                       const Camera& camera) {
  TransformData* transform_data = Data<TransformData>(entity);
  RenderMeshData* rendermesh_data = Data<RenderMeshData>(entity);

  mat4 world_transform = transform_data->world_transform;

  const mat4 mvp = camera.GetTransformMatrix() * world_transform;
  const mat4 world_matrix_inverse = world_transform.Inverse();

  renderer.camera_pos() = world_matrix_inverse * camera.position();
  renderer.light_pos() = world_matrix_inverse * light_position_;
  renderer.model_view_projection() = mvp;

  if (rendermesh_data->shader) {
    rendermesh_data->shader->Set(renderer);
  }
  rendermesh_data->mesh->Render(renderer);
}

void RenderMeshComponent::RenderAllEntities(Renderer& renderer,
                                            const Camera& camera) {
  // todo(ccornell) - instead of iterating like this, sort by
  // z depth and alpha blend mode.
  for (auto iter = component_data_.begin();
       iter != component_data_.end(); ++iter) {
    RenderEntity(iter->entity, renderer, camera);
  }
}

void RenderMeshComponent::AddFromRawData(entity::EntityRef& entity,
                                         const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_RenderMeshDef);
  auto rendermesh_def =
      static_cast<const RenderMeshDef*>(component_data->data());

  // You need to call set_material_manager before you can add from raw data,
  // otherwise it can't load up new meshes!
  assert(material_manager_ != nullptr);
  assert(rendermesh_def->source_file() != nullptr);
  assert(rendermesh_def->shader() != nullptr);

  RenderMeshData* rendermesh_data = AddEntity(entity);

  rendermesh_data->mesh =
      material_manager_->LoadMesh(rendermesh_def->source_file()->c_str());
  rendermesh_data->shader =
      material_manager_->LoadShader(rendermesh_def->shader()->c_str());

  // TODO: Load this from a flatbuffer file instead of setting it.
  rendermesh_data->tint = mathfu::kOnes4f;
}

}  // fpl_project
}  // fpl
