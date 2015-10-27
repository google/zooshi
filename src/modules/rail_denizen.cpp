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
using breadboard::TypeRegistry;
using fpl::component_library::GraphComponent;

namespace fpl {
namespace zooshi {

// Returns the rail denizen component data of the given entity.
class RailDenizenNode : public BaseNode {
 public:
  RailDenizenNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<RailDenizenDataRef>();
  }

  virtual void Initialize(NodeArguments* args) {
    auto entity = args->GetInput<entity::EntityRef>(1);
    args->SetOutput(0, RailDenizenDataRef(rail_denizen_component_, *entity));
  }

  virtual void Execute(NodeArguments* args) { Initialize(args); }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

// Returns the lap value from the given rail denizen data.
class LapNode : public BaseNode {
 public:
  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(1);
    args->SetOutput(0, rail_denizen_ref->GetComponentData()->lap);
  }
};

// Fires a pulse whenever a new lap has been started.
class NewLapNode : public BaseNode {
 public:
  NewLapNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kNewLapEventId);
  }

  virtual void Initialize(NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(0);
    args->BindBroadcaster(
        0, graph_component_->GetCreateBroadcaster(rail_denizen_ref->entity()));
  }

  virtual void Execute(NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

// Sets the rail denizen's speed.
class GetRailSpeedNode : public BaseNode {
 public:
  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(0);
    args->SetOutput(
        0, rail_denizen_ref->GetComponentData()->PlaybackRate());
  }
};

// Sets the rail denizen's speed.
class SetRailSpeedNode : public BaseNode {
 public:
  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddInput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      // TODO: Add this as a parameter instead of being constant.
      static const float kSetRailSpeedTransitionTime = 300.0f;
      auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(1);
      auto speed = args->GetInput<float>(2);
      RailDenizenData* rail_denizen_data = rail_denizen_ref->GetComponentData();
      rail_denizen_data->SetPlaybackRate(*speed, kSetRailSpeedTransitionTime);
    }
  }
};

void InitializeRailDenizenModule(ModuleRegistry* module_registry,
                                 RailDenizenComponent* rail_denizen_component,
                                 GraphComponent* graph_component) {
  auto rail_denizen_ctor = [rail_denizen_component]() {
    return new RailDenizenNode(rail_denizen_component);
  };
  auto new_lap_ctor = [graph_component]() {
    return new NewLapNode(graph_component);
  };
  TypeRegistry<RailDenizenDataRef>::RegisterType("RailDenizenData");
  Module* module = module_registry->RegisterModule("rail_denizen");
  module->RegisterNode<RailDenizenNode>("rail_denizen", rail_denizen_ctor);
  module->RegisterNode<LapNode>("lap");
  module->RegisterNode<NewLapNode>("new_lap", new_lap_ctor);
  module->RegisterNode<SetRailSpeedNode>("set_rail_speed");
  module->RegisterNode<GetRailSpeedNode>("get_rail_speed");
}

}  // zooshi
}  // fpl
