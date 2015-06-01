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

#ifndef FPL_EVENT_PAYLOAD_H_
#define FPL_EVENT_PAYLOAD_H_

#include <cassert>

#include "event_system/event_registry.h"

namespace fpl {
namespace event {

class EventListener;

// An `EventPayload` is a wrapper around a void* that allows for
// somewhat-typesafe casting. Each event id is registered with a specific
// payload data type. If the event that was broadcast and the event payload do
// not match the event system will assert immediately.
class EventPayload {
 public:
  // Returns the event_id of the associated payload type.
  int event_id() const { return event_id_; }

  // Cast the payload to the expected type. If there is a mismatch between the
  // event id and the type registered with the event, this function will assert.
  template <typename T>
  const T* ToData() const {
    assert(event_id_ == EventIdRegistry<T>::kEventId);
    return static_cast<const T*>(event_data_);
  }

 private:
  friend EventListener;
  EventPayload(int event_id) : event_id_(event_id), event_data_(nullptr) {}

  template <typename T>
  EventPayload(int event_id, const T* event_data)
      : event_id_(event_id),
        event_data_(static_cast<const void*>(event_data)) {}

  int event_id_;
  const void* event_data_;
};

}  // event
}  // fpl

#endif  // FPL_EVENT_PAYLOAD_H_

