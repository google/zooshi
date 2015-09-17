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

#include "modules/logic.h"
#include "event/event_system.h"
#include "event/base_node.h"

namespace fpl {
namespace fpl_project {

static void SetBooleanEdges(event::NodeArguments* args, bool value,
                            int bool_index, int true_index, int false_index) {
  args->SetOutput(bool_index, value);
  if (value) {
    args->SetOutput(true_index);
  } else {
    args->SetOutput(false_index);
  }
}

// All logical nodes have three outputs, the boolean result of the logical
// operation, and two void outputs which carry no data. The first one is
// triggered when the result evaluates true, the second is triggered when the
// result evaluates false.
//
// clang-format off
#define LOGICAL_NODE(name, op)                               \
  class name : public event::BaseNode {                      \
   public:                                                   \
    static void OnRegister(event::NodeSignature* node_sig) { \
      node_sig->AddInput<bool>();                            \
      node_sig->AddInput<bool>();                            \
      node_sig->AddOutput<bool>();                           \
      node_sig->AddOutput<void>();                           \
      node_sig->AddOutput<void>();                           \
    }                                                        \
                                                             \
    virtual void Execute(event::NodeArguments* args) {       \
      auto a = args->GetInput<bool>(0);                      \
      auto b = args->GetInput<bool>(1);                      \
      bool result = *a op *b;                                \
      SetBooleanEdges(args, result, 0, 1, 2);                \
    }                                                        \
  }

LOGICAL_NODE(AndNode, &&);
LOGICAL_NODE(OrNode, ||);
LOGICAL_NODE(XorNode, ^);
// clang-format on

// Logical Not.
class NotNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<bool>();
    node_sig->AddOutput<bool>();
    node_sig->AddOutput<void>();
    node_sig->AddOutput<void>();
  }

  virtual void Execute(event::NodeArguments* args) {
    auto a = args->GetInput<bool>(0);
    bool result = !*a;
    SetBooleanEdges(args, result, 0, 1, 2);
  }
};

void InitializeLogicModule(event::EventSystem* event_system) {
  event::Module* module = event_system->AddModule("logic");
  module->RegisterNode<AndNode>("and");
  module->RegisterNode<OrNode>("or");
  module->RegisterNode<XorNode>("xor");
  module->RegisterNode<NotNode>("not");
}

}  // fpl_project
}  // fpl
