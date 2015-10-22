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

#include "modules/zooshi.h"

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/meta.h"
#include "components/services.h"
#include "entity/entity_manager.h"
#include "mathfu/glsl_mappings.h"

using fpl::entity::EntityRef;
using fpl::entity::EntityManager;

namespace fpl {
namespace zooshi {

// Returns the entity representing the player.
class PlayerEntityNode : public breadboard::BaseNode {
 public:
  PlayerEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->player_entity());
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }

 private:
  ServicesComponent* services_component_;
};

// Returns the entity representing the raft.
class RaftEntityNode : public breadboard::BaseNode {
 public:
  RaftEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->raft_entity());
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }

 private:
  ServicesComponent* services_component_;
};

void InitializeZooshiModule(breadboard::EventSystem* event_system,
                            ServicesComponent* services_component) {
  auto player_entity_ctor = [services_component]() {
    return new PlayerEntityNode(services_component);
  };
  auto raft_entity_ctor = [services_component]() {
    return new RaftEntityNode(services_component);
  };
  breadboard::Module* module = event_system->AddModule("zooshi");
  module->RegisterNode<PlayerEntityNode>("player_entity", player_entity_ctor);
  module->RegisterNode<RaftEntityNode>("raft_entity", raft_entity_ctor);
}

}  // zooshi
}  // fpl
