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
#include "event_system/event_manager.h"
#include "fplbase/input.h"
#include "fplbase/material_manager.h"
#include "pindrop/pindrop.h"

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

  void Initialize(const Config* config, MaterialManager* material_manager,
                  pindrop::AudioEngine* audio_engine,
                  event::EventManager* event_manager) {
    config_ = config;
    material_manager_ = material_manager;
    audio_engine_ = audio_engine;
    event_manager_ = event_manager;
  }


  const Config* config() { return config_; }
  MaterialManager* material_manager() { return material_manager_; }
  pindrop::AudioEngine* audio_engine() { return audio_engine_; }
  event::EventManager* event_manager() { return event_manager_; }

  // This component should never be added to an entity.  It is only provided
  // as an interface for other components to access common resources.
  void AddFromRawData(entity::EntityRef& /*entity*/,
                                         const void* /*raw_data*/) {
    assert(false);
  }

 private:
  MaterialManager* material_manager_;
  pindrop::AudioEngine* audio_engine_;
  event::EventManager* event_manager_;
  const Config* config_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::ServicesComponent,
                              fpl::fpl_project::ServicesData,
                              ComponentDataUnion_ServicesDef)

#endif  // COMPONENTS_SERVICES_H_
