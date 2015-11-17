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
#include "breadboard/event.h"
#include "breadboard/module_registry.h"
#include "components/scenery.h"
#include "components/services.h"
#include "corgi/entity_manager.h"
#include "corgi_component_library/animation.h"
#include "corgi_component_library/graph.h"
#include "corgi_component_library/transform.h"
#include "mathfu/glsl_mappings.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using corgi::component_library::AnimationComponent;
using corgi::component_library::GraphComponent;
using corgi::component_library::TransformComponent;
using corgi::EntityManager;
using corgi::EntityRef;

namespace fpl {
namespace zooshi {

// Returns the entity representing the player.
class PlayerEntityNode : public BaseNode {
 public:
  PlayerEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}
  virtual ~PlayerEntityNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(NodeArguments* args) {
    args->SetOutput(0, services_component_->player_entity());
  }

 private:
  ServicesComponent* services_component_;
};

// Returns the entity representing the raft.
class RaftEntityNode : public BaseNode {
 public:
  RaftEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}
  virtual ~RaftEntityNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(NodeArguments* args) {
    args->SetOutput(0, services_component_->raft_entity());
  }

 private:
  ServicesComponent* services_component_;
};

// A node that executes every frame.
class AdvanceFrameNode : public BaseNode {
 public:
  AdvanceFrameNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}
  virtual ~AdvanceFrameNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddOutput<void>();
    node_sig->AddListener(corgi::component_library::kAdvanceFrameEventId);
  }

  virtual void Initialize(NodeArguments* args) {
    args->BindBroadcaster(0, graph_component_->advance_frame_broadcaster());
  }

  virtual void Execute(NodeArguments* args) { args->SetOutput(0); }

 private:
  GraphComponent* graph_component_;
};

// Sets the override animation to play in the show state.
class SetShowOverrideNode : public BaseNode {
 public:
  SetShowOverrideNode(SceneryComponent* scenery_component)
      : scenery_component_(scenery_component) {}
  virtual ~SetShowOverrideNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();       // Void to trigger the animation.
    node_sig->AddInput<EntityRef>();  // The entity to be animated.
    node_sig->AddInput<int>();        // The index into the AnimTable.
    node_sig->AddOutput<void>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      EntityRef entity = *args->GetInput<EntityRef>(1);
      int override_idx = *args->GetInput<int>(2);
      scenery_component_->ApplyShowOverride(
          entity, static_cast<SceneryState>(override_idx));
      args->SetOutput(0);
    }
  }

 private:
  SceneryComponent* scenery_component_;
};

void InitializeZooshiModule(ModuleRegistry* module_registry,
                            ServicesComponent* services_component,
                            GraphComponent* graph_component,
                            SceneryComponent* scenery_component) {
  auto player_entity_ctor = [services_component]() {
    return new PlayerEntityNode(services_component);
  };
  auto raft_entity_ctor = [services_component]() {
    return new RaftEntityNode(services_component);
  };
  auto advance_frame_ctor = [graph_component]() {
    return new AdvanceFrameNode(graph_component);
  };
  auto set_show_override_ctor = [scenery_component]() {
    return new SetShowOverrideNode(scenery_component);
  };
  Module* module = module_registry->RegisterModule("zooshi");
  module->RegisterNode<PlayerEntityNode>("player_entity", player_entity_ctor);
  module->RegisterNode<RaftEntityNode>("raft_entity", raft_entity_ctor);
  module->RegisterNode<AdvanceFrameNode>("advance_frame", advance_frame_ctor);
  module->RegisterNode<SetShowOverrideNode>("set_show_override",
                                            set_show_override_ctor);
}

}  // zooshi
}  // fpl
