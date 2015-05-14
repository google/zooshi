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

#include "components/player_projectile.h"
#include "components/transform.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

void PlayerProjectileComponent::InitializeAudio(
    pindrop::AudioEngine* audio_engine, const char* whoosh) {
  audio_engine_ = audio_engine;
  whoosh_handle_ = audio_engine_->GetSoundHandle(whoosh);
}

void PlayerProjectileComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    PlayerProjectileData* projectile_data =
        Data<PlayerProjectileData>(iter->entity);

    // Once an event system is in place, fire an event rather than play a sound
    // directly. b/21117936
    if (projectile_data->whoosh_channel.Valid()) {
      TransformData* transform_data = Data<TransformData>(iter->entity);
      projectile_data->whoosh_channel.SetLocation(transform_data->position);
    }
  }
}

void PlayerProjectileComponent::InitEntity(entity::EntityRef& entity) {
  if (audio_engine_) {
    PlayerProjectileData* data = Data<PlayerProjectileData>(entity);
    data->whoosh_channel = audio_engine_->PlaySound(whoosh_handle_);
  }
}

void PlayerProjectileComponent::CleanupEntity(entity::EntityRef& entity) {
  PlayerProjectileData* data = Data<PlayerProjectileData>(entity);
  if (data->whoosh_channel.Valid()) {
    data->whoosh_channel.Stop();
  }
}

void PlayerProjectileComponent::AddFromRawData(entity::EntityRef& entity,
                                               const void* /*raw_data*/) {
  AddEntity(entity);
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

}  // fpl_project
}  // fpl

