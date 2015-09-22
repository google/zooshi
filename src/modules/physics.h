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

#ifndef FPL_ZOOSHI_MODULES_PHYSICS_H_
#define FPL_ZOOSHI_MODULES_PHYSICS_H_

#include "breadboard/event_system.h"
#include "component_library/physics.h"
#include "modules/entity.h"

namespace fpl {
namespace fpl_project {

void InitializePhysicsModule(
    breadboard::EventSystem* event_system,
    fpl::component_library::PhysicsComponent* physics_component);

}  // fpl_project
}  // fpl

#endif  // FPL_ZOOSHI_MODULES_PHYSICS_H_
