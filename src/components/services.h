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

#ifndef COMPONENTS_SERVICES_H_
#define COMPONENTS_SERVICES_H_

#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "event/event_manager.h"
#include "imgui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "motive/engine.h"
#include "pindrop/pindrop.h"
#include "railmanager.h"
#include "zooshi_entity_factory.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct ServicesData {};

// This is a somewhat unique component - No entities will directly subscribe
// to it, and it has no per-entity data.  However, it provides an easy place
// for other components to access game services and managers.  (Since components
// don't have direct access to the gamestate, but they do have access to other
// components.)
class ServicesComponent : public entity::Component<ServicesData> {
 public:
  ServicesComponent() {}

  void Initialize(const Config* config, AssetManager* asset_manager,
                  InputSystem* input_system, pindrop::AudioEngine* audio_engine,
                  motive::MotiveEngine* motive_engine,
                  event::EventManager* event_manager, FontManager* font_manager,
                  RailManager* rail_manager,
                  ZooshiEntityFactory* zooshi_entity_factory) {
    config_ = config;
    asset_manager_ = asset_manager;
    input_system_ = input_system;
    audio_engine_ = audio_engine;
    motive_engine_ = motive_engine;
    event_manager_ = event_manager;
    font_manager_ = font_manager;
    rail_manager_ = rail_manager;
    zooshi_entity_factory_ = zooshi_entity_factory;
  }

  const Config* config() { return config_; }
  AssetManager* asset_manager() { return asset_manager_; }
  pindrop::AudioEngine* audio_engine() { return audio_engine_; }
  motive::MotiveEngine* motive_engine() { return motive_engine_; }
  event::EventManager* event_manager() { return event_manager_; }
  FontManager* font_manager() { return font_manager_; }
  InputSystem* input_system() { return input_system_; }
  RailManager* rail_manager() { return rail_manager_; }
  entity::EntityRef raft_entity() { return raft_entity_; }
  void set_raft_entity(entity::EntityRef entity) { raft_entity_ = entity; }
  ZooshiEntityFactory* zooshi_entity_factory() {
    return zooshi_entity_factory_;
  }
  const void* component_def_binary_schema() const {
    if (component_def_binary_schema_ == "") {
      LogInfo(
          "ServicesComponent: ComponentDef binary schema not yet loaded, did "
          "you call LoadComponentDefBinarySchema()?");
      return nullptr;
    }
    return component_def_binary_schema_.c_str();
  }
  // This component should never be added to an entity.  It is only provided
  // as an interface for other components to access common resources.
  void AddFromRawData(entity::EntityRef& /*entity*/, const void* /*raw_data*/) {
    assert(false);
  }
  void LoadComponentDefBinarySchema(const char* filename) {
    if (!LoadFile(filename, &component_def_binary_schema_)) {
      LogInfo("Couldn't load ComponentDef binary schema from %s", filename);
    }
  }

 private:
  const Config* config_;

  AssetManager* asset_manager_;
  motive::MotiveEngine* motive_engine_;
  pindrop::AudioEngine* audio_engine_;
  event::EventManager* event_manager_;
  InputSystem* input_system_;
  FontManager* font_manager_;
  RailManager* rail_manager_;
  entity::EntityRef raft_entity_;
  ZooshiEntityFactory* zooshi_entity_factory_;
  std::string component_def_binary_schema_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::ServicesComponent,
                              fpl::fpl_project::ServicesData)

#endif  // COMPONENTS_SERVICES_H_
