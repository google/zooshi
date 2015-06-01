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

#include "components/physics.h"
#include "components/transform.h"
#include "event_system/event_manager.h"
#include "events/event_ids.h"
#include "events/play_sound.h"
#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "pindrop/pindrop.h"

using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const float kGroundPlane = 0.0f;

void PhysicsComponent::Initialize(event::EventManager* event_manager,
                                  pindrop::SoundHandle bounce_handle,
                                  const Config* config) {
  event_manager_ = event_manager;
  bounce_handle_ = bounce_handle;
  config_ = config;

  broadphase_.reset(new btDbvtBroadphase());
  collision_configuration_.reset(new btDefaultCollisionConfiguration());
  collision_dispatcher_.reset(
      new btCollisionDispatcher(collision_configuration_.get()));
  constraint_solver_.reset(new btSequentialImpulseConstraintSolver());
  bullet_world_.reset(new btDiscreteDynamicsWorld(
      collision_dispatcher_.get(), broadphase_.get(), constraint_solver_.get(),
      collision_configuration_.get()));
  bullet_world_->setGravity(btVector3(0, 0, config->gravity()));

  // TODO: Create from config data (b/21502254)
  // Create the ground plane.
  ground_shape_.reset(new btStaticPlaneShape(btVector3(0, 0, 1), 1));

  ground_motion_state_.reset(new btDefaultMotionState(
      btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, kGroundPlane))));
  btVector3 inertia(0, 0, 0);
  btRigidBody::btRigidBodyConstructionInfo rigid_body_builder(
      0, ground_motion_state_.get(), ground_shape_.get(), inertia);
  ground_rigid_body_.reset(new btRigidBody(rigid_body_builder));
  ground_rigid_body_->setRestitution(1.0f);
  bullet_world_->addRigidBody(ground_rigid_body_.get());
}

PhysicsComponent::~PhysicsComponent() {
  ClearComponentData();

  bullet_world_->removeRigidBody(ground_rigid_body_.get());
}

void PhysicsComponent::AddFromRawData(entity::EntityRef& entity,
                                      const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PhysicsDef);
  auto physics_def = static_cast<const PhysicsDef*>(component_data->data());
  PhysicsData* physics_data = AddEntity(entity);

  (void)physics_def;
  (void)physics_data;

  // TODO - populate data here. (b/21502254)
}

void PhysicsComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  // Step the world.
  bullet_world_->stepSimulation(delta_time / 1000.f,
                                config_->bullet_max_steps());

  // Copy position information to Transforms.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    PhysicsData* physics_data = Data<PhysicsData>(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);

    auto trans = physics_data->rigid_body->getWorldTransform();
    transform_data->position =
        mathfu::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(),
                     trans.getOrigin().getZ());
    transform_data->orientation =
        mathfu::quat(trans.getRotation().getW(), trans.getRotation().getX(),
                     trans.getRotation().getY(), trans.getRotation().getZ());

    // TODO: Generate this event based on the collision (b/21522963)
    if (transform_data->position.z() < kGroundPlane) {
      event_manager_->BroadcastEvent(
          PlaySoundEvent(bounce_handle_, transform_data->position));
    }
  }
}

// Physics component requires that you have a transform component:
void PhysicsComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void PhysicsComponent::CleanupEntity(entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  bullet_world_->removeRigidBody(physics_data->rigid_body.get());
}

void PhysicsComponent::UpdatePhysicsFromTransform(entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  TransformData* transform_data = Data<TransformData>(entity);

  btQuaternion orientation(transform_data->orientation.scalar(),
                           transform_data->orientation.vector().x(),
                           transform_data->orientation.vector().y(),
                           transform_data->orientation.vector().z());
  btVector3 position(transform_data->position.x(), transform_data->position.y(),
                     transform_data->position.z());
  physics_data->rigid_body->setWorldTransform(
      btTransform(orientation, position));
}

}  // fpl_project
}  // fpl
