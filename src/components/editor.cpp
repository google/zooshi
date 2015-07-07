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

#include <string.h>
#include "components_generated.h"
#include "components/editor.h"
#include "components/rendermesh.h"
#include "components/services.h"
#include "events/editor_event.h"
#include "mathfu/utilities.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::EditorComponent,
                            fpl::fpl_project::EditorData);

namespace fpl {
namespace fpl_project {

void EditorComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();

  services->event_manager()->RegisterListener(EventSinkUnion_EditorEvent, this);
}

void EditorComponent::AddFromRawData(entity::EntityRef& entity,
                                     const void* raw_data) {
  EditorData* editor_data = AddEntity(entity);
  if (raw_data != nullptr) {
    auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
    assert(component_data->data_type() == ComponentDataUnion_EditorDef);
    auto editor_def = static_cast<const EditorDef*>(component_data->data());
    if (editor_def->entity_id() != nullptr) {
      if (editor_data->entity_id != "") {
        RemoveEntityFromDictionary(editor_data->entity_id);
      }
      editor_data->entity_id = editor_def->entity_id()->c_str();
      AddEntityToDictionary(editor_data->entity_id, entity);
    }
    if (editor_def->prototype() != nullptr) {
      editor_data->prototype = editor_def->prototype()->c_str();
    }
    if (editor_def->comment() != nullptr) {
      editor_data->comment = editor_def->comment()->c_str();
    }
    if (editor_def->selection_option() != EditorSelectionOption_Unspecified) {
      editor_data->selection_option = editor_def->selection_option();
    }
    if (editor_def->render_option() != EditorRenderOption_Unspecified) {
      editor_data->render_option = editor_def->render_option();
    }
  }
}
void EditorComponent::AddFromPrototypeData(entity::EntityRef& entity,
                                           const EditorDef* editor_def) {
  EditorData* editor_data = AddEntity(entity);
  if (editor_def->comment() != nullptr) {
    editor_data->comment = editor_def->comment()->c_str();
  }
  if (editor_def->selection_option() != EditorSelectionOption_Unspecified) {
    editor_data->selection_option = editor_def->selection_option();
  }
  if (editor_def->render_option() != EditorRenderOption_Unspecified) {
    editor_data->render_option = editor_def->render_option();
  }
}

void EditorComponent::AddWithSourceFile(entity::EntityRef& entity,
                                        const std::string& source_file) {
  EditorData* data = AddEntity(entity);
  data->source_file = source_file;
  auto ext = data->source_file.rfind('.');
  if (ext != std::string::npos) {
    data->source_file = data->source_file.substr(0, ext);
  }
}

entity::ComponentInterface::RawDataUniquePtr EditorComponent::ExportRawData(
    entity::EntityRef& entity) const {
  const EditorData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  // Const-cast should be okay here because we're just giving
  // something an entity ID before exporting it (if it doesn't already
  // have one).
  auto entity_id =
      fbb.CreateString(const_cast<EditorComponent*>(this)->GetEntityID(entity));
  auto prototype =
      (data->prototype != "") ? fbb.CreateString(data->prototype) : 0;
  auto comment = (data->comment != "") ? fbb.CreateString(data->comment) : 0;

  EditorDefBuilder builder(fbb);
  builder.add_entity_id(entity_id);
  if (data->prototype != "") {
    builder.add_prototype(prototype);
  }
  if (data->comment != "") {
    builder.add_comment(comment);
  }
  if (data->selection_option != EditorSelectionOption_Unspecified)
    builder.add_selection_option(data->selection_option);

  if (data->render_option != EditorRenderOption_Unspecified)
    builder.add_render_option(data->render_option);

  auto component = CreateComponentDefInstance(fbb, ComponentDataUnion_EditorDef,
                                              builder.Finish().Union());

  fbb.Finish(component);
  return fbb.ReleaseBufferPointer();
}

void EditorComponent::InitEntity(entity::EntityRef& entity) {
  EditorData* data = GetComponentData(entity);
  if (data != nullptr && data->entity_id != "")
    AddEntityToDictionary(data->entity_id, entity);
}

void EditorComponent::CleanupEntity(entity::EntityRef& entity) {
  EditorData* data = GetComponentData(entity);
  if (data != nullptr && data->entity_id != "")
    RemoveEntityFromDictionary(data->entity_id);
}

const std::string& EditorComponent::GetEntityID(entity::EntityRef& entity) {
  EditorData* data = GetComponentData(entity);
  if (data == nullptr) {
    return empty_string;
  }
  if (data->entity_id == "") {
    // If this entity doesn't already have an ID, generate a new random one.
    GenerateRandomEntityID(&data->entity_id);
    AddEntityToDictionary(data->entity_id, entity);
  }
  return data->entity_id;
}

void EditorComponent::AddEntityToDictionary(const std::string& key,
                                            entity::EntityRef& entity) {
  entity_dictionary_[key] = entity;
}
void EditorComponent::RemoveEntityFromDictionary(const std::string& key) {
  auto i = entity_dictionary_.find(key);
  if (i != entity_dictionary_.end()) {
    entity_dictionary_.erase(i);
  }
}
entity::EntityRef EditorComponent::GetEntityFromDictionary(
    const std::string& key) {
  auto i = entity_dictionary_.find(key);
  if (i == entity_dictionary_.end()) {
    return entity::EntityRef();
  }
  if (!i->second.IsValid()) {
    // The entity with this key is no longer valid, we'd better remove it.
    RemoveEntityFromDictionary(key);
    return entity::EntityRef();
  }
  return i->second;
}

static const size_t kMaximumGeneratedEntityIDStringLength = 33;

void EditorComponent::GenerateRandomEntityID(std::string* output) {
  int num_digits = 16;
  const char digits[] = "0123456789abcdef";
  int digit_choices = strlen(digits);
  const char separator = '-';
  const int separator_every = 5;

  char name[kMaximumGeneratedEntityIDStringLength + 1];
  size_t i = 0;
  name[i++] = '$';
  for (; i < sizeof(name) - 1; i++) {
    if (i % separator_every != 0) {
      int random_digit = mathfu::RandomInRange(0, digit_choices - 1);
      name[i] = digits[random_digit];
      num_digits--;
    } else {
      name[i] = separator;
    }
    if (num_digits == 0) break;
  }
  name[i + 1] = '\0';
  *output = std::string(name);
}

void EditorComponent::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_EditorEvent: {
      auto* event = event_payload.ToData<EditorEventPayload>();
      auto render_mesh_component =
          entity_manager_->GetComponent<RenderMeshComponent>();
      if (event->action == EditorEventAction_Enter) {
        for (auto iter = component_data_.begin(); iter != component_data_.end();
             ++iter) {
          if (iter->data.render_option == EditorRenderOption_OnlyInEditor ||
              iter->data.render_option == EditorRenderOption_NotInEditor) {
            RenderMeshData* rendermesh_data =
                render_mesh_component->GetComponentData(iter->entity);
            if (rendermesh_data != nullptr) {
              bool hide =
                  (iter->data.render_option == EditorRenderOption_NotInEditor);
              iter->data.backup_rendermesh_hidden =
                  rendermesh_data->currently_hidden;
              rendermesh_data->currently_hidden = hide;
            }
          }
        }
      } else if (event->action == EditorEventAction_Exit) {
        for (auto iter = component_data_.begin(); iter != component_data_.end();
             ++iter) {
          if (iter->data.render_option == EditorRenderOption_OnlyInEditor ||
              iter->data.render_option == EditorRenderOption_NotInEditor) {
            RenderMeshData* rendermesh_data =
                render_mesh_component->GetComponentData(iter->entity);
            if (rendermesh_data != nullptr) {
              rendermesh_data->currently_hidden =
                  iter->data.backup_rendermesh_hidden;
            }
          }
        }
      }
      break;
    }
    default: { assert(0); }
  }
}

}  // namespace fpl_project
}  // namespace fpl
