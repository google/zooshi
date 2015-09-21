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

#include "modules/state.h"
#include "event/event_system.h"
#include "event/base_node.h"

namespace fpl {
namespace fpl_project {

// Sets an integer that informs the state machine what state it should
// transition to next.
class RequestStateNode : public event::BaseNode {
 public:
  RequestStateNode(int* state) : state_(state) {}

  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<int>();
  }

  virtual void Execute(event::NodeArguments* args) {
    auto new_state = args->GetInput<int>(1);
    *state_ = *new_state;
  }

 private:
  int* state_;
};

void InitializeStateModule(event::EventSystem* event_system, int* state) {
  auto request_node_ctor = [state]() {
    return new RequestStateNode(state);
  };
  event::Module* module = event_system->AddModule("game_state");
  module->RegisterNode<RequestStateNode>("request_state", request_node_ctor);
}

}  // fpl_project
}  // fpl
