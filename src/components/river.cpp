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

// Generates the actual mesh for the river, and adds it to this entitiy's
// rendermesh component.
void RiverComponent::CreateRiverMesh(entity::EntityRef& entity) {
  static const Attribute kMeshFormat[] = {kPosition3f, kTexCoord2f, kNormal3f,
                                          kTangent4f, kEND};
  std::vector<mathfu::vec3_packed> track;

  int index_count = 0;
  int vectex_count = 0;

  RailDenizenComponent* rd_comp =
      entity_manager_->GetComponent<RailDenizenComponent>();

  Rail* rail = rd_comp->rail();
  const Config* config =
      entity_manager_->GetComponent<ServicesComponent>()->config();

  // Generate the spline data and store it in our track vector:
  rail->Positions(config->river_config()->spline_stepsize(), &track);

  int segment_count = track.size();
  int vert_max = segment_count * 2;
  int index_max = (segment_count - 1) * 6;

  // Need to allocate some space to plan out our mesh in:
  // Throw everything into unique pointers, to ensure that it gets deallocated
  // when we're done with it:
  std::unique_ptr<NormalMappedVertex[]> verts(new NormalMappedVertex[vert_max]);
  std::unique_ptr<unsigned short[]> indexes(new unsigned short[index_max]);

  // the normal to where we are on the track right now.  Initialized to
  // our best guess, based on the first two points.
  vec3 track_normal =
      vec3::CrossProduct(vec3(track[1]) - vec3(track[0]), mathfu::kAxisZ3f)
          .Normalized();

  // Construct the actual mesh data for the river:
  for (int i = 0; i < segment_count; i++) {
    if (i != 0) {
      track_normal = vec3::CrossProduct(vec3(track[i]) - vec3(track[i - 1]),
                                        mathfu::kAxisZ3f)
                         .Normalized();
    }
    track_normal *= config->river_config()->track_half_width();
    vec3 track_pos = vec3(track[i]);

    verts[vectex_count].pos =
        vec3(track_pos - track_normal) +
        vec3(0.0f, 0.0f, config->river_config()->track_height());
    verts[vectex_count].norm = vec3(0, 1, 0);
    verts[vectex_count].tangent = vec4(1, 0, 0, 1);
    verts[vectex_count].tc = vec2(
        0, static_cast<float>(i) / config->river_config()->texture_tile_size());
    vectex_count++;

    verts[vectex_count].pos =
        vec3(track_pos + track_normal) +
        vec3(0.0f, 0.0f, config->river_config()->track_height());
    verts[vectex_count].norm = vec3(0, 1, 0);
    verts[vectex_count].tangent = vec4(1, 0, 0, 1);
    verts[vectex_count].tc = vec2(
        1, static_cast<float>(i) / config->river_config()->texture_tile_size());
    vectex_count++;

    if (i != 0) {
      indexes[index_count++] = vectex_count - 4;
      indexes[index_count++] = vectex_count - 3;
      indexes[index_count++] = vectex_count - 2;

      indexes[index_count++] = vectex_count - 2;
      indexes[index_count++] = vectex_count - 3;
      indexes[index_count++] = vectex_count - 1;
    }
  }

  // Make sure we used as much data as expected, and no more.  (Or less!)
  assert(index_count == index_max);
  assert(vectex_count == vert_max);

  MaterialManager* material_manager =
      entity_manager_->GetComponent<ServicesComponent>()->material_manager();

  // Load the material from file, and check validity.
  Material* material = material_manager->LoadMaterial(
      config->river_config()->material()->c_str());

  // Create the actual mesh object, and stuff all the data we just
  // generated into it.
  Mesh* mesh =
      new Mesh(verts.get(), vectex_count, sizeof(NormalMappedVertex),
               kMeshFormat);

  mesh->AddIndices(indexes.get(), index_count, material);

  RenderMeshData* mesh_data = Data<RenderMeshData>(entity);
  mesh_data->shader =
      material_manager->LoadShader(config->river_config()->shader()->c_str());
  mesh_data->mesh = mesh;
  mesh_data->ignore_culling = true;  // Never cull the river.
}

}  // fpl_project
}  // fpl
