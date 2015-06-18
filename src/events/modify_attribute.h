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

#ifndef FPL_EVENTS_MODIFY_ATTRIBUTE_EVENT_H_
#define FPL_EVENTS_MODIFY_ATTRIBUTE_EVENT_H_

#include "event/event_registry.h"
#include "events_generated.h"

namespace fpl {
namespace fpl_project {

struct ModifyAttributePayload {
  ModifyAttributePayload(entity::EntityRef target_,
                         const ModifyAttribute* modify_attribute_)
      : target(target_), modify_attribute(modify_attribute_) {}

  entity::EntityRef target;
  const ModifyAttribute* modify_attribute;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_PAYLOAD_ID(fpl::fpl_project::EventSinkUnion_ModifyAttribute,
                              fpl::fpl_project::ModifyAttributePayload)

#endif  // FPL_EVENTS_MODIFY_ATTRIBUTE_EVENT_H_

