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

#include "breadboard/base_node.h"
#include "breadboard/module_registry.h"
#include "components/rail_denizen.h"
#include "entity/entity_manager.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using corgi::component_library::GraphComponent;
using corgi::EntityRef;

namespace fpl {
namespace zooshi {

// Fires a pulse whenever a new lap has been started.
class NewLapNode : public BaseNode {
 public:
  NewLapNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<EntityRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kNewLapEventId);
  }

  virtual void Initialize(NodeArguments* args) {
    auto entity = args->GetInput<EntityRef>(0);
    args->BindBroadcaster(0, graph_component_->GetCreateBroadcaster(*entity));
  }

  virtual void Execute(NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

// Returns the lap value from the given rail denizen data.
class GetLapNode : public BaseNode {
 public:
  GetLapNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      auto entity = args->GetInput<EntityRef>(1);
      auto rail_denizen_data =
          rail_denizen_component_->GetComponentData(*entity);
      args->SetOutput(0, rail_denizen_data->total_lap_progress);
    }
  }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

// Sets the rail denizen's speed.
class GetRailSpeedNode : public BaseNode {
 public:
  GetRailSpeedNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      auto entity = args->GetInput<EntityRef>(1);
      auto rail_denizen_data =
          rail_denizen_component_->GetComponentData(*entity);
      args->SetOutput(0, rail_denizen_data->PlaybackRate());
    }
  }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

// Sets the rail denizen's speed.
class SetRailSpeedNode : public BaseNode {
 public:
  SetRailSpeedNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddInput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      // TODO: Add this as a parameter instead of being constant.
      static const float kSetRailSpeedTransitionTime = 300.0f;
      auto entity = args->GetInput<EntityRef>(1);
      auto speed = args->GetInput<float>(2);
      RailDenizenData* rail_denizen_data =
          rail_denizen_component_->GetComponentData(*entity);
      rail_denizen_data->SetPlaybackRate(*speed, kSetRailSpeedTransitionTime);
    }
  }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

void InitializeRailDenizenModule(ModuleRegistry* module_registry,
                                 RailDenizenComponent* rail_denizen_component,
                                 GraphComponent* graph_component) {
  auto new_lap_ctor = [graph_component]() {
    return new NewLapNode(graph_component);
  };
  auto get_lap_ctor = [rail_denizen_component]() {
    return new GetLapNode(rail_denizen_component);
  };
  auto set_rail_speed_ctor = [rail_denizen_component]() {
    return new SetRailSpeedNode(rail_denizen_component);
  };
  auto get_rail_speed_ctor = [rail_denizen_component]() {
    return new GetRailSpeedNode(rail_denizen_component);
  };
  Module* module = module_registry->RegisterModule("rail_denizen");
  module->RegisterNode<NewLapNode>("new_lap", new_lap_ctor);
  module->RegisterNode<GetLapNode>("get_lap", get_lap_ctor);
  module->RegisterNode<SetRailSpeedNode>("set_rail_speed", set_rail_speed_ctor);
  module->RegisterNode<GetRailSpeedNode>("get_rail_speed", get_rail_speed_ctor);
}

}  // zooshi
}  // fpl
