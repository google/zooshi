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

#include "event/event_system.h"
#include "event/base_node.h"

namespace fpl {
namespace fpl_project {

// Converts the given int to a string.
class IntToStringNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<int>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto i = in->Get<int>(0);
    std::stringstream stream;
    stream << *i;
    out->Set(0, stream.str());
  }
};

// Converts the given float to a string.
class FloatToStringNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<float>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto f = in->Get<float>(0);
    std::stringstream stream;
    stream << *f;
    out->Set(0, stream.str());
  }
};

// Contactenates the given strings.
class ConcatNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<std::string>();
    node_sig->AddInput<std::string>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto str_a = in->Get<std::string>(0);
    auto str_b = in->Get<std::string>(1);
    out->Set(0, *str_a + *str_b);
  }
};

void InitializeStringModule(event::EventSystem* event_system) {
  event::Module* module = event_system->AddModule("string");
  module->RegisterNode<IntToStringNode>("int_to_string");
  module->RegisterNode<FloatToStringNode>("float_to_string");
  module->RegisterNode<ConcatNode>("concat");
}

}  // fpl_project
}  // fpl
