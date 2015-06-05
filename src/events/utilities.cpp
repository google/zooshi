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

#include "events/utilities.h"

namespace fpl {
namespace fpl_project {

void ApplyOperation(float* value, Operation op, float operand) {
  switch (op) {
    case Operation_Set: {
      *value = operand;
      break;
    }
    case Operation_Add: {
      *value += operand;
      break;
    }
    case Operation_Multiply: {
      *value *= operand;
      break;
    }
    default: { assert(0); }
  }
}

}  // fpl_project
}  // fpl

