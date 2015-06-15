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

#include "components/sound.h"
#include "components/services.h"
#include "components/transform.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

void SoundComponent::Init() {
  audio_engine_ =
      entity_manager_->GetComponent<ServicesComponent>()->audio_engine();
}

void SoundComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    SoundData* sound_data = Data<SoundData>(iter->entity);
    if (sound_data->channel.Valid()) {
      TransformData* transform_data = Data<TransformData>(iter->entity);
      sound_data->channel.SetLocation(transform_data->position);
    }
  }
}

void SoundComponent::CleanupEntity(entity::EntityRef& entity) {
  SoundData* sound_data = Data<SoundData>(entity);
  if (sound_data->channel.Valid()) {
    sound_data->channel.Stop();
  }
}

void SoundComponent::AddFromRawData(entity::EntityRef& entity,
                                    const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_SoundDef);
  auto sound_def = static_cast<const SoundDef*>(component_data->data());

  SoundData* sound_data = AddEntity(entity);
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);

  TransformData* transform_data = Data<TransformData>(entity);
  sound_data->channel = audio_engine_->PlaySound(sound_def->sound()->c_str(),
                                                 transform_data->position);
}

}  // fpl_project
}  // fpl

