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

#include "config_generated.h"
#include "components_generated.h"
#include "components/river.h"
#include "components/rendermesh.h"
#include "components/rail_denizen.h"
#include "components/services.h"
#include "fplbase/utilities.h"
#include <math.h>
#include <memory>

using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

void RiverComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<RenderMeshComponent>(entity);

  CreateRiverMesh(entity);
}

void RiverComponent::AddFromRawData(entity::EntityRef& parent,
                                    const void* /*raw_data*/) {
  AddEntity(parent);
}

entity::ComponentInterface::RawDataUniquePtr RiverComponent::ExportRawData(
    entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* RiverComponent::PopulateRawData(entity::EntityRef& entity,
                                      void* helper) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);
  auto component = CreateComponentDefInstance(*fbb, ComponentDataUnion_RiverDef,
                                              CreateRiverDef(*fbb).Union());
  return reinterpret_cast<void*>(component.o);
}

// Generates the actual mesh for the river, and adds it to this entitiy's
// rendermesh component.
void RiverComponent::CreateRiverMesh(entity::EntityRef& entity) {
  static const Attribute kMeshFormat[] = {kPosition3f, kTexCoord2f, kNormal3f,
                                          kTangent4f, kEND};
  std::vector<mathfu::vec3_packed> track;

  const Config* config =
      entity_manager_->GetComponent<ServicesComponent>()->config();

  Rail* rail = entity_manager_->GetComponent<ServicesComponent>()
                   ->rail_manager()
                   ->GetRail(config->river_config()->rail_filename()->c_str());

  // Generate the spline data and store it in our track vector:
  rail->Positions(config->river_config()->spline_stepsize(), &track);

  size_t segment_count = track.size();
  size_t river_vert_max = segment_count * 2;
  size_t river_index_max = (segment_count - 1) * 6;
  size_t bank_segments = 3;
  size_t bank_vert_max = segment_count * 2 * (bank_segments + 1);
  size_t bank_index_max = (segment_count - 1) * 6 * (bank_segments * 2);

  // Need to allocate some space to plan out our mesh in:
  std::vector<NormalMappedVertex> river_verts(river_vert_max);
  river_verts.clear();
  std::vector<unsigned short> river_indices(river_index_max);
  river_indices.clear();

  std::vector<NormalMappedVertex> bank_verts(bank_vert_max);
  bank_verts.clear();
  std::vector<unsigned short> bank_indices(bank_index_max);
  bank_indices.clear();

  // the normal to where we are on the track right now.  Initialized to
  // our best guess, based on the first two points.
  vec3 track_normal =
      vec3::CrossProduct(vec3(track[1]) - vec3(track[0]), mathfu::kAxisZ3f)
          .Normalized();

  // Construct the actual mesh data for the river:
  for (size_t i = 0; i < segment_count; i++) {
    if (i != 0) {
      track_normal = vec3::CrossProduct(vec3(track[i]) - vec3(track[i - 1]),
                                        mathfu::kAxisZ3f)
                         .Normalized();
    }
    track_normal *= config->river_config()->track_half_width();
    vec3 track_pos = vec3(track[i]);
    float texture_y = config->river_config()->texture_tile_size() *
                      static_cast<float>(i) / static_cast<float>(segment_count);

    auto make_vert = [&](std::vector<NormalMappedVertex>& verts, float xpos,
                         float xtc, float ypos) {
      verts.push_back(NormalMappedVertex{
          vec3_packed(
              vec3(track_pos + track_normal * xpos) +
              vec3(0.0f, 0.0f, config->river_config()->track_height() + ypos)),
          vec2_packed(vec2(xtc, texture_y)), vec3_packed(vec3(0, 1, 0)),
          vec4_packed(vec4(1, 0, 0, 1)),
      });
    };

    make_vert(river_verts, -1, 0, 0);
    make_vert(river_verts, 1, 1, 0);

    make_vert(bank_verts, -3.0f, -2.0f, 0.5f + mathfu::Random<float>() * 0.7f);
    make_vert(bank_verts, -2.0f, -1.5f, 0.7f + mathfu::Random<float>() * 0.7f);
    make_vert(bank_verts, -1.2f, -1.2f, 0.3f + mathfu::Random<float>() * 0.7f);
    make_vert(bank_verts, -1.0f, -1.0f, 0.0f);  // Matches river vert.
    make_vert(bank_verts, 1.0f, 1.0f, 0.0f);    // Matches river vert.
    make_vert(bank_verts, 1.2f, 1.2f, 0.3f + mathfu::Random<float>() * 0.7f);
    make_vert(bank_verts, 2.0f, 1.5f, 0.7f + mathfu::Random<float>() * 0.7f);
    make_vert(bank_verts, 3.0f, 2.0f, 0.5f + mathfu::Random<float>() * 0.7f);

    // Force the beginning and end to line up in their geometry:
    if (i == segment_count - 1) {
      river_verts[river_verts.size() - 2].pos = river_verts[0].pos;
      river_verts[river_verts.size() - 1].pos = river_verts[1].pos;

      for (size_t j = 0; j < 8; j++)
        bank_verts[bank_verts.size() - (8 - j)].pos = bank_verts[j].pos;
    }

    // Not counting the first segment, create triangles in our index
    // list to represent this segment.
    if (i != 0) {
      auto make_quad = [&](std::vector<unsigned short>& indices,
                           std::vector<NormalMappedVertex>& verts, int off1,
                           int off2) {
        indices.push_back(verts.size() - 2 - off2);
        indices.push_back(verts.size() - 1 - off2);
        indices.push_back(verts.size() - 2 - off1);

        indices.push_back(verts.size() - 2 - off1);
        indices.push_back(verts.size() - 1 - off2);
        indices.push_back(verts.size() - 1 - off1);
      };

      make_quad(river_indices, river_verts, 0, 2);

      make_quad(bank_indices, bank_verts, 0, 8);
      make_quad(bank_indices, bank_verts, 1, 9);
      make_quad(bank_indices, bank_verts, 2, 10);
      make_quad(bank_indices, bank_verts, 4, 12);
      make_quad(bank_indices, bank_verts, 5, 13);
      make_quad(bank_indices, bank_verts, 6, 14);
    }
  }

  // Make sure we used as much data as expected, and no more.  (Or less!)
  assert(river_indices.size() == river_index_max);
  assert(river_verts.size() == river_vert_max);
  assert(bank_indices.size() == bank_index_max);
  assert(bank_verts.size() == bank_vert_max);

  Mesh::ComputeNormalsTangents(bank_verts.data(), bank_indices.data(),
                               bank_verts.size(), bank_indices.size());

  AssetManager* asset_manager =
      entity_manager_->GetComponent<ServicesComponent>()->asset_manager();

  // Load the material from files.
  Material* river_material =
      asset_manager->LoadMaterial(config->river_config()->material()->c_str());
  Material* bank_material =
      asset_manager->LoadMaterial("materials/ground_material.fplmat");

  // Create the actual mesh objects, and stuff all the data we just
  // generated into it.
  Mesh* river_mesh = new Mesh(river_verts.data(), river_verts.size(),
                              sizeof(NormalMappedVertex), kMeshFormat);

  river_mesh->AddIndices(river_indices.data(), river_indices.size(),
                         river_material);

  Mesh* bank_mesh = new Mesh(bank_verts.data(), bank_verts.size(),
                             sizeof(NormalMappedVertex), kMeshFormat);

  bank_mesh->AddIndices(bank_indices.data(), bank_indices.size(),
                        bank_material);

  // Add the river mesh to the river entity.
  RenderMeshData* mesh_data = Data<RenderMeshData>(entity);
  mesh_data->shader =
      asset_manager->LoadShader(config->river_config()->shader()->c_str());
  mesh_data->mesh = river_mesh;
  mesh_data->ignore_culling = true;  // Never cull the river.
  mesh_data->pass_mask = 1 << RenderPass_kOpaque;

  RiverData* river_data = Data<RiverData>(entity);
  if (!river_data->bank) {
    // Now we make a new entity to hold the bank mesh.
    river_data->bank = entity_manager_->AllocateNewEntity();
    entity_manager_->AddEntityToComponent<RenderMeshComponent>(
        river_data->bank);

    // Then we stick it as a child of the river entity, so it always moves
    // with it and stays aligned:
    TransformComponent* transform_component =
        GetComponent<TransformComponent>();
    transform_component->AddChild(river_data->bank, entity);
  }

  RenderMeshData* child_render_data = Data<RenderMeshData>(river_data->bank);
  child_render_data->shader = asset_manager->LoadShader("shaders/cardboard");
  child_render_data->mesh = bank_mesh;
  child_render_data->ignore_culling = true;  // Never cull the banking.
  child_render_data->pass_mask = 1 << RenderPass_kOpaque;
}

}  // fpl_project
}  // fpl
