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

#ifndef FPL_EVENT_REGISTRY_H_
#define FPL_EVENT_REGISTRY_H_

// Use this macro to register an event id to an event payload data type. For
// example, if you had a Foo event, you could associate the FooEventId with the
// FooEventPayload as follows:
//
//     FPL_REGISTER_EVENT_ID(FooEventId, FooEventPayload)
//
// FooEventId must be a unique integer that is not registered to any other
// types, and each payload type may be registered with only one event id.
#define FPL_REGISTER_EVENT_ID(event_id, EventPayloadDataType) \
  namespace fpl {                                             \
  namespace event {                                           \
  template <>                                                 \
  struct EventIdRegistry<EventPayloadDataType> {              \
    static const int kEventId = event_id;                     \
  };                                                          \
  }                                                           \
  }

namespace fpl {
namespace event {

// This class acts as a static event id storage, it is not meant to be
// instantiated.
template <typename T>
struct EventIdRegistry {
  static const int kEventId;

 private:
  EventIdRegistry();
};

// Special value representing an event type that has not been registered with
// the event system.
const int kEventIdRegistryInvalidId = -9999;

template <typename T>
const int EventIdRegistry<T>::kEventId = kEventIdRegistryInvalidId;

}  // event
}  // fpl

#endif  // FPL_EVENT_REGISTRY_H_
