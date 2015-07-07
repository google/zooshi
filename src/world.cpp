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
#include "components/editor.h"
#include "flatbuffers/flatbuffers.h"
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

static const char kEntityLibraryFile[] = "entity_prototypes.bin";

// count = 5  ==>  low = -2, high = 3  ==> range -2..2
// count = 6  ==>  low = -3, high = 3  ==> range -2..3
static RangeInt GridRange(int count) {
  return RangeInt(-count / 2, (count + 1) / 2);
}

bool World::LoadEntitiesFromFile(const char* filename) {
  LogInfo("LoadEntitiesFromFile: Reading %s", filename);
  if (loaded_entity_files_.find(filename) == loaded_entity_files_.end()) {
    std::string data;
    if (!LoadFile(filename, &data)) {
      LogInfo("LoadEntitiesFromFile: Couldn't open file %s", filename);
      return false;
    }
    loaded_entity_files_[filename] = data;
  }
  const EntityListDef* list =
      GetEntityListDef(loaded_entity_files_[filename].c_str());
  int total = 0;
  for (size_t i = 0; i < list->entity_list()->size(); i++) {
    total++;
    entity::EntityRef entity =
        entity_manager.CreateEntityFromData(list->entity_list()->Get(i));
    // Make sure we track which file this entity came from.
    entity_manager.GetComponent<EditorComponent>()->AddWithSourceFile(entity,
                                                                      filename);
  }
  LogInfo("LoadEntitiesFromFile: Loaded %d entities from %s", total, filename);

  return true;
}

void World::Initialize(const Config& config_, InputSystem* input_system,
                       BasePlayerController* input_controller,
                       AssetManager* asset_manager, FontManager* font_manager,
                       pindrop::AudioEngine* audio_engine,
                       event::EventManager* event_manager) {
  motive::SmoothInit::Register();

  config = &config_;

  // Important!  Registering and initializing the services component needs
  // to happen BEFORE other components are registered, because many of them
  // depend on it during their own init functions.
  services_component.Initialize(config, asset_manager, input_system,
                                audio_engine, &motive_engine, event_manager,
                                font_manager, &rail_manager, &entity_factory);
  entity_factory.SetComponentId(
      ComponentDataUnion_ServicesDef,
      entity_manager.RegisterComponent(&services_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_AttributesDef,
      entity_manager.RegisterComponent(&attributes_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_RailDenizenDef,
      entity_manager.RegisterComponent(&rail_denizen_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_PlayerDef,
      entity_manager.RegisterComponent(&player_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_PlayerProjectileDef,
      entity_manager.RegisterComponent(&player_projectile_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_RenderMeshDef,
      entity_manager.RegisterComponent(&render_mesh_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_PhysicsDef,
      entity_manager.RegisterComponent(&physics_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_PatronDef,
      entity_manager.RegisterComponent(&patron_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_TimeLimitDef,
      entity_manager.RegisterComponent(&time_limit_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_ListenerDef,
      entity_manager.RegisterComponent(&audio_listener_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_SoundDef,
      entity_manager.RegisterComponent(&sound_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_RiverDef,
      entity_manager.RegisterComponent(&river_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_ShadowControllerDef,
      entity_manager.RegisterComponent(&shadow_controller_component));
  entity_factory.SetComponentId(
      ComponentDataUnion_EditorDef,
      entity_manager.RegisterComponent(&editor_component_));
  entity_factory.SetComponentId(
      ComponentDataUnion_TransformDef,
      entity_manager.RegisterComponent(&transform_component));

  services_component.LoadComponentDefBinarySchema(
      "flatbufferschemas/components.bfbs");
  entity_factory.SetEntityLibrary(kEntityLibraryFile);
  entity_manager.set_entity_factory(&entity_factory);
  // entity_factory.set_debug(true);

  render_mesh_component.set_light_position(vec3(-10, -20, 20));

  for (size_t i = 0; i < config->entity_files()->size(); i++) {
    const char* filename = config->entity_files()->Get(i)->c_str();
    LoadEntitiesFromFile(filename);
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
  TransformData* player_transform =
      transform_component.GetComponentData(player_entity);
  entity::EntityRef raft_entity = player_transform->parent;
  services_component.set_raft_entity(raft_entity);
}

}  // fpl_project
}  // fpl
