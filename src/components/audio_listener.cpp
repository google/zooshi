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
#include "components/transform.h"
#include "entity/entity_common.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

void AudioListenerComponent::Initialize(pindrop::AudioEngine* audio_engine) {
  audio_engine_ = audio_engine;
}

void AudioListenerComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
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

}  // fpl_project
}  // fpl

