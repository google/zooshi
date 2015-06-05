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

// This file contains utilities functions that are common to more than one
// event.

#ifndef FPL_EVENT_UTILITIES_H_
#define FPL_EVENT_UTILITIES_H_

#include "events_generated.h"

namespace fpl {
namespace fpl_project {

void ApplyOperation(float* value, Operation op, float operand);

}  // fpl_project
}  // fpl

#endif  // FPL_EVENT_UTILITIES_H_

