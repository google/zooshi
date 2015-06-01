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

#ifndef FPL_EVENT_LISTENER_H_
#define FPL_EVENT_LISTENER_H_

#include "event_system/event_payload.h"
#include "event_system/event_registry.h"

namespace fpl {
namespace event {

class EventPayload;

// An interface for listening for events. An `EventListener` can be registered
// with any number with event queues. When an event is broadcast the
// `EventManager` will call the OnEvent function on each registered listener.
class EventListener {
 public:
  // Use this function to send an event payload to specific this EventListener.
  // T must be a type that has been registered with the EventRegistry. See
  // event_registry.h for details.
  template <typename T>
  void SendEvent(const T& payload) {
    OnEvent(EventPayload(EventIdRegistry<T>::kEventId, &payload));
  }

 private:
  // Override this function to respond to events that have been sent to the
  // event listener. The EventPayload contains an ID and a void pointer which
  // can be cast to the appropriate type using the member function ToData().
  // Typical usage would look something like this:
  //
  //     void Foo:OnEvent(const EventPayload& event_payload) {
  //       switch(event_payload.event_id()) {
  //         case BarEventId: {
  //           BarEvent* bar_event = event_payload.ToData<BarEvent>();
  //           // Do things with your bar_event here...
  //         }
  //         case BazEventId: {
  //           BazEvent* baz_event = event_payload.ToData<BazEvent>();
  //           // Do things with your baz_event here...
  //         }
  //       }
  //     }
  virtual void OnEvent(const EventPayload& event_payload) = 0;
};

}  // event
}  // fpl

#endif  // FPL_EVENT_LISTENER_H_

