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

#include "components/patron.h"
#include "components/physics.h"
#include "components/player_projectile.h"
#include "components/score.h"
#include "components/transform.h"
#include "events/hit_patron.h"
#include "events/hit_patron_body.h"
#include "events/hit_patron_mouth.h"
#include "mathfu/glsl_mappings.h"

using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

// All of these numbers were picked for purely aesthetic reasons:
static const float kHitRadius = 6.0f;
static const float kHitRadiusSquared = kHitRadius * kHitRadius;
static const float kHitMinHeight = 10.0;
static const float kHitMaxHeight = 20.0;

static const float kSplatterCount = 10;

static const float kGravity = 0.00005f;
static const float kAtRestThreshold = 0.005f;
static const float kBounceFactor = 0.4f;

void PatronComponent::Initialize(const Config* config,
                                 event::EventManager* event_manager) {
  config_ = config;
  event_manager_ = event_manager;
}

void PatronComponent::AddFromRawData(entity::EntityRef& parent,
                                     const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PatronDef);
  auto patron_def = static_cast<const PatronDef*>(component_data->data());
  PatronData* patron_data = AddEntity(parent);

  (void)patron_data;
  (void)patron_def;
}

void PatronComponent::InitEntity(entity::EntityRef& entity) { (void)entity; }

void PatronComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef patron = iter->entity;
    TransformData* transform_data = Data<TransformData>(patron);
    PatronData* patron_data = Data<PatronData>(patron);
    if (!patron_data->fallen) {
      // This is terrible, terrible collision detection code.  It will be
      // replaced with non-terrible code once we get Bullet physics working
      // in the game, but we needed someting quick to prove out gameplay,
      // and here it is.
      PlayerProjectileComponent* projectile_component =
          entity_manager_->GetComponent<PlayerProjectileComponent>();
      assert(projectile_component != nullptr);

      for (auto projectile_iter = projectile_component->begin();
           projectile_iter != projectile_component->end(); ++projectile_iter) {
        entity::EntityRef projectile = projectile_iter->entity;
        TransformData* projectile_transform =
            entity_manager_->GetComponentData<TransformData>(projectile);
        vec3 projectile_pos = projectile_transform->position;
        vec3 patron_pos = transform_data->position;

        float projectile_height = projectile_pos.z();
        projectile_pos.z() = 0;
        patron_pos.z() = 0;

        // Check to see if the projectile has collided with the patron.
        // Collision here is just a simple cylinder check.  If you hit the
        // top part, you knock it over.
        if (projectile_height <= kHitMaxHeight &&
            (projectile_pos - patron_pos).LengthSquared() < kHitRadiusSquared) {
          PlayerProjectileData* projectile_data =
              Data<PlayerProjectileData>(projectile);
          entity::EntityRef& projectile_owner = projectile_data->owner;

          // have to hit it near the top to actually knock it over
          if (projectile_height >= kHitMinHeight) {
            vec3 spin_direction_vector =
                entity_manager_->GetComponentData<PhysicsData>(projectile)
                    ->velocity;
            spin_direction_vector.z() = 0;  // flatten so it's on the XY plane.

            patron_data->falling_rotation = quat::RotateFromTo(
                spin_direction_vector, vec3(0.0f, 0.0f, 1.0f));
            patron_data->fallen = true;
            patron_data->y = 1.0f;
            patron_data->dy = 0.0f;
            patron_data->original_orientation = transform_data->orientation;
            event_manager_->BroadcastEvent(
                kEventIdHitPatronMouth,
                HitPatronMouthEventPayload(projectile_owner, projectile,
                                           patron));
          } else {
            event_manager_->BroadcastEvent(
                kEventIdHitPatronBody,
                HitPatronBodyEventPayload(projectile_owner, projectile,
                                          patron));
          }
          event_manager_->BroadcastEvent(
              kEventIdHitPatron,
              HitPatronEventPayload(projectile_owner, projectile, patron));

          // Even if you didn't hit the top, if got here, you got some
          // kind of collision, so you get a splatter.
          SpawnSplatter(projectile_transform->position, kSplatterCount);
          entity_manager_->DeleteEntity(projectile);
        }
      }
    }
    // This is explicitly not an if-else, because we need to make things vanish
    // that have been hit this update.
    if (patron_data->fallen && !patron_data->at_rest) {
      // Some basic math to fake a convincing fall + bounce on a hinge joint.
      // Again, should be handled a bit better once we get bullet physics in.

      float y = patron_data->y;
      float dy = patron_data->dy;
      dy -= static_cast<float>(delta_time) * kGravity;
      y += dy;
      if (y < 0) {
        dy *= -kBounceFactor;
        y = 0.0f;
        if (dy < kAtRestThreshold) patron_data->at_rest = true;
      }
      patron_data->y = y;
      patron_data->dy = dy;
      // For our simple simulation of falling and bouncing, Y is always
      // guaranteed to be between 0 and 1.
      transform_data->orientation =
          patron_data->original_orientation *
          quat::Slerp(quat::identity, patron_data->falling_rotation, 1.0 - y);
    }
  }
}

void PatronComponent::SpawnSplatter(const mathfu::vec3& position, int count) {
  for (int i = 0; i < count; i++) {
    entity::EntityRef particle =
        entity_manager_->CreateEntityFromData(config_->splatter_particle_def());

    TransformData* transform_data =
        entity_manager_->GetComponentData<TransformData>(particle);
    PhysicsData* physics_data =
        entity_manager_->GetComponentData<PhysicsData>(particle);

    transform_data->position = position;

    physics_data->velocity = vec3(mathfu::RandomInRange(-0.25f, 0.25f),
                                  mathfu::RandomInRange(-0.25f, 0.25f),
                                  mathfu::RandomInRange(0.0f, 0.5f));
    physics_data->angular_velocity = mathfu::quat::FromEulerAngles(mathfu::vec3(
        mathfu::RandomInRange(0.05f, 0.1f), mathfu::RandomInRange(0.05f, 0.1f),
        mathfu::RandomInRange(0.05f, 0.1f)));
  }
}

}  // fpl_project
}  // fpl
