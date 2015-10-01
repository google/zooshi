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

#include "modules/entity.h"

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/meta.h"
#include "components/services.h"
#include "entity/entity_manager.h"
#include "mathfu/glsl_mappings.h"

using fpl::entity::EntityRef;
using fpl::entity::EntityManager;
using fpl::component_library::GraphComponent;
using fpl::component_library::MetaComponent;

namespace fpl {
namespace fpl_project {

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

// Given an input string, return the named entity.
class EntityNode : public breadboard::BaseNode {
 public:
  EntityNode(MetaComponent* meta_component) : meta_component_(meta_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<std::string>();
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto entity_id = args->GetInput<std::string>(0);
    EntityRef entity =
        meta_component_->GetEntityFromDictionary(entity_id->c_str());
    assert(entity.IsValid());
    args->SetOutput(0, entity);
  }

 private:
  MetaComponent* meta_component_;
};

// Return the entity that owns this graph.
class GraphEntityNode : public breadboard::BaseNode {
 public:
  GraphEntityNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    args->SetOutput(0, graph_component_->graph_entity());
  }

 private:
  GraphComponent* graph_component_;
};

// Delete the given entity.
class DeleteEntityNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<EntityRef>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto entity_ref = args->GetInput<EntityRef>(0);
    entity_manager_->DeleteEntity(*entity_ref);
  }

 private:
  EntityManager* entity_manager_;
};

void InitializeEntityModule(breadboard::EventSystem* event_system,
                            ServicesComponent* services_component,
                            MetaComponent* meta_component,
                            GraphComponent* graph_component) {
  auto entity_ctor = [meta_component]() {
    return new EntityNode(meta_component);
  };
  auto graph_entity_ctor = [graph_component]() {
    return new GraphEntityNode(graph_component);
  };
  auto player_entity_ctor = [services_component]() {
    return new PlayerEntityNode(services_component);
  };
  auto raft_entity_ctor = [services_component]() {
    return new RaftEntityNode(services_component);
  };
  breadboard::Module* module = event_system->AddModule("entity");
  module->RegisterNode<EntityNode>("entity", entity_ctor);
  module->RegisterNode<GraphEntityNode>("graph_entity", graph_entity_ctor);
  module->RegisterNode<PlayerEntityNode>("player_entity", player_entity_ctor);
  module->RegisterNode<RaftEntityNode>("raft_entity", raft_entity_ctor);
}

}  // fpl_project
}  // fpl
