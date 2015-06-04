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
#include "components/services.h"
#include "components/transform.h"
#include "components_generated.h"
#include "fplbase/mesh.h"

using mathfu::vec3;
using mathfu::mat4;

namespace fpl {
namespace fpl_project {

// Offset the frustrum by this many world-units.  As long as no objects are
// larger than this number, they should still all draw, even if their
// registration points technically fall outside our frustrum.
static const float kFrustrumOffset = 50.0f;

void RenderMeshComponent::Init() {
  asset_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->asset_manager();
}

// Rendermesh depends on transform:
void RenderMeshComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RenderMeshComponent::RenderPrep(const Camera& camera) {
  for (int pass = 0; pass < RenderPass_kCount; pass++) {
    pass_render_list[pass].clear();
  }
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RenderMeshData* rendermesh_data = GetComponentData(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);

    float max_cos = cos(camera.viewport_angle());
    vec3 camera_facing = camera.facing();
    vec3 camera_position = camera.position();

    // Put each entity into the list for each render pass it is
    // planning on participating in.
    for (int pass = 0; pass < RenderPass_kCount; pass++) {
      if (rendermesh_data->pass_mask & (1 << pass)) {
        if (!rendermesh_data->ignore_culling) {
          // Check to make sure objects are inside the frustrum of our
          // view-cone before we draw:
          vec3 entity_position =
              transform_data->world_transform.TranslationVector3D();
          vec3 pos_relative_to_camera = (entity_position - camera_position) +
                                        camera_facing * kFrustrumOffset;

          // Cache off the distance from the camera because we'll use it
          // later as a depth aproxamation.
          rendermesh_data->z_depth =
              (entity_position - camera.position()).LengthSquared();

          if (vec3::DotProduct(pos_relative_to_camera.Normalized(),
                               camera_facing.Normalized()) < max_cos) {
            // The origin point for this mesh is not in our field of view.  Cut
            // out early, and don't bother rendering it.
            continue;
          }
        }
        pass_render_list[pass].push_back(
            RenderlistEntry(iter->entity, &iter->data));
      }
    }
  }
  std::sort(pass_render_list[RenderPass_kOpaque].begin(),
            pass_render_list[RenderPass_kOpaque].end());
  std::sort(pass_render_list[RenderPass_kAlpha].begin(),
            pass_render_list[RenderPass_kAlpha].end(),
              std::greater<RenderlistEntry>());
}


void RenderMeshComponent::RenderAllEntities(Renderer& renderer,
                                            const Camera& camera) {
  for (int pass = 0; pass < RenderPass_kCount; pass++) {
    for (size_t i = 0; i < pass_render_list[pass].size(); i++) {
      entity::EntityRef& entity = pass_render_list[pass][i].entity;

      TransformData* transform_data = Data<TransformData>(entity);
      RenderMeshData* rendermesh_data = Data<RenderMeshData>(entity);

      mat4 world_transform = transform_data->world_transform;

      const mat4 mvp = camera.GetTransformMatrix() * world_transform;
      const mat4 world_matrix_inverse = world_transform.Inverse();

      renderer.camera_pos() = world_matrix_inverse * camera.position();
      renderer.light_pos() = world_matrix_inverse * light_position_;
      renderer.model_view_projection() = mvp;
      renderer.color() = rendermesh_data->tint;

      if (rendermesh_data->shader) {
        rendermesh_data->shader->Set(renderer);
      }
      rendermesh_data->mesh->Render(renderer);
    }
  }
}

void RenderMeshComponent::AddFromRawData(entity::EntityRef& entity,
                                         const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_RenderMeshDef);
  auto rendermesh_def =
      static_cast<const RenderMeshDef*>(component_data->data());

  // You need to call asset_manager before you can add from raw data,
  // otherwise it can't load up new meshes!
  assert(asset_manager_ != nullptr);
  assert(rendermesh_def->source_file() != nullptr);
  assert(rendermesh_def->shader() != nullptr);

  RenderMeshData* rendermesh_data = AddEntity(entity);

  rendermesh_data->mesh_filename = rendermesh_def->source_file()->c_str();
  rendermesh_data->shader_filename = rendermesh_def->shader()->c_str();
  rendermesh_data->ignore_culling = rendermesh_def->ignore_culling();

  rendermesh_data->mesh =

  asset_manager_->LoadMesh(rendermesh_def->source_file()->c_str());
  assert(rendermesh_data->mesh != nullptr);
  rendermesh_data->shader =
    asset_manager_->LoadShader(rendermesh_def->shader()->c_str());
  assert(rendermesh_data->shader != nullptr);
  rendermesh_data->ignore_culling = rendermesh_def->ignore_culling();

  rendermesh_data->pass_mask = 0;
  if (rendermesh_def->render_pass() != nullptr) {
    for (size_t i = 0; i < rendermesh_def->render_pass()->size(); i++) {
      int render_pass = rendermesh_def->render_pass()->Get(i);
      assert(render_pass < RenderPass_kCount);
      rendermesh_data->pass_mask |= 1 << render_pass;
    }
  } else {
    // Anything unspecified is assumed to be opaque.
    rendermesh_data->pass_mask = (1 << RenderPass_kOpaque);
  }

  // TODO: Load this from a flatbuffer file instead of setting it.
  rendermesh_data->tint = mathfu::kOnes4f;
}

entity::ComponentInterface::RawDataUniquePtr RenderMeshComponent::ExportRawData(
    entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* RenderMeshComponent::PopulateRawData(entity::EntityRef& entity,
                                           void* helper) const {
  const RenderMeshData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;
  if (data->mesh_filename == "" || data->shader_filename == "") {
    // If we don't have a mesh filename or a shader, we can't be exported;
    // we were obviously created programatically.
    return nullptr;
  }
  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);

  auto source_file =
      (data->mesh_filename != "") ? fbb->CreateString(data->mesh_filename) : 0;
  auto shader = (data->shader_filename != "")
                    ? fbb->CreateString(data->shader_filename)
                    : 0;

  RenderMeshDefBuilder builder(*fbb);
  if (data->mesh_filename != "") {
    builder.add_source_file(source_file);
  }
  if (data->shader_filename != "") {
    builder.add_shader(shader);
  }
  builder.add_ignore_culling(data->ignore_culling);

  auto component = CreateComponentDefInstance(
      *fbb, ComponentDataUnion_RenderMeshDef, builder.Finish().Union());

  return reinterpret_cast<void*>(component.o);
}

}  // fpl_project
}  // fpl
