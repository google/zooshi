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

#ifndef AUDIO_EVENT_H_
#define AUDIO_EVENT_H_

#include "event_system/event_manager.h"
#include "events/event_ids.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

struct AudioEventPayload {
  AudioEventPayload(pindrop::SoundHandle handle_, mathfu::vec3 location_)
      : handle(handle_), location(location_) {}

  pindrop::SoundHandle handle;
  mathfu::vec3 location;
};

}  // fpl_project
}  // fpl

FPL_REGISTER_EVENT_ID(fpl::fpl_project::kEventIdPlayAudio,
                      fpl::fpl_project::AudioEventPayload)

#endif  // AUDIO_EVENT_H_

