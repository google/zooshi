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

#include "components_generated.h"
#include "config_generated.h"
#include "flatbuffers/flatbuffers.h"
#include "fplbase/input.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "world.h"
#include "zooshi_entity_factory.h"
#include "fplbase/render_target.h"

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

static const int kShadowMapTextureID = 7;

static const char kEntityLibraryFile[] = "entity_prototypes.bin";

// count = 5  ==>  low = -2, high = 3  ==> range -2..2
// count = 6  ==>  low = -3, high = 3  ==> range -2..3
static RangeInt GridRange(int count) {
  return RangeInt(-count / 2, (count + 1) / 2);
}

void World::Initialize(const Config& config_, InputSystem* input_system,
                       BasePlayerController* input_controller,
                       AssetManager* asset_mgr, WorldRenderer* worldrenderer,
                       FontManager* font_manager,
                       pindrop::AudioEngine* audio_engine,
                       event::EventManager* event_manager, Renderer* renderer) {
  entity_factory.reset(new ZooshiEntityFactory());
  motive::SmoothInit::Register();

  asset_manager = asset_mgr;
  world_renderer = worldrenderer;

  config = &config_;

  physics_component.set_gravity(config->gravity());
  physics_component.set_max_steps(config->bullet_max_steps());

  // Important!  Registering and initializing the services components needs
  // to happen BEFORE other components are registered, because many of them
  // depend on it during their own init functions.
  common_services_component.Initialize(asset_manager, entity_factory.get(),
                                       event_manager, input_system, renderer);
  services_component.Initialize(
      config, asset_manager, input_system, audio_engine, &motive_engine,
      event_manager, font_manager, &rail_manager, entity_factory.get());

  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&common_services_component),
      ComponentDataUnion_ServicesDef, "CommonServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&services_component),
      ComponentDataUnion_ServicesDef, "ServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&attributes_component),
      ComponentDataUnion_AttributesDef, "AttributesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_denizen_component),
      ComponentDataUnion_RailDenizenDef, "RailDenizenDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&player_component),
      ComponentDataUnion_PlayerDef, "PlayerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&player_projectile_component),
      ComponentDataUnion_PlayerProjectileDef, "PlayerProjectileDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&render_mesh_component),
      ComponentDataUnion_RenderMeshDef, "RenderMeshDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&physics_component),
      ComponentDataUnion_PhysicsDef, "PhysicsDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&patron_component),
      ComponentDataUnion_PatronDef, "PatronDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&time_limit_component),
      ComponentDataUnion_TimeLimitDef, "TimeLimitDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&audio_listener_component),
      ComponentDataUnion_ListenerDef, "ListenerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&sound_component),
      ComponentDataUnion_SoundDef, "SoundDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&digit_component),
      ComponentDataUnion_DigitDef, "DigitDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&river_component),
      ComponentDataUnion_RiverDef, "RiverDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&shadow_controller_component),
      ComponentDataUnion_ShadowControllerDef, "ShadowControllerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&meta_component_),
      ComponentDataUnion_MetaDef, "MetaDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&edit_options_component_),
      ComponentDataUnion_EditOptionsDef, "EditOptionsDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_node_component),
      ComponentDataUnion_RailNodeDef, "RailNodeDef");
  // Make sure you register TransformComponent after any components that use it.
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&transform_component),
      ComponentDataUnion_TransformDef, "TransformDef");

  services_component.LoadComponentDefBinarySchema(
      "flatbufferschemas/components.bfbs");
  entity_factory->set_debug_entity_creation(false);
  entity_factory->SetFlatbufferSchema("flatbufferschemas/components.bfbs");
  entity_factory->AddEntityLibrary(kEntityLibraryFile);

  entity_manager.set_entity_factory(entity_factory.get());

  render_mesh_component.set_light_position(vec3(-10, -20, 20));

  for (size_t i = 0; i < config->entity_files()->size(); i++) {
    const char* filename = config->entity_files()->Get(i)->c_str();
    entity_factory->LoadEntitiesFromFile(filename, &entity_manager);
  }

  for (auto iter = player_component.begin(); iter != player_component.end();
       ++iter) {
    entity_manager.GetComponentData<PlayerData>(iter->entity)
        ->set_input_controller(input_controller);
  }
  active_player_entity = player_component.begin()->entity;

  transform_component.PostLoadFixup();  // sets up parent-child links
  patron_component.PostLoadFixup();

  entity::EntityRef player_entity = player_component.begin()->entity;
  auto player_transform = transform_component.GetComponentData(player_entity);
  entity::EntityRef raft_entity = player_transform->parent;
  services_component.set_raft_entity(raft_entity);
}

}  // fpl_project
}  // fpl
