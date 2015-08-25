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

#ifndef FPL_GRAPH_FACTORY_H_
#define FPL_GRAPH_FACTORY_H_

#include "event/event_system.h"
#include "event/graph.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

typedef std::unordered_map<std::string, std::unique_ptr<event::Graph>>
    GraphDictionary;

event::Graph* LoadGraph(event::EventSystem* event_system_,
                        GraphDictionary* graph_dictionary, const char* filename,
                        pindrop::AudioEngine* audio_engine);

}  // fpl_project
}  // fpl

#endif  // FPL_GRAPH_FACTORY_H_
