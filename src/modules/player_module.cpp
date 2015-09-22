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

#include "modules/player_module.h"

#include <string>

#include "entity/entity_manager.h"
#include "breadboard/event_system.h"
#include "breadboard/base_node.h"

namespace fpl {
namespace fpl_project {

// Fires a pulse whenever a on_fire has been started.
class OnFireNode : public breadboard::BaseNode {
 public:
  OnFireNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kOnFireEventId);
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    // Bind the node to player entity's broadcaster.
    auto entity = args->GetInput<entity::EntityRef>(0);
    args->BindBroadcaster(
        0, graph_component_->GetCreateBroadcaster(*entity));
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

void InitializePlayerModule(breadboard::EventSystem* event_system,
                            GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("player");
  auto on_fire_ctor = [graph_component]() {
    return new OnFireNode(graph_component);
  };
  module->RegisterNode<OnFireNode>("on_fire", on_fire_ctor);
}

}  // fpl_project
}  // fpl
