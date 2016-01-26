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

#ifndef FPL_ZOOSHI_COMPONENTS_SERVICES_H_
#define FPL_ZOOSHI_COMPONENTS_SERVICES_H_

#include "camera.h"
#include "components_generated.h"
#include "config_generated.h"
#include "corgi/component.h"
#include "corgi_component_library/entity_factory.h"
#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "motive/engine.h"
#include "pindrop/pindrop.h"
#include "railmanager.h"
#include "scene_lab/scene_lab.h"

namespace fpl {
namespace zooshi {

// This needs to be forward declared here, as the World uses this as well.
struct World;

// Data for scene object components.
struct ServicesData {};

// This is a somewhat unique component - No entities will directly subscribe
// to it, and it has no per-entity data.  However, it provides an easy place
// for other components to access game services and managers.  (Since
// components
// don't have direct access to the gamestate, but they do have access to other
// components.)
class ServicesComponent : public corgi::Component<ServicesData> {
 public:
  ServicesComponent() {}

  void Initialize(const Config* config, fplbase::AssetManager* asset_manager,
                  fplbase::InputSystem* input_system,
                  pindrop::AudioEngine* audio_engine,
                  flatui::FontManager* font_manager, RailManager* rail_manager,
                  corgi::component_library::EntityFactory* entity_factory,
                  World* world, scene_lab::SceneLab* scene_lab) {
    config_ = config;
    asset_manager_ = asset_manager;
    input_system_ = input_system;
    audio_engine_ = audio_engine;
    font_manager_ = font_manager;
    rail_manager_ = rail_manager;
    entity_factory_ = entity_factory;
    world_ = world;
    scene_lab_ = scene_lab;
    // The camera is set seperately dependent on the game state.
    camera_ = nullptr;
  }

  const Config* config() { return config_; }
  fplbase::AssetManager* asset_manager() { return asset_manager_; }
  pindrop::AudioEngine* audio_engine() { return audio_engine_; }
  flatui::FontManager* font_manager() { return font_manager_; }
  fplbase::InputSystem* input_system() { return input_system_; }
  RailManager* rail_manager() { return rail_manager_; }
  corgi::EntityRef raft_entity() { return raft_entity_; }
  void set_raft_entity(corgi::EntityRef entity) { raft_entity_ = entity; }
  corgi::EntityRef player_entity() { return player_entity_; }
  void set_player_entity(corgi::EntityRef entity) { player_entity_ = entity; }
  corgi::component_library::EntityFactory* entity_factory() {
    return entity_factory_;
  }
  World* world() { return world_; }
  // Scene Lab is not guaranteed to be present in all versions of the game.
  scene_lab::SceneLab* scene_lab() { return scene_lab_; }
  void set_camera(Camera* camera) { camera_ = camera; }
  Camera* camera() { return camera_; }

  const void* component_def_binary_schema() const {
    if (component_def_binary_schema_ == "") {
      fplbase::LogInfo(
          "ServicesComponent: ComponentDef binary schema not yet loaded, did "
          "you call LoadComponentDefBinarySchema()?");
      return nullptr;
    }
    return component_def_binary_schema_.c_str();
  }
  // This component should never be added to an entity.  It is only provided
  // as an interface for other components to access common resources.
  void AddFromRawData(corgi::EntityRef& /*entity*/, const void* /*raw_data*/) {
    assert(false);
  }
  void LoadComponentDefBinarySchema(const char* filename) {
    if (!fplbase::LoadFile(filename, &component_def_binary_schema_)) {
      fplbase::LogInfo("Couldn't load ComponentDef binary schema from %s",
                       filename);
    }
  }

 private:
  const Config* config_;

  fplbase::AssetManager* asset_manager_;
  pindrop::AudioEngine* audio_engine_;
  fplbase::InputSystem* input_system_;
  flatui::FontManager* font_manager_;
  RailManager* rail_manager_;
  corgi::EntityRef raft_entity_;
  corgi::EntityRef player_entity_;
  corgi::component_library::EntityFactory* entity_factory_;
  std::string component_def_binary_schema_;
  World* world_;
  scene_lab::SceneLab* scene_lab_;
  Camera* camera_;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::ServicesComponent,
                         fpl::zooshi::ServicesData)

#endif  // FPL_ZOOSHI_COMPONENTS_SERVICES_H_
