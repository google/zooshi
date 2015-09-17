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

#include "zooshi_graph_factory.h"

#include <string>

#include "event/event_system.h"
#include "event/graph.h"
#include "graph_generated.h"
#include "fplbase/utilities.h"
#include "mathfu/vector_3.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

static bool ParseType(size_t node_index, size_t edge_index, event::Graph* graph,
                      const graph::InputEdgeDef* edge_def,
                      pindrop::AudioEngine* audio_engine) {
  if (edge_def->edge_type() >= graph::InputType_Max) {
    LogError("Cannot load data of unknown type from data files. (Got %i)",
             edge_def->edge_type());
    return false;
  }
  switch (edge_def->edge_type()) {
    case graph::InputType_Pulse: {
      // Pulses have no data associated with them.
      break;
    }
    case graph::InputType_Bool: {
      const graph::Bool* default_bool =
          static_cast<const graph::Bool*>(edge_def->edge());
      graph->SetDefaultValue<bool>(node_index, edge_index,
                                   default_bool->value() != 0);
      break;
    }
    case graph::InputType_Int: {
      const graph::Int* default_int =
          static_cast<const graph::Int*>(edge_def->edge());
      graph->SetDefaultValue<int>(node_index, edge_index, default_int->value());
      break;
    }
    case graph::InputType_Float: {
      const graph::Float* default_float =
          static_cast<const graph::Float*>(edge_def->edge());
      graph->SetDefaultValue<float>(node_index, edge_index,
                                    default_float->value());
      break;
    }
    case graph::InputType_String: {
      const graph::String* default_string =
          static_cast<const graph::String*>(edge_def->edge());
      graph->SetDefaultValue<std::string>(node_index, edge_index,
                                          default_string->value()->c_str());
      break;
    }
    case graph::InputType_Entity: {
      // Entities can not be specified directly. Generally you would attach
      // to an output edge that supplied an entity, but it is legal to
      // leave an entity unspecified.
      break;
    }
    case graph::InputType_Vec3: {
      const graph::Vec3* default_vec3 =
          static_cast<const graph::Vec3*>(edge_def->edge());
      mathfu::vec3 vec(default_vec3->value()->x(), default_vec3->value()->y(),
                       default_vec3->value()->z());
      graph->SetDefaultValue<mathfu::vec3>(node_index, edge_index, vec);
      break;
    }
    case graph::InputType_SoundHandle: {
      const graph::SoundHandle* default_sound_handle =
          static_cast<const graph::SoundHandle*>(edge_def->edge());
      pindrop::SoundHandle sound_handle =
          audio_engine->GetSoundHandle(default_sound_handle->value()->c_str());
      graph->SetDefaultValue<pindrop::SoundHandle>(node_index, edge_index,
                                                   sound_handle);
      break;
    }
    case graph::InputType_AudioChannel: {
      // AudioChannels can not be specified directly. Generally you would attach
      // to an output edge that supplied an audio channel, but it is legal to
      // leave an audio channel unspecified.
      break;
    }
    case graph::InputType_OutputEdgeTarget: {
      // OutputEdgeTargets are a special case - they point at the output of
      // another node and so they do not need to store any default value.
      break;
    }
    default: {
      // Not all types can necessarily be loaded from data. Error out if
      // attempting to load these types.
      if (edge_def->edge_type() < graph::InputType_Max) {
        LogError("Cannot load data of type \"%s\" from data files.",
                 EnumNameInputType(edge_def->edge_type()));
      } else {
        LogError("Cannot load data of unknown type from data files. (Got %i)",
                 edge_def->edge_type());
      }
      return false;
    }
  }
  return true;
}

bool ZooshiGraphFactory::ParseData(event::EventSystem* event_system,
                                   event::Graph* graph,
                                   const std::string* data) {
  const graph::GraphDef* graph_def = graph::GetGraphDef(data->c_str());
  for (size_t i = 0; i != graph_def->node_list()->size(); ++i) {
    // Get Module.
    const graph::NodeDef* node_def = graph_def->node_list()->Get(i);
    const char* module_name = node_def->module()->c_str();
    const event::Module* module = event_system->GetModule(module_name);
    if (module == nullptr) return false;

    // Get NodeSignature.
    const char* node_sig_name = node_def->name()->c_str();
    const event::NodeSignature* node_sig =
        module->GetNodeSignature(node_sig_name);
    if (node_sig == nullptr) return false;

    // Create Node and fill in edges.
    event::Node* node = graph->AddNode(node_sig);
    auto edges = node_def->input_edge_list();
    if (edges != nullptr) {
      for (auto iter = edges->begin(); iter != edges->end(); ++iter) {
        const graph::InputEdgeDef* edge_def = *iter;
        node->input_edges().push_back(event::InputEdge());
        if (edge_def->edge_type() == graph::InputType_OutputEdgeTarget) {
          const graph::OutputEdgeTarget* target =
              static_cast<const graph::OutputEdgeTarget*>(edge_def->edge());
          node->input_edges().back().SetTarget(target->node_index(),
                                               target->edge_index());
        }
      }
    }
  }

  // Finalize Nodes and fill in default values.
  if (!graph->FinalizeNodes()) return false;

  // Set up default values.
  for (size_t node_index = 0; node_index != graph_def->node_list()->size();
       ++node_index) {
    const graph::NodeDef* node_def = graph_def->node_list()->Get(node_index);
    auto edges = node_def->input_edge_list();
    if (edges != nullptr) {
      for (size_t edge_index = 0; edge_index < edges->size(); ++edge_index) {
        const graph::InputEdgeDef* edge_def = edges->Get(edge_index);
        if (!ParseType(node_index, edge_index, graph, edge_def,
                       audio_engine_)) {
          return false;
        }
      }
    }
  }
  return true;
}

}  // fpl_project
}  // fpl
