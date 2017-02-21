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

#include "world.h"

#include "breadboard/graph_factory.h"
#include "components_generated.h"
#include "config_generated.h"
#include "corgi_component_library/default_entity_factory.h"

#include "mathfu/internal/disable_warnings_begin.h"

#include "firebase/analytics.h"
#include "firebase/invites.h"

#include "mathfu/internal/disable_warnings_end.h"

#include "flatbuffers/flatbuffers.h"
#include "fplbase/input.h"
#include "fplbase/render_target.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"

#if FPLBASE_ANDROID_VR
#include "fplbase/renderer_hmd.h"
#endif

using scene_lab::SceneLab;
using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

static const char kEntityLibraryFile[] = "entity_prototypes.zooentity";
static const char kComponentDefBinarySchema[] =
    "flatbufferschemas/components.bfbs";

void World::Initialize(
    const Config& config_, fplbase::InputSystem* input_system,
    fplbase::AssetManager* asset_mgr, WorldRenderer* worldrenderer,
    flatui::FontManager* font_manager, pindrop::AudioEngine* audio_engine,
    breadboard::GraphFactory* graph_factory, fplbase::Renderer* renderer,
    SceneLab* scene_lab, UnlockableManager* unlockable_mgr, XpSystem* xpsystem,
    InvitesListener* invites_lstr, MessageListener* message_lstr,
    AdMobHelper* admob_hlpr) {
  entity_factory.reset(new corgi::component_library::DefaultEntityFactory());
  motive::SplineInit::Register();
  motive::MatrixInit::Register();
  motive::OvershootInit::Register();
  motive::RigInit::Register();

  asset_manager = asset_mgr;
  world_renderer = worldrenderer;
  unlockables = unlockable_mgr;
  xp_system = xpsystem;

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
                                entity_factory.get(), this, scene_lab);

  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&common_services_component),
      ComponentDataUnion_ServicesDef, "corgi.CommonServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&services_component),
      ComponentDataUnion_ServicesDef, "corgi.ServicesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&graph_component),
      ComponentDataUnion_corgi_GraphDef, "corgi.GraphDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&attributes_component),
      ComponentDataUnion_AttributesDef, "fpl.AttributesDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_denizen_component),
      ComponentDataUnion_RailDenizenDef, "fpl.RailDenizenDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&simple_movement_component),
      ComponentDataUnion_SimpleMovementDef, "fpl.SimpleMovementDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&lap_dependent_component),
      ComponentDataUnion_LapDependentDef, "fpl.LapDependentDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&player_component),
      ComponentDataUnion_PlayerDef, "fpl.PlayerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&player_projectile_component),
      ComponentDataUnion_PlayerProjectileDef, "fpl.PlayerProjectileDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&render_mesh_component),
      ComponentDataUnion_corgi_RenderMeshDef, "corgi.RenderMeshDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&physics_component),
      ComponentDataUnion_corgi_PhysicsDef, "corgi.PhysicsDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&patron_component),
      ComponentDataUnion_PatronDef, "fpl.PatronDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&time_limit_component),
      ComponentDataUnion_TimeLimitDef, "fpl.TimeLimitDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&audio_listener_component),
      ComponentDataUnion_ListenerDef, "fpl.ListenerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&sound_component),
      ComponentDataUnion_SoundDef, "fpl.SoundDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&river_component),
      ComponentDataUnion_RiverDef, "fpl.RiverDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&shadow_controller_component),
      ComponentDataUnion_ShadowControllerDef, "fpl.ShadowControllerDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&meta_component),
      ComponentDataUnion_corgi_MetaDef, "corgi.MetaDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&edit_options_component),
      ComponentDataUnion_scene_lab_EditOptionsDef, "scene_lab.EditOptionsDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&scenery_component),
      ComponentDataUnion_SceneryDef, "fpl.SceneryDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&animation_component),
      ComponentDataUnion_corgi_AnimationDef, "corgi.AnimationDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&rail_node_component),
      ComponentDataUnion_RailNodeDef, "fpl.RailNodeDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&render_3d_text_component),
      ComponentDataUnion_Render3dTextDef, "fpl.Render3dTextDef");
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&light_component),
      ComponentDataUnion_LightDef, "fpl.LightDef");
  // Make sure you register TransformComponent after any components that use it.
  entity_factory->SetComponentType(
      entity_manager.RegisterComponent(&transform_component),
      ComponentDataUnion_corgi_TransformDef, "corgi.TransformDef");

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

  cardboard_settings_gear =
      asset_manager->FindMaterial("materials/settings_gear.fplmat");

  rendering_options_[kRenderingMonoscopic][kShadowEffect] =
      config->rendering_config()->render_shadows_by_default();
  rendering_options_[kRenderingMonoscopic][kPhongShading] =
      config->rendering_config()->apply_phong_by_default();
  rendering_options_[kRenderingMonoscopic][kSpecularEffect] =
      config->rendering_config()->apply_specular_by_default();

  rendering_options_[kRenderingStereoscopic][kShadowEffect] =
      config->rendering_config()->render_shadows_by_default_cardboard();
  rendering_options_[kRenderingStereoscopic][kPhongShading] =
      config->rendering_config()->apply_phong_by_default_cardboard();
  rendering_options_[kRenderingStereoscopic][kSpecularEffect] =
      config->rendering_config()->apply_specular_by_default_cardboard();

  invites_listener = invites_lstr;
  message_listener = message_lstr;
  admob_helper = admob_hlpr;
}

void World::AddController(BasePlayerController* controller) {
  input_controllers.push_back(
      std::unique_ptr<BasePlayerController>(controller));
}

void World::SetActiveController(ControllerType controller_type) {
  bool set_controller = false;
  for (auto it = input_controllers.begin(); it != input_controllers.end();
       ++it) {
    BasePlayerController* controller = it->get();
    if (controller->controller_type() == controller_type &&
        controller->enabled()) {
      for (auto iter = player_component.begin(); iter != player_component.end();
           ++iter) {
        entity_manager.GetComponentData<PlayerData>(iter->entity)
            ->set_input_controller(controller);
        set_controller = true;
      }
      break;
    }
  }
  assert(set_controller);
  (void)set_controller;
}

void World::ResetControllerFacing() {
  for (auto it = input_controllers.begin(); it != input_controllers.end();
       ++it) {
    it->get()->ResetFacing();
  }
}

void World::SetRenderingMode(RenderingMode rendering_mode) {
  if (rendering_mode == rendering_mode_) return;

  rendering_mode_ = rendering_mode;
  rendering_dirty_ = true;
#if FPLBASE_ANDROID_VR
  // Turn on the Cardboard setting button when in Cardboard mode.
  fplbase::SetCardboardButtonEnabled(rendering_mode == kRenderingStereoscopic);
#endif  // FPLBASE_ANDROID_VR
}

void World::SetRenderingOption(RenderingMode rendering_mode, ShaderDefines s,
                               bool enable_option) {
  assert(0 <= rendering_mode && rendering_mode <= kNumRenderingModes &&
         0 <= s && s < kNumShaderDefines);

  // Early out if nothing is being changed.
  // Prevents unnecessary dirtying.
  if (rendering_options_[rendering_mode][s] == enable_option) return;

  // Update the option. Mark dirty if the option is currently active.
  rendering_options_[rendering_mode][s] = enable_option;
  if (rendering_mode_ == rendering_mode) {
    rendering_dirty_ = true;
  }
}

bool World::RenderingOptionEnabled(ShaderDefines s) const {
  assert(0 <= s && s < kNumShaderDefines);
  return rendering_options_[rendering_mode_][s];
}

bool World::RenderingOptionEnabled(RenderingMode rendering_mode,
                                   ShaderDefines s) const {
  assert(0 <= rendering_mode && rendering_mode <= kNumRenderingModes &&
         0 <= s && s < kNumShaderDefines);
  return rendering_options_[rendering_mode][s];
}

void LoadWorldDef(World* world, const WorldDef* world_def) {
  for (auto iter = world->entity_manager.begin();
       iter != world->entity_manager.end(); ++iter) {
    world->entity_manager.DeleteEntity(iter.ToReference());
  }
  world->entity_manager.DeleteMarkedEntities();
  assert(world->entity_manager.begin() == world->entity_manager.end());
  for (size_t i = 0; i < world_def->entity_files()->size(); i++) {
    flatbuffers::uoffset_t index = static_cast<flatbuffers::uoffset_t>(i);
    const char* filename = world_def->entity_files()->Get(index)->c_str();
    world->entity_factory->LoadEntitiesFromFile(filename,
                                                &world->entity_manager);
  }
  const LevelDef* level_def = world_def->levels()->Get(
    static_cast<flatbuffers::uoffset_t>(world->level_index));
  for (size_t i = 0; i < level_def->entity_files()->size(); i++) {
    const char* filename = level_def->entity_files()->Get(
      static_cast<flatbuffers::uoffset_t>(i))->c_str();
    world->entity_factory->LoadEntitiesFromFile(filename,
                                                &world->entity_manager);
  }

  world->SetActiveController(kControllerDefault);
  world->active_player_entity = world->player_component.begin()->entity;

  world->transform_component.PostLoadFixup();  // sets up parent-child links
  world->patron_component.PostLoadFixup();
  world->rail_denizen_component.PostLoadFixup();
  world->scenery_component.PostLoadFixup();

  corgi::EntityRef player_entity = world->player_component.begin()->entity;
  world->services_component.set_player_entity(player_entity);
  auto player_transform =
      world->transform_component.GetComponentData(player_entity);
  corgi::EntityRef raft_entity = player_transform->parent;
  world->services_component.set_raft_entity(raft_entity);

  world->graph_component.PostLoadFixup();
}

}  // zooshi
}  // fpl
