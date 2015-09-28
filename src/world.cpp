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

#include "breadboard/graph_factory.h"
#include "components_generated.h"
#include "config_generated.h"
#include "flatbuffers/flatbuffers.h"
#include "fplbase/input.h"
#include "fplbase/render_target.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"
#include "world.h"
#include "zooshi_entity_factory.h"

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

using fpl::editor::WorldEditor;
using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const char kEntityLibraryFile[] = "entity_prototypes.bin";
static const char kComponentDefBinarySchema[] =
    "flatbufferschemas/components.bfbs";

void World::Initialize(const Config& config_, InputSystem* input_system,
                       AssetManager* asset_mgr, WorldRenderer* worldrenderer,
                       FontManager* font_manager,
                       pindrop::AudioEngine* audio_engine,
                       breadboard::GraphFactory* graph_factory,
                       Renderer* renderer, WorldEditor* world_editor) {
  entity_factory.reset(new ZooshiEntityFactory());
  motive::SmoothInit::Register();
  motive::MatrixInit::Register();
  motive::RigInit::Register();

  asset_manager = asset_mgr;
  world_renderer = worldrenderer;

  config = &config_;

  physics_component.set_gravity(config->gravity());
  physics_component.set_max_steps(config->bullet_max_steps());

  // Important!  Registering and initializing the services components needs
  // to happen BEFORE other components are registered, because many of them
  // depend on it during their own init functions.
  common_services_component.Initialize(asset_manager, entity_factory.get(),
                                       graph_factory, input_system, renderer);
  services_component.Initialize(config, asset_manager, input_system,
                                audio_engine, font_manager, &rail_manager,
                                entity_factory.get(), this, world_editor);

  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&common_services_component),
      ComponentDataUnion_ServicesDef, "CommonServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&services_component),
      ComponentDataUnion_ServicesDef, "ServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&graph_component),
      ComponentDataUnion_GraphDef, "GraphDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&attributes_component),
      ComponentDataUnion_AttributesDef, "AttributesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_denizen_component),
      ComponentDataUnion_RailDenizenDef, "RailDenizenDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&simple_movement_component),
      ComponentDataUnion_SimpleMovementDef, "SimpleMovementDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&lap_dependent_component),
      ComponentDataUnion_LapDependentDef, "LapDependentDef");
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
      entity_manager.RegisterComponent(&meta_component),
      ComponentDataUnion_MetaDef, "MetaDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&edit_options_component),
      ComponentDataUnion_EditOptionsDef, "EditOptionsDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&scenery_component),
      ComponentDataUnion_SceneryDef, "SceneryDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&animation_component),
      ComponentDataUnion_AnimationDef, "AnimationDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_node_component),
      ComponentDataUnion_RailNodeDef, "RailNodeDef");
  // Make sure you register TransformComponent after any components that use it.
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&transform_component),
      ComponentDataUnion_TransformDef, "TransformDef");

  physics_component.set_collision_callback(&PatronComponent::CollisionHandler,
                                           &patron_component);

  services_component.LoadComponentDefBinarySchema(kComponentDefBinarySchema);
  entity_factory->set_debug_entity_creation(false);
  entity_factory->SetFlatbufferSchema(kComponentDefBinarySchema);
  entity_factory->AddEntityLibrary(kEntityLibraryFile);

  entity_manager.set_entity_factory(entity_factory.get());

  render_mesh_component.set_light_position(vec3(-10, -20, 20));
  render_mesh_component.SetCullDistance(
      config->rendering_config()->cull_distance());
}

void World::AddController(BasePlayerController* controller) {
  input_controllers.push_back(
      std::unique_ptr<BasePlayerController>(controller));
}

void World::SetActiveController(ControllerType controller_type) {
  for (auto it = input_controllers.begin(); it != input_controllers.end();
       ++it) {
    BasePlayerController* controller = it->get();
    if (controller->controller_type() == controller_type) {
      for (auto iter = player_component.begin(); iter != player_component.end();
           ++iter) {
        entity_manager.GetComponentData<PlayerData>(iter->entity)
            ->set_input_controller(controller);
      }
      break;
    }
  }
}

void LoadWorldDef(World* world, const WorldDef* world_def) {
  for (auto iter = world->entity_manager.begin();
       iter != world->entity_manager.end(); ++iter) {
    world->entity_manager.DeleteEntity(iter.ToReference());
  }
  world->entity_manager.DeleteMarkedEntities();
  assert(world->entity_manager.begin() == world->entity_manager.end());
  for (size_t i = 0; i < world_def->entity_files()->size(); i++) {
    const char* filename = world_def->entity_files()->Get(i)->c_str();
    world->entity_factory->LoadEntitiesFromFile(filename,
                                                &world->entity_manager);
  }

  world->SetActiveController(kControllerDefault);
  world->active_player_entity = world->player_component.begin()->entity;

  world->transform_component.PostLoadFixup();  // sets up parent-child links
  world->patron_component.PostLoadFixup();
  world->rail_denizen_component.PostLoadFixup();
  world->scenery_component.PostLoadFixup();

  entity::EntityRef player_entity = world->player_component.begin()->entity;
  world->services_component.set_player_entity(player_entity);
  auto player_transform =
      world->transform_component.GetComponentData(player_entity);
  entity::EntityRef raft_entity = player_transform->parent;
  world->services_component.set_raft_entity(raft_entity);

  world->graph_component.PostLoadFixup();
}

}  // fpl_project
}  // fpl
