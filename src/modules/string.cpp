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

#include "modules/string.h"

#include <string>
#include <sstream>

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"

namespace fpl {
namespace fpl_project {

// Compares two strings.
class EqualsNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<std::string>();
    node_sig->AddInput<std::string>();
    node_sig->AddOutput<bool>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto str_a = args->GetInput<std::string>(0);
    auto str_b = args->GetInput<std::string>(1);
    args->SetOutput(0, *str_a == *str_b);
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }
};

// Converts the given int to a string.
class IntToStringNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<int>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto i = args->GetInput<int>(0);
    std::stringstream stream;
    stream << *i;
    args->SetOutput(0, stream.str());
  }
};

// Converts the given float to a string.
class FloatToStringNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<float>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto f = args->GetInput<float>(0);
    std::stringstream stream;
    stream << *f;
    args->SetOutput(0, stream.str());
  }
};

// Contactenates the given strings.
class ConcatNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<std::string>();
    node_sig->AddInput<std::string>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto str_a = args->GetInput<std::string>(0);
    auto str_b = args->GetInput<std::string>(1);
    args->SetOutput(0, *str_a + *str_b);
  }
};

void InitializeStringModule(breadboard::EventSystem* event_system) {
  breadboard::Module* module = event_system->AddModule("string");
  module->RegisterNode<EqualsNode>("equals");
  module->RegisterNode<IntToStringNode>("int_to_string");
  module->RegisterNode<FloatToStringNode>("float_to_string");
  module->RegisterNode<ConcatNode>("concat");
}

}  // fpl_project
}  // fpl
