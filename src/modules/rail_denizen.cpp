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

#include "modules/rail_denizen.h"

#include <string>

#include "components/rail_denizen.h"
#include "entity/entity_manager.h"
#include "event/event_system.h"
#include "event/node_interface.h"

namespace fpl {
namespace fpl_project {

// Returns the rail denizen component data of the given entity.
class RailDenizenNode : public event::NodeInterface {
 public:
  RailDenizenNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<RailDenizenDataRef>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto entity = in->Get<entity::EntityRef>(0);
    out->Set(0, RailDenizenDataRef(rail_denizen_component_, *entity));
  }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

// Returns the lap value from the given rail denizen data.
class LapNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto rail_denizen_ref = in->Get<RailDenizenDataRef>(0);
    out->Set(0, rail_denizen_ref->GetComponentData()->lap);
  }
};

void InitializeRailDenizenModule(event::EventSystem* event_system,
                                 RailDenizenComponent* rail_denizen_component) {
  event::Module* module = event_system->AddModule("rail_denizen");
  auto rail_denizen_ctor = [rail_denizen_component]() {
    return new RailDenizenNode(rail_denizen_component);
  };
  module->RegisterNode<RailDenizenNode>("rail_denizen", rail_denizen_ctor);
  module->RegisterNode<LapNode>("lap");
}

}  // fpl_project
}  // fpl
