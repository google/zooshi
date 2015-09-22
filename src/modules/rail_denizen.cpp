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
#include "breadboard/event_system.h"
#include "components/rail_denizen.h"
#include "entity/entity_manager.h"

namespace fpl {
namespace fpl_project {

// Returns the rail denizen component data of the given entity.
class RailDenizenNode : public breadboard::BaseNode {
 public:
  RailDenizenNode(RailDenizenComponent* rail_denizen_component)
      : rail_denizen_component_(rail_denizen_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<RailDenizenDataRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto entity = args->GetInput<entity::EntityRef>(1);
    args->SetOutput(0, RailDenizenDataRef(rail_denizen_component_, *entity));
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }

 private:
  RailDenizenComponent* rail_denizen_component_;
};

// Returns the lap value from the given rail denizen data.
class LapNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(1);
    args->SetOutput(0, rail_denizen_ref->GetComponentData()->lap);
  }
};

// Fires a pulse whenever a new lap has been started.
class NewLapNode : public breadboard::BaseNode {
 public:
  NewLapNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kNewLapEventId);
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(0);
    args->BindBroadcaster(
        0, graph_component_->GetCreateBroadcaster(rail_denizen_ref->entity()));
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    Initialize(args);
    args->SetOutput(0);
  }

 private:
  GraphComponent* graph_component_;
};

// Sets the rail denizen's speed.
class GetRailSpeedNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(0);
    args->SetOutput(0,
                    rail_denizen_ref->GetComponentData()->spline_playback_rate);
  }
};

// Sets the rail denizen's speed.
class SetRailSpeedNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<RailDenizenDataRef>();
    node_sig->AddInput<float>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto rail_denizen_ref = args->GetInput<RailDenizenDataRef>(1);
    auto speed = args->GetInput<float>(2);
    RailDenizenData* rail_denizen_data = rail_denizen_ref->GetComponentData();
    rail_denizen_data->spline_playback_rate = *speed;
    rail_denizen_data->motivator.SetSplinePlaybackRate(
        rail_denizen_data->spline_playback_rate);
  }
};

void InitializeRailDenizenModule(breadboard::EventSystem* event_system,
                                 RailDenizenComponent* rail_denizen_component,
                                 GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("rail_denizen");
  auto rail_denizen_ctor = [rail_denizen_component]() {
    return new RailDenizenNode(rail_denizen_component);
  };
  auto new_lap_ctor = [graph_component]() {
    return new NewLapNode(graph_component);
  };
  module->RegisterNode<RailDenizenNode>("rail_denizen", rail_denizen_ctor);
  module->RegisterNode<LapNode>("lap");
  module->RegisterNode<NewLapNode>("new_lap", new_lap_ctor);
  module->RegisterNode<SetRailSpeedNode>("set_rail_speed");
  module->RegisterNode<GetRailSpeedNode>("get_rail_speed");
}

}  // fpl_project
}  // fpl
