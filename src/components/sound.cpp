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
#include "corgi_component_library/transform.h"
#include "pindrop/pindrop.h"

// In windows.h, PlaySound is #defined to be either PlaySoundW or PlaySoundA.
// We need to undef this macro or AudioEngine::PlaySound() won't compile.
// TODO(amablue): Change our PlaySound to have a different name (b/30090037).
#if defined(PlaySound)
#undef PlaySound
#endif  // defined(PlaySound)

CORGI_DEFINE_COMPONENT(fpl::zooshi::SoundComponent, fpl::zooshi::SoundData)

namespace fpl {
namespace zooshi {

using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;

void SoundComponent::Init() {
  audio_engine_ =
      entity_manager_->GetComponent<ServicesComponent>()->audio_engine();
}

void SoundComponent::UpdateAllEntities(corgi::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    SoundData* sound_data = Data<SoundData>(iter->entity);
    if (sound_data->channel.Valid()) {
      TransformData* transform_data = Data<TransformData>(iter->entity);
      sound_data->channel.SetLocation(transform_data->position);
    }
  }
}

void SoundComponent::CleanupEntity(corgi::EntityRef& entity) {
  SoundData* sound_data = Data<SoundData>(entity);
  if (sound_data->channel.Valid()) {
    sound_data->channel.Stop();
  }
}

void SoundComponent::AddFromRawData(corgi::EntityRef& entity,
                                    const void* raw_data) {
  auto sound_def = static_cast<const SoundDef*>(raw_data);

  SoundData* sound_data = AddEntity(entity);
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);

  TransformData* transform_data = Data<TransformData>(entity);
  sound_data->channel = audio_engine_->PlaySound(sound_def->sound()->c_str(),
                                                 transform_data->position);
}

}  // zooshi
}  // fpl
