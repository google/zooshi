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
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "world.h"

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

// count = 5  ==>  low = -2, high = 3  ==> range -2..2
// count = 6  ==>  low = -3, high = 3  ==> range -2..3
static RangeInt GridRange(int count) {
  return RangeInt(-count / 2, (count + 1) / 2);
}

void World::Initialize(const Config& config_, InputSystem* input_system,
                       BasePlayerController* input_controller,
                       MaterialManager* material_manager,
                       FontManager* font_manager,
                       pindrop::AudioEngine* audio_engine,
                       event::EventManager* event_manager) {
  motive::SmoothInit::Register();

  config = &config_;

  // Important!  Registering and initializing the services component needs
  // to happen BEFORE other components are registered, because many of them
  // depend on it during their own init functions.
  services_component.Initialize(config, material_manager, input_system,
                                audio_engine, &motive_engine, event_manager,
                                font_manager, &rail_manager);
  entity_manager.RegisterComponent(&services_component);

  entity_manager.RegisterComponent(&transform_component);
  entity_manager.RegisterComponent(&rail_denizen_component);
  entity_manager.RegisterComponent(&player_component);
  entity_manager.RegisterComponent(&player_projectile_component);
  entity_manager.RegisterComponent(&render_mesh_component);
  entity_manager.RegisterComponent(&physics_component);
  entity_manager.RegisterComponent(&patron_component);
  entity_manager.RegisterComponent(&time_limit_component);
  entity_manager.RegisterComponent(&audio_listener_component);
  entity_manager.RegisterComponent(&sound_component);
  entity_manager.RegisterComponent(&attributes_component);
  entity_manager.RegisterComponent(&river_component);

  entity_manager.set_entity_factory(&entity_factory);

  render_mesh_component.set_light_position(vec3(-10, -20, 20));

  // Create entities that are explicitly detailed in `entity_list`.
  for (size_t i = 0; i < config->entity_list()->size(); i++) {
    entity_manager.CreateEntityFromData(config->entity_list()->Get(i));
  }
  // Create entities in a grid formation, as specified by `entity_grid_list`.
  for (size_t i = 0; i < config->entity_grid_list()->size(); i++) {
    auto grid = config->entity_grid_list()->Get(i);
    auto entity_def = config->entity_defs()->Get(grid->entity());

    const vec3i count = LoadVec3i(grid->count());
    const RangeInt x_range = GridRange(count.x());
    const RangeInt y_range = GridRange(count.y());
    const RangeInt z_range = GridRange(count.z());
    const vec3 grid_position = LoadVec3(grid->position());
    const vec3 grid_separation = LoadVec3(grid->separation());
    const vec3 grid_scale = LoadVec3(grid->scale());

    for (int x = x_range.start(); x < x_range.end(); ++x) {
      for (int y = y_range.start(); y < y_range.end(); ++y) {
        for (int z = z_range.start(); z < z_range.end(); ++z) {
          entity::EntityRef entity =
              entity_manager.CreateEntityFromData(entity_def);

          TransformData* transform_data =
              entity_manager.GetComponentData<TransformData>(entity);

          const vec3 coord(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(z));
          transform_data->position = grid_position + coord * grid_separation;
          transform_data->scale = grid_scale;
        }
      }
    }
  }

  // Create entities in a ring formation, as specified by `entity_ring_list`.
  for (size_t i = 0; i < config->entity_ring_list()->size(); i++) {
    auto ring = config->entity_ring_list()->Get(i);
    auto entity_def = config->entity_defs()->Get(ring->entity());
    const vec3 ring_position = LoadVec3(ring->position());
    const vec3 ring_scale = LoadVec3(ring->scale());

    for (int j = 0; j < ring->count(); ++j) {
      entity::EntityRef entity =
          entity_manager.CreateEntityFromData(entity_def);

      TransformData* transform_data =
          entity_manager.GetComponentData<TransformData>(entity);

      const Angle position_angle =
          Angle::FromDegrees(ring->position_offset_angle()) +
          Angle::FromWithinThreePi(j * kTwoPi / ring->count());
      const Angle orientation_angle =
          Angle::FromDegrees(ring->orientation_offset_angle()) - position_angle;
      transform_data->position =
          ring_position + ring->radius() * position_angle.ToXYVector();
      transform_data->scale = ring_scale;
      transform_data->orientation =
          quat::FromAngleAxis(orientation_angle.ToRadians(), mathfu::kAxisZ3f);

      if (entity->IsRegisteredForComponent(
              physics_component.GetComponentId())) {
        physics_component.UpdatePhysicsFromTransform(entity);
      }
    }
  }

  // Create entity based on the predefined entity list.
  for (size_t i = 0; i < config->predefined_entity_list()->size(); i++) {
    auto predefined = config->predefined_entity_list()->Get(i);
    auto entity_def = config->entity_defs()->Get(predefined->entity());
    entity::EntityRef entity = entity_manager.CreateEntityFromData(entity_def);
    TransformData* transform_data =
        entity_manager.GetComponentData<TransformData>(entity);
    auto pos = predefined->position();
    auto orientation = predefined->orientation();
    auto scale = predefined->scale();
    if (pos != nullptr) {
      transform_data->position = LoadVec3(pos);
    }
    if (orientation != nullptr) {
      transform_data->orientation = mathfu::quat::FromEulerAngles(
          LoadVec3(orientation) * kDegreesToRadians);
    }
    if (scale != nullptr) {
      transform_data->scale = LoadVec3(scale);
    }
    if (entity->IsRegisteredForComponent(physics_component.GetComponentId())) {
      physics_component.UpdatePhysicsFromTransform(entity);
    }
  }

  for (auto iter = player_component.begin(); iter != player_component.end();
       ++iter) {
    entity_manager.GetComponentData<PlayerData>(iter->entity)
        ->set_input_controller(input_controller);
  }
  active_player_entity = player_component.begin()->entity;

  patron_component.PostLoadFixup();
}

}  // fpl_project
}  // fpl

