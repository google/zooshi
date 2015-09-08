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

#include "modules/vec3.h"

#include "event/event_system.h"
#include "event/base_node.h"
#include "mathfu/glsl_mappings.h"

using mathfu::vec3;

namespace fpl {
namespace fpl_project {

// Creates a vector from 3 floats.
class Vec3Node : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<float>();
    node_sig->AddInput<float>();
    node_sig->AddInput<float>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto x = in->Get<float>(0);
    auto y = in->Get<float>(1);
    auto z = in->Get<float>(2);
    out->Set(0, vec3(*x, *y, *z));
  }
};

// Returns the individual elements of the given vector.
class ElementsNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<float>();
    node_sig->AddOutput<float>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec = in->Get<vec3>(0);
    out->Set(0, vec->x());
    out->Set(1, vec->y());
    out->Set(2, vec->z());
  }
};

// Adds the two given vectors.
class AddNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec_a = in->Get<vec3>(0);
    auto vec_b = in->Get<vec3>(1);
    out->Set(0, *vec_a + *vec_b);
  }
};

// Subtracts the two given vectors.
class SubtractNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec_a = in->Get<vec3>(0);
    auto vec_b = in->Get<vec3>(1);
    out->Set(0, *vec_a - *vec_b);
  }
};

class ScalarMultiplyNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<float>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec = in->Get<vec3>(0);
    auto scalar = in->Get<float>(1);
    out->Set(0, *vec * *scalar);
  }
};

class ScalarDivideNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<float>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec = in->Get<vec3>(0);
    auto scalar = in->Get<float>(1);
    out->Set(0, *vec / *scalar);
  }
};

class CrossProductNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec_a = in->Get<vec3>(0);
    auto vec_b = in->Get<vec3>(1);
    out->Set(0, vec3::CrossProduct(*vec_a, *vec_b));
  }
};

class DotProductNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<vec3>();
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto vec_a = in->Get<vec3>(0);
    auto vec_b = in->Get<vec3>(1);
    out->Set(0, vec3::DotProduct(*vec_a, *vec_b));
  }
};

void InitializeVec3Module(event::EventSystem* event_system) {
  event::Module* module = event_system->AddModule("vec3");
  module->RegisterNode<Vec3Node>("vec3");
  module->RegisterNode<ElementsNode>("elements");
  module->RegisterNode<AddNode>("add");
  module->RegisterNode<SubtractNode>("subtract");
  module->RegisterNode<ScalarMultiplyNode>("scalar_multiply");
  module->RegisterNode<ScalarDivideNode>("scalar_divide");
  module->RegisterNode<CrossProductNode>("cross_product");
  module->RegisterNode<DotProductNode>("dot_product");
}

}  // fpl_project
}  // fpl
