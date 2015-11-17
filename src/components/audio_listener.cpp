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
#include "corgi/entity_common.h"
#include "corgi_component_library/transform.h"
#include "pindrop/pindrop.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::AudioListenerComponent,
                       fpl::zooshi::AudioListenerData)

namespace fpl {
namespace zooshi {

using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;

void AudioListenerComponent::Init() {
  audio_engine_ =
      entity_manager_->GetComponent<ServicesComponent>()->audio_engine();
}

void AudioListenerComponent::UpdateAllEntities(
    corgi::WorldTime /*delta_time*/) {
  TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef& entity = iter->entity;
    AudioListenerData* listener_data = Data<AudioListenerData>(entity);
    assert(listener_data->listener.Valid());
    mathfu::mat4 listener_matrix = transform_component->WorldTransform(entity);
    listener_data->listener.SetMatrix(listener_matrix);
  }
}

void AudioListenerComponent::InitEntity(corgi::EntityRef& entity) {
  AudioListenerData* listener_data = Data<AudioListenerData>(entity);
  listener_data->listener = audio_engine_->AddListener();
}

void AudioListenerComponent::CleanupEntity(corgi::EntityRef& entity) {
  AudioListenerData* listener_data = Data<AudioListenerData>(entity);
  audio_engine_->RemoveListener(&listener_data->listener);
}

void AudioListenerComponent::AddFromRawData(corgi::EntityRef& entity,
                                            const void* /*raw_data*/) {
  AddEntity(entity);
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

corgi::ComponentInterface::RawDataUniquePtr
AudioListenerComponent::ExportRawData(const corgi::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  fbb.Finish(CreateListenerDef(fbb));
  return fbb.ReleaseBufferPointer();
}

}  // zooshi
}  // fpl
