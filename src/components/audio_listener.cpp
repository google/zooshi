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

#include "components/audio_listener.h"
#include "components/services.h"
#include "components/transform.h"
#include "entity/entity_common.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

void AudioListenerComponent::Init() {
  audio_engine_ =
      entity_manager_->GetComponent<ServicesComponent>()->audio_engine();
}

void AudioListenerComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    AudioListenerData* listener_data = Data<AudioListenerData>(iter->entity);
    assert(listener_data->listener.Valid());
    TransformData* transform_data = Data<TransformData>(iter->entity);
    listener_data->listener.SetLocation(transform_data->position);
  }
}

void AudioListenerComponent::InitEntity(entity::EntityRef& entity) {
  AudioListenerData* listener_data = Data<AudioListenerData>(entity);
  listener_data->listener = audio_engine_->AddListener();
}

void AudioListenerComponent::CleanupEntity(entity::EntityRef& entity) {
  AudioListenerData* listener_data = Data<AudioListenerData>(entity);
  audio_engine_->RemoveListener(&listener_data->listener);
}

void AudioListenerComponent::AddFromRawData(entity::EntityRef& entity,
                                            const void* /*raw_data*/) {
  AddEntity(entity);
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

entity::ComponentInterface::RawDataUniquePtr
AudioListenerComponent::ExportRawData(entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* AudioListenerComponent::PopulateRawData(entity::EntityRef& entity,
                                              void* helper) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);
  auto component = CreateComponentDefInstance(
      *fbb, ComponentDataUnion_ListenerDef, CreateListenerDef(*fbb).Union());
  return reinterpret_cast<void*>(component.o);
}

}  // fpl_project
}  // fpl
