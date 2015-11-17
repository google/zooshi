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

#include "modules/player.h"

#include <string>

#include "component_library/meta.h"
#include "entity/entity_manager.h"
#include "breadboard/module_registry.h"
#include "breadboard/base_node.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using corgi::component_library::GraphComponent;

namespace fpl {
namespace zooshi {

// Number of patron types.
const int32_t kPatronTypes = 6;

// Fires a pulse whenever a on_fire has been started.
class OnFireNode : public BaseNode {
 public:
  OnFireNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}
  virtual ~OnFireNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<corgi::EntityRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kOnFireEventId);
  }

  virtual void Initialize(NodeArguments* args) {
    // Bind the node to player entity's broadcaster.
    auto entity = args->GetInput<corgi::EntityRef>(0);
    args->BindBroadcaster(0, graph_component_->GetCreateBroadcaster(*entity));
  }

  virtual void Execute(NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

// Store a patron's feeding status for a player.
class FedPatronNode : public BaseNode {
 public:
  FedPatronNode(PlayerComponent* player_component)
      : player_component_(player_component) {}
  virtual ~FedPatronNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<bool>();
    node_sig->AddInput<corgi::EntityRef>();  // Player entity.
    node_sig->AddInput<corgi::EntityRef>();  // Collision target.
    node_sig->AddOutput<void>();
  }

  virtual void Execute(NodeArguments* args) {
    auto hit = args->GetInput<bool>(0);
    if (*hit) {
      auto entity = args->GetInput<corgi::EntityRef>(1);
      auto player_data = player_component_->GetComponentData(*entity);

      auto target_entity = args->GetInput<corgi::EntityRef>(2);
      auto target_data = player_component_->
        Data<corgi::component_library::MetaData>(*target_entity);
      if (player_data) {
        auto id = target_data->prototype;
        player_data->get_patrons_feed_status().insert(id);
      }
    }
    args->SetOutput(0);
  }

 private:
  PlayerComponent* player_component_;
};

// Check a patron's feeding status for a player.
class CheckAllPatronsFedNode : public BaseNode {
 public:
  CheckAllPatronsFedNode(PlayerComponent* player_component)
      : player_component_(player_component) {}
  virtual ~CheckAllPatronsFedNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<corgi::EntityRef>();
    node_sig->AddOutput<int32_t>();
  }

  virtual void Execute(NodeArguments* args) {
    int32_t ret = 0;
    auto entity = args->GetInput<corgi::EntityRef>(0);
    auto player_data = player_component_->GetComponentData(*entity);
    if (player_data) {
      if (player_data->get_patrons_feed_status().size() >= kPatronTypes) {
        ret = 1;
      }
    }
    args->SetOutput(0, ret);
  }

 private:
  PlayerComponent* player_component_;
};

void InitializePlayerModule(ModuleRegistry* module_registry,
                            PlayerComponent* player_component,
                            GraphComponent* graph_component) {
  Module* module = module_registry->RegisterModule("player");
  auto on_fire_ctor = [graph_component]() {
    return new OnFireNode(graph_component);
  };
  module->RegisterNode<OnFireNode>("on_fire", on_fire_ctor);

  auto fed_patron_ctor = [player_component]() {
    return new FedPatronNode(player_component);
  };
  module->RegisterNode<FedPatronNode>("fed_patron", fed_patron_ctor);

  auto check_all_patrons_fed_ctor = [player_component]() {
    return new CheckAllPatronsFedNode(player_component);
  };
  module->RegisterNode<CheckAllPatronsFedNode>("check_all_patrons_fed",
                                               check_all_patrons_fed_ctor);
}

}  // zooshi
}  // fpl
