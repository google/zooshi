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

#ifndef FPL_ZOOSHI_ZOOSHI_GRAPH_FACTORY_H_
#define FPL_ZOOSHI_ZOOSHI_GRAPH_FACTORY_H_

#include <string>

#include "event/event_system.h"
#include "event/graph.h"
#include "event/graph_factory.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

class ZooshiGraphFactory : public event::GraphFactory {
 public:
  ZooshiGraphFactory(event::EventSystem* event_system,
                     event::LoadFileCallback load_file_callback,
                     pindrop::AudioEngine* audio_engine)
      : event::GraphFactory(event_system, load_file_callback),
        audio_engine_(audio_engine) {}

 private:
  virtual bool ParseData(event::EventSystem* event_system, event::Graph* graph,
                         const std::string* data);

  pindrop::AudioEngine* audio_engine_;
};

}  // fpl_project
}  // fpl

#endif  // FPL_ZOOSHI_ZOOSHI_GRAPH_FACTORY_H_
