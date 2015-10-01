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

using fpl::component_library::GraphComponent;

namespace fpl {
namespace fpl_project {

// Number of patron types.
const int32_t kPatronTypes = 6;

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
    args->BindBroadcaster(0, graph_component_->GetCreateBroadcaster(*entity));
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

// Store a patron's feeding status for a player.
class FedPatronNode : public breadboard::BaseNode {
 public:
  FedPatronNode(PlayerComponent* player_component)
      : player_component_(player_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<bool>();
    node_sig->AddInput<entity::EntityRef>();  // Player entity.
    node_sig->AddInput<entity::EntityRef>();  // Collision target.
    node_sig->AddOutput<void>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto hit = args->GetInput<bool>(0);
    if (*hit) {
      auto entity = args->GetInput<entity::EntityRef>(1);
      auto player_data = player_component_->GetComponentData(*entity);

      auto target_entity = args->GetInput<entity::EntityRef>(2);
      auto target_data =
          player_component_->Data<component_library::MetaData>(*target_entity);
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
class CheckAllPatronsFedNode : public breadboard::BaseNode {
 public:
  CheckAllPatronsFedNode(PlayerComponent* player_component)
      : player_component_(player_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<int32_t>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    int32_t ret = 0;
    auto entity = args->GetInput<entity::EntityRef>(0);
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

void InitializePlayerModule(breadboard::EventSystem* event_system,
                            PlayerComponent* player_component,
                            GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("player");
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

}  // fpl_project
}  // fpl
