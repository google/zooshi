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

#include "components/player.h"

#include "entity/entity_common.h"

namespace fpl {

void PlayerComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {}

void PlayerComponent::AddFromRawData(entity::EntityRef& /*entity*/,
                                     const void* /*raw_data*/) {}

void PlayerComponent::InitEntity(entity::EntityRef& /*entity*/) {}

}  // fpl

