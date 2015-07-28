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

#ifndef COMPONENTS_PATRON_H_
#define COMPONENTS_PATRON_H_

#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "entity/entity_manager.h"
#include "event/event_manager.h"
#include "events_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace event {

class EventManager;

}  // event

namespace fpl_project {

enum PatronState {
  // Laying down in wait for the raft to come in range. If this patron has been
  // fed this lap, it will not stand up again until the next lap.
  kPatronStateLayingDown,

  // Standing up, ready to be hit by the player.
  kPatronStateUpright,

  // The patron has been hit, and is falling down.
  kPatronStateFalling,

  // The patron is within range of the boad, and is standing up.
  kPatronStateGettingUp,
};

// Data for scene object components.
struct PatronData {
  PatronData()
      : on_collision(nullptr),
        state(kPatronStateLayingDown),
        last_lap_fed(-1),
        y(0.0f),
        dy(0.0f) {}

  // The event to trigger when a projectile collides with this patron.
  const ActionDef* on_collision;
  std::vector<unsigned char> on_collision_flatbuffer;

  // Whether the patron is standing up or falling down.
  PatronState state;

  // Keep track of the last time this patron was fed so we know whether they
  // should pop up this lap.
  int last_lap_fed;

  // misc data for simulating the fall:
  mathfu::quat original_orientation;
  mathfu::quat falling_rotation;
  float y;
  float dy;

  // If the raft entity is within the pop_in_range it will stand up. If it is
  // once up, if it's not in the pop out range, it will fall down. As a minor
  // optimization, it's stored here as the square of the distance.
  float pop_in_radius_squared;
  float pop_out_radius_squared;
  float pop_in_radius;
  float pop_out_radius;

  // Each time the raft makes a lap around the river, its lap counter is
  // incremented.  Patrons will only stand up when the lap counter is in the
  // range [min_lap, max_lap].
  int min_lap;
  int max_lap;

  // The tag of the body part that needs to be hit to trigger a fall.
  // Note that an empty name means any collision counts.
  std::string target_tag;
};

class PatronComponent : public entity::Component<PatronData>,
                        public event::EventListener {
 public:
  PatronComponent() {}

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& parent, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  // This needs to be called after the entities have been loaded from data.
  void PostLoadFixup();

  virtual void OnEvent(const event::EventPayload& payload);

 private:
  void HandleCollision(const entity::EntityRef& patron_entity,
                       const entity::EntityRef& proj_entity,
                       const std::string& part_tag);
  void SpawnSplatter(const mathfu::vec3& position, int count);

  const Config* config_;
  event::EventManager* event_manager_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PatronComponent,
                              fpl::fpl_project::PatronData)

#endif  // COMPONENTS_PATRON_H_
