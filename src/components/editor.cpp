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
#include "mathfu/utilities.h"

namespace fpl {
namespace fpl_project {

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
    if (editor_def->ignore_selection() != -1) {
      editor_data->ignore_selection = editor_def->ignore_selection();
    }
  }
}
void EditorComponent::AddFromPrototypeData(entity::EntityRef& entity,
                                           const EditorDef* editor_def) {
  EditorData* editor_data = AddEntity(entity);
  if (editor_def->comment() != nullptr) {
    editor_data->comment = editor_def->comment()->c_str();
  }
  if (editor_def->ignore_selection() != -1) {
    editor_data->ignore_selection = editor_def->ignore_selection();
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
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* EditorComponent::PopulateRawData(entity::EntityRef& entity,
                                       void* helper) const {
  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);

  const EditorData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;
  // Const-cast should be okay here because we're just giving
  // something an entity ID before exporting it (if it doesn't already
  // have one).
  auto entity_id = fbb->CreateString(
      const_cast<EditorComponent*>(this)->GetEntityID(entity));
  auto prototype =
      (data->prototype != "") ? fbb->CreateString(data->prototype) : 0;
  auto comment = (data->comment != "") ? fbb->CreateString(data->comment) : 0;

  EditorDefBuilder builder(*fbb);
  builder.add_entity_id(entity_id);
  if (data->prototype != "") {
    builder.add_prototype(prototype);
  }
  if (data->comment != "") {
    builder.add_comment(comment);
  }
  if (data->ignore_selection != -1)
    builder.add_ignore_selection(data->ignore_selection);

  auto component = CreateComponentDefInstance(
      *fbb, ComponentDataUnion_EditorDef, builder.Finish().Union());

  return reinterpret_cast<void*>(component.o);
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

}  // namespace fpl_project
}  // namespace fpl
