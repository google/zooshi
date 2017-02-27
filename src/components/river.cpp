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

#include "components/river.h"
#include <math.h>
#include <memory>
#include "common.h"
#include "components/rail_denizen.h"
#include "components/rail_node.h"
#include "components/services.h"
#include "components_generated.h"
#include "config_generated.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/rendermesh.h"
#include "corgi_component_library/transform.h"
#include "fplbase/debug_markers.h"
#include "fplbase/utilities.h"
#include "scene_lab/scene_lab.h"
#include "world.h"

using mathfu::vec2;
using mathfu::vec2_packed;
using mathfu::vec3;
using mathfu::vec3_packed;
using mathfu::vec4;
using mathfu::vec4_packed;
using mathfu::quat;
using mathfu::kAxisZ3f;
using fplbase::Material;
using fplbase::Mesh;

CORGI_DEFINE_COMPONENT(fpl::zooshi::RiverComponent, fpl::zooshi::RiverData)

namespace fpl {
namespace zooshi {

using corgi::component_library::PhysicsComponent;
using corgi::component_library::RenderMeshComponent;
using corgi::component_library::RenderMeshData;
using scene_lab::SceneLab;

static const size_t kNumIndicesPerQuad = 6;

// A vertex definition specific to normalmapping with colors.
struct NormalMappedColorVertex {
  vec3_packed pos;
  vec2_packed tc;
  vec3_packed norm;
  vec4_packed tangent;
  unsigned char color[4];
};

void RiverComponent::Init() {
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  SceneLab* scene_lab = services->scene_lab();
  if (scene_lab) {
    scene_lab->AddOnUpdateEntityCallback(
        [this](const scene_lab::GenericEntityId& /*entity*/) {
          TriggerRiverUpdate();
        });
  }
  river_offset_ = 0;
}

void RiverComponent::AddFromRawData(corgi::EntityRef& entity,
                                    const void* raw_data) {
  auto river_def = static_cast<const RiverDef*>(raw_data);
  RiverData* river_data = AddEntity(entity);
  river_data->rail_name = river_def->rail_name()->c_str();
  river_data->random_seed = river_def->random_seed();

  entity_manager_->AddEntityToComponent<RenderMeshComponent>(entity);
  TriggerRiverUpdate();
}

// The update function here really just handles keeping the river offset
// up to date so the river scrolls correctly.
void RiverComponent::UpdateAllEntities(corgi::WorldTime /*delta_time*/) {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();
  corgi::EntityRef raft_entity = services->raft_entity();
  RailDenizenData* rd_raft_data = Data<RailDenizenData>(raft_entity);
  float speed = rd_raft_data->PlaybackRate();
  speed += services->world()->CurrentLevel()->river_config()->speed_boost();
  float texture_repeats =
      services->world()->CurrentLevel()->river_config()->texture_repeats();
  river_offset_ += speed / (texture_repeats * texture_repeats);
  river_offset_ -= floor(river_offset_);
}

void RiverComponent::TriggerRiverUpdate() {
  // TODO - it would be nice if this only updated the river that we were editing
  // (instead of marking all rivers as needing an update) but due to how river
  // edit nodes interact with the river mesh, that's tricky.
  for (auto iter = begin(); iter != end(); ++iter) {
    RiverData* river_data = Data<RiverData>(iter->entity);
    river_data->render_mesh_needs_update_ = true;
  }
}

corgi::ComponentInterface::RawDataUniquePtr RiverComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  const RiverData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  auto rail_name =
      data->rail_name != "" ? fbb.CreateString(data->rail_name) : 0;

  RiverDefBuilder builder(fbb);
  if (rail_name.o != 0) {
    builder.add_rail_name(rail_name);
  }
  builder.add_random_seed(data->random_seed);

  fbb.Finish(builder.Finish().Union());
  return fbb.ReleaseBufferPointer();
}

// Iterate through the river meshes and update any of them that need to be
// regenerated.  Split out into a separate function so it can be called from
// the render thread.  (Warning:  Crashes if you try to call it on the main
// thread, because it doesn't have access to the opengl context!)
void RiverComponent::UpdateRiverMeshes() {
  PushDebugMarker("UpdateRiverMeshes");
  for (auto iter = begin(); iter != end(); ++iter) {
    RiverData* river_data = Data<RiverData>(iter->entity);
    if (river_data->render_mesh_needs_update_) CreateRiverMesh(iter->entity);
  }
  PopDebugMarker();
}

// Generates the actual mesh for the river, and adds it to this entitiy's
// rendermesh component.
void RiverComponent::CreateRiverMesh(corgi::EntityRef& entity) {
  static const fplbase::Attribute kMeshFormat[] = {
      fplbase::kPosition3f, fplbase::kTexCoord2f, fplbase::kNormal3f,
      fplbase::kTangent4f, fplbase::kEND};
  static const fplbase::Attribute kBankMeshFormat[] = {
      fplbase::kPosition3f, fplbase::kTexCoord2f, fplbase::kNormal3f,
      fplbase::kTangent4f,  fplbase::kColor4ub,   fplbase::kEND};
  std::vector<vec3_packed> track;
  const RiverConfig* river = entity_manager_->GetComponent<ServicesComponent>()
                                 ->world()
                                 ->CurrentLevel()
                                 ->river_config();

  RiverData* river_data = Data<RiverData>(entity);
  river_data->render_mesh_needs_update_ = false;

  // Initialize the static mesh that will be made around the river banks.
  auto* physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  physics_component->InitStaticMesh(entity);

  Rail* rail = entity_manager_->GetComponent<ServicesComponent>()
                   ->rail_manager()
                   ->GetRailFromComponents(river_data->rail_name.c_str(),
                                           entity_manager_);

  // Generate the spline data and store it in our track vector:
  rail->Positions(river->spline_stepsize(), &track);

  fplbase::AssetManager* asset_manager =
      entity_manager_->GetComponent<ServicesComponent>()->asset_manager();

  const size_t num_bank_contours = river->default_banks()->Length();
  const size_t num_bank_quads = num_bank_contours - 2;
  const size_t river_idx = river->river_index();
  const size_t segment_count = track.size();
  const size_t river_vert_max = segment_count * 2;
  const size_t river_index_max = (segment_count - 1) * kNumIndicesPerQuad;
  const size_t bank_vert_max = segment_count * num_bank_contours;
  const size_t bank_index_max =
      (segment_count - 1) * kNumIndicesPerQuad * num_bank_quads;
  assert(num_bank_contours >= 2 && river_idx < num_bank_contours - 1);
  const unsigned int num_zones = river->zones()->Length();

  // Need to allocate some space to plan out our mesh in:
  std::vector<NormalMappedVertex> river_verts(river_vert_max);
  river_verts.clear();
  std::vector<unsigned short> river_indices(river_index_max);
  river_indices.clear();

  std::vector<NormalMappedColorVertex> bank_verts(bank_vert_max);
  bank_verts.clear();
  std::vector<unsigned short> bank_indices(bank_index_max);
  bank_indices.clear();
  // Use one set of bank vertices for the entire riverbank, but separate out
  // the zones via indices, so we can use different materials (and possibly
  // shaders) per zone. We still keep bank_indices for the normal calculation,
  // though.
  std::vector<std::vector<unsigned short>> bank_indices_by_zone;
  bank_indices_by_zone.resize(num_zones);

  std::vector<unsigned int> bank_zones;  // indexed by segment
  bank_zones.resize(segment_count, 0);   // default of 0
  unsigned int zone_id = 0;

  // TODO: Use a local random number generator. Resetting the global random
  //       number generator is not a nice. Also, there's no guarantee that
  //       mathfu::Random will continue to use rand().
  srand(river_data->random_seed);

  std::vector<float> actual_zone_end;
  actual_zone_end.resize(segment_count, 1);
  // Precalculate the actual zone end locations.
  for (size_t i = 0; i < segment_count; i++) {
    const float fraction =
        static_cast<float>(i) / static_cast<float>(segment_count);
    if (zone_id + 1 < river->zones()->Length() &&
        fraction > river->zones()->Get(zone_id + 1)->zone_start()) {
      actual_zone_end[zone_id] = fraction;
      zone_id = zone_id + 1;
    }
  }
  // Start over from zone 0.
  zone_id = 0;

  const RiverZone* current_zone = river->zones()->Get(zone_id);
  Material* current_bank_material =
      asset_manager->LoadMaterial(current_zone->material()->c_str());
  float river_width = current_zone->width() != 0 ? current_zone->width()
                                                 : river->default_width();

  // Construct the actual mesh data for the river:
  std::vector<vec2> offsets(num_bank_contours);
  for (size_t i = 0; i < segment_count; i++) {
    // Get the current position on the track, and the normal (to the side).
    vec3 track_delta;
    if (i > 0) {
      track_delta = vec3(track[i]) - vec3(track[i - 1]);
    } else if (rail->wraps()) {
      // River track is circular.
      track_delta = vec3(track[i]) - vec3(track[segment_count - 1]);
    } else {
      // Not circular, so point towards the next point.
      track_delta = vec3(track[1]) - vec3(track[0]);
    }
    const vec3 track_normal =
        vec3::CrossProduct(track_delta, kAxisZ3f).Normalized();
    const vec3 track_position =
        vec3(track[i]) + river->track_height() * kAxisZ3f;

    // The river texture is tiled several times along the course of the river.
    // TODO: Change this from tile count to actual physical size for a tile.
    //       Requires that we know the total path distance.
    const float texture_v = river->texture_tile_size() * static_cast<float>(i) /
                            static_cast<float>(segment_count);

    // Fraction of the river we have gone through, approximately.
    const float fraction =
        static_cast<float>(i) / static_cast<float>(segment_count);

    if (fraction >= actual_zone_end[zone_id]) {
      zone_id = zone_id + 1;
      current_zone = river->zones()->Get(zone_id);
      // Each zone has its own texture.
      current_bank_material =
          asset_manager->LoadMaterial(current_zone->material()->c_str());
      // Each zone has its own river width.
      river_width = current_zone->width() != 0 ? current_zone->width()
                                               : river->default_width();
      current_bank_material = asset_manager->LoadMaterial(
          river->zones()->Get(zone_id)->material()->c_str());
    }
    bank_zones[i] = zone_id;
    float zone_start = zone_id == 0 ? 0 : actual_zone_end[zone_id - 1];
    float zone_end = actual_zone_end[zone_id];
    float within_fraction = (fraction - zone_start) / (zone_end - zone_start);
    if (current_bank_material->textures().size() == 1) {
      // Ensure we stay continuous with transitional zones.
      within_fraction = within_fraction < 0.5f ? 1.0f : 0.0f;
    }

    int within_color = static_cast<int>(255.0 * within_fraction);
    // Cap the color to 0..255 byte.
    unsigned char within_color_byte = static_cast<unsigned char>(
        within_color < 0 ? 0 : (within_color > 255 ? 255 : within_color));

    // Get the (side, up) offsets of the bank vertices.
    // The offsets are relative to `track_position`.
    // side == distance along `track_normal`
    // up == distance along kAxisZ3f
    for (size_t j = 0; j < num_bank_contours; ++j) {
      flatbuffers::uoffset_t index = static_cast<flatbuffers::uoffset_t>(j);
      const RiverBankContour* b = (current_zone->banks() != nullptr)
                                      ? current_zone->banks()->Get(index)
                                      : river->default_banks()->Get(index);
      offsets[j] =
          vec2(mathfu::Lerp(b->x_min(), b->x_max(), mathfu::Random<float>()),
               mathfu::Lerp(b->z_min(), b->z_max(), mathfu::Random<float>()));
    }

    // Create the bank vertices for this segment.
    for (size_t j = 0; j < num_bank_contours; ++j) {
      const bool left_bank = j <= river_idx;
      const vec2 off = offsets[j];
      const vec3 vertex =
          track_position +
          (off.x + river_width * (left_bank ? -1 : 1)) * track_normal +
          off.y * kAxisZ3f;
      // The texture is stretched from the side of the river to the far end
      // of the bank. There are two banks, however, separated by the river.
      // We need to know the width of the bank to caluate the `texture_u`
      // coordinate.
      const size_t bank_start = left_bank ? 0 : num_bank_contours - 1;
      const size_t bank_end = left_bank ? river_idx : river_idx + 1;
      const float bank_width = offsets[bank_start].x - offsets[bank_end].x;
      const float texture_u = (off.x - offsets[bank_end].x) / bank_width;

      bank_verts.push_back(NormalMappedColorVertex());
      bank_verts.back().pos = vec3_packed(vertex);
      bank_verts.back().tc = vec2_packed(vec2(texture_u, texture_v));
      bank_verts.back().norm = vec3_packed(vec3(0, 1, 0));
      bank_verts.back().tangent = vec4_packed(vec4(1, 0, 0, 1));
      unsigned char color_bytes[4] = {255, 255, 255, within_color_byte};
      memcpy(bank_verts.back().color, color_bytes, sizeof(color_bytes));
    }

    // Ensure vertices don't go behind previous vertices on the inside of
    // a tight corner.
    if (i > 0) {
      const NormalMappedColorVertex* prev_verts =
          &bank_verts[bank_verts.size() - 2 * num_bank_contours];
      NormalMappedColorVertex* cur_verts =
          &bank_verts[bank_verts.size() - num_bank_contours];
      for (size_t j = 0; j < num_bank_contours; j++) {
        const vec3 vert_delta =
            vec3(cur_verts[j].pos) - vec3(prev_verts[j].pos);
        const float dot = vec3::DotProduct(vert_delta, track_delta);
        const bool cur_vert_goes_backwards_along_track = dot <= 0.0f;
        if (cur_vert_goes_backwards_along_track) {
          cur_verts[j].pos = vec3(prev_verts[j].pos) + 0.000001f * track_delta;
        }
      }
    }

    // Force the beginning and end to line up in their geometry:
    if (i == segment_count - 1 && rail->wraps()) {
      for (size_t j = 0; j < num_bank_contours; j++)
        bank_verts[bank_verts.size() - (8 - j)].pos = bank_verts[j].pos;
    }

    // The river has two of the middle vertices of the bank.
    // The texture coordinates are different, however.
    const size_t river_vert = bank_verts.size() - num_bank_contours + river_idx;
    float normalized_texture_v = i / static_cast<float>(segment_count);
    river_verts.push_back(NormalMappedVertex());
    river_verts.back().pos = bank_verts[river_vert].pos;
    river_verts.back().tc = vec2(0.0f, normalized_texture_v);
    river_verts.back().norm = bank_verts[river_vert].norm;
    river_verts.back().tangent = bank_verts[river_vert].tangent;

    river_verts.push_back(NormalMappedVertex());
    river_verts.back().pos = bank_verts[river_vert + 1].pos;
    river_verts.back().tc = vec2(1.0f, normalized_texture_v);
    river_verts.back().norm = bank_verts[river_vert + 1].norm;
    river_verts.back().tangent = bank_verts[river_vert + 1].tangent;
  }

  // Not counting the first segment, create triangles in our index
  // list to represent this segment.
  //
  for (size_t i = 0; i < segment_count - 1; i++) {
    auto make_quad = [&](std::vector<unsigned short>& indices, int base_index,
                         int off1, int off2) {
      indices.push_back(static_cast<unsigned short>(base_index + off1));
      indices.push_back(static_cast<unsigned short>(base_index + off1 + 1));
      indices.push_back(static_cast<unsigned short>(base_index + off2));

      indices.push_back(static_cast<unsigned short>(base_index + off2));
      indices.push_back(static_cast<unsigned short>(base_index + off1 + 1));
      indices.push_back(static_cast<unsigned short>(base_index + off2 + 1));
    };

    // River only has one quad per segment.
    make_quad(river_indices, 2 * static_cast<int>(i), 0, 2);

    // Case when kNumBankCountours = 8, and river_idx = 3;
    //
    //  0___1___2___3   4___5___6___7
    //  | _/| _/| _/|   | _/| _/| _/|
    //  |/__|/__|/__|   |/__|/__|/__|
    //  8   9  10  11  12  13  14  15
    for (size_t j = 0; j <= num_bank_quads; ++j) {
      // Do not create bank geo for the river.
      if (j == river_idx) continue;
      unsigned int zone = bank_zones[i];
      int base_index = static_cast<int>(i * num_bank_contours);
      int offset1 = static_cast<int>(j);
      int offset2 = static_cast<int>(num_bank_contours + j);
      make_quad(bank_indices, base_index, offset1, offset2);
      make_quad(bank_indices_by_zone[zone], base_index, offset1, offset2);

      // Add the same triangles to the static mesh associated with the entity.
      physics_component->AddStaticMeshTriangle(
          entity, vec3(bank_verts[base_index + offset1].pos),
          vec3(bank_verts[base_index + offset1 + 1].pos),
          vec3(bank_verts[base_index + offset2].pos));

      physics_component->AddStaticMeshTriangle(
          entity, vec3(bank_verts[base_index + offset2].pos),
          vec3(bank_verts[base_index + offset1 + 1].pos),
          vec3(bank_verts[base_index + offset2 + 1].pos));
    }
  }

  // Make sure we used as much data as expected, and no more.
  assert(river_indices.size() == river_index_max);
  assert(river_verts.size() == river_vert_max);
  assert(bank_indices.size() == bank_index_max);
  assert(bank_verts.size() == bank_vert_max);

  Mesh::ComputeNormalsTangents(bank_verts.data(), bank_indices.data(),
                               static_cast<int>(bank_verts.size()),
                               static_cast<int>(bank_indices.size()));

  // Load the material from files.
  Material* river_material =
      asset_manager->LoadMaterial(river->material()->c_str());
  // Create the actual mesh objects, and stuff all the data we just
  // generated into it.
  Mesh* river_mesh =
      new Mesh(river_verts.data(), river_verts.size(),
               static_cast<int>(sizeof(NormalMappedVertex)), kMeshFormat);

  river_mesh->AddIndices(river_indices.data(),
                         static_cast<int>(river_indices.size()),
                         river_material);

  // Add the river mesh to the river entity.
  RenderMeshData* mesh_data = Data<RenderMeshData>(entity);
  mesh_data->shaders.push_back(
      asset_manager->LoadShader(river->shader()->c_str()));
  mesh_data->shaders.push_back(
      asset_manager->LoadShader("shaders/render_depth"));
  if (mesh_data->mesh != nullptr) {
    // Mesh's destructor handles cleaning up its GL buffers
    delete mesh_data->mesh;
    mesh_data->mesh = nullptr;
  }
  mesh_data->mesh = river_mesh;
  mesh_data->culling_mask = 0;  // Never cull the river.
  mesh_data->pass_mask = 1 << corgi::RenderPass_Opaque;
  mesh_data->debug_name = "river";

  river_data->banks.resize(num_zones, corgi::EntityRef());
  for (unsigned int zone = 0; zone < num_zones; zone++) {
    Material* bank_material = asset_manager->LoadMaterial(
        river->zones()->Get(zone)->material()->c_str());

    Mesh* bank_mesh =
        new Mesh(bank_verts.data(), static_cast<int>(bank_verts.size()),
                 sizeof(NormalMappedColorVertex), kBankMeshFormat);

    bank_mesh->AddIndices(bank_indices_by_zone[zone].data(),
                          static_cast<int>(bank_indices_by_zone[zone].size()),
                          bank_material);
    if (!river_data->banks[zone]) {
      // Now we make a new entity to hold the bank mesh.
      river_data->banks[zone] = entity_manager_->AllocateNewEntity();
      entity_manager_->AddEntityToComponent<RenderMeshComponent>(
          river_data->banks[zone]);

      // Then we stick it as a child of the river entity, so it always moves
      // with it and stays aligned:
      auto transform_component =
          GetComponent<corgi::component_library::TransformComponent>();
      transform_component->AddChild(river_data->banks[zone], entity);
    }

    RenderMeshData* child_render_data =
        Data<RenderMeshData>(river_data->banks[zone]);
    if (bank_material->textures().size() == 1) {
      child_render_data->shaders.push_back(
          asset_manager->LoadShader("shaders/textured_lit"));
    } else {
      child_render_data->shaders.push_back(
          asset_manager->LoadShader("shaders/bank"));
    }
    if (child_render_data->mesh != nullptr) {
      // Mesh's destructor handles cleaning up its GL buffers
      delete child_render_data->mesh;
      child_render_data->mesh = nullptr;
    }
    child_render_data->mesh = bank_mesh;
    child_render_data->culling_mask = 0;  // Don't cull the banks for now.
    child_render_data->pass_mask = 1 << corgi::RenderPass_Opaque;
    std::ostringstream debug_name;
    debug_name << "river bank" << zone + 1;
    child_render_data->debug_name = debug_name.str();
  }

  // Finalize the static physics mesh created on the river bank.
  short collision_type = static_cast<short>(river->collision_type());
  short collides_with = 0;
  if (river->collides_with()) {
    for (auto collides = river->collides_with()->begin();
         collides != river->collides_with()->end(); ++collides) {
      collides_with |= static_cast<short>(*collides);
    }
  }
  std::string user_tag = river->user_tag() ? river->user_tag()->c_str() : "";
  physics_component->FinalizeStaticMesh(entity, collision_type, collides_with,
                                        river->mass(), river->restitution(),
                                        user_tag);

  // We don't want to keep the random number generator set to the same
  // value every time we generate the river, so reset the seed back to time.
  srand(static_cast<unsigned int>(time(nullptr)));
}

void RiverComponent::UpdateRiverMeshes(corgi::EntityRef entity) {
  const RailNodeData* node_data =
      entity_manager_->GetComponentData<RailNodeData>(entity);
  if (node_data != nullptr) {
    // For now, update all rivers. In the future only update rivers that
    // have the same rail_name that just changed.
    for (auto iter = begin(); iter != end(); ++iter) {
      CreateRiverMesh(iter->entity);
    }
  }
}

}  // zooshi
}  // fpl
