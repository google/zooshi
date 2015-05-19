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

#ifndef COMPONENTS_PHYSICS_H_
#define COMPONENTS_PHYSICS_H_

#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct PhysicsData {
 public:
  mathfu::vec3 velocity;
  mathfu::quat angular_velocity;
  // TODO: Later, replace this stuff with a binding to a bullet physics object.
  // b/20176055
};

class PhysicsComponent : public entity::Component<PhysicsData> {
 public:
  PhysicsComponent() {}

  void InitializeAudio(pindrop::AudioEngine* audio_engine, const char* bounce);

  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);

  virtual void InitEntity(entity::EntityRef& /*entity*/);

  virtual void UpdateAllEntities(entity::WorldTime delta_time);

 private:
  pindrop::AudioEngine* audio_engine_;
  pindrop::SoundHandle bounce_handle_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PhysicsComponent,
                              fpl::fpl_project::PhysicsData,
                              ComponentDataUnion_PhysicsDef)

#endif  // COMPONENTS_PHYSICS_H_
