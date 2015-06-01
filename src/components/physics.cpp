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
  bullet_world_->setGravity(btVector3(0.0f, 0.0f, config->gravity()));
}

PhysicsComponent::~PhysicsComponent() { ClearComponentData(); }

void PhysicsComponent::AddFromRawData(entity::EntityRef& entity,
                                      const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PhysicsDef);
  auto physics_def = static_cast<const PhysicsDef*>(component_data->data());
  PhysicsData* physics_data = AddEntity(entity);

  if (physics_def->shape() != nullptr) {
    switch (physics_def->shape()->data_type()) {
      case BulletShapeUnion_BulletSphereDef: {
        auto sphere_data =
            static_cast<const BulletSphereDef*>(physics_def->shape()->data());
        physics_data->shape.reset(new btSphereShape(sphere_data->radius()));
        break;
      }
      case BulletShapeUnion_BulletBoxDef: {
        auto box_data =
            static_cast<const BulletBoxDef*>(physics_def->shape()->data());
        btVector3 half_extents(box_data->half_extents()->x(),
                               box_data->half_extents()->y(),
                               box_data->half_extents()->z());
        physics_data->shape.reset(new btBoxShape(btVector3(half_extents)));
        break;
      }
      case BulletShapeUnion_BulletCylinderDef: {
        auto cylinder_data =
            static_cast<const BulletCylinderDef*>(physics_def->shape()->data());
        btVector3 half_extents(cylinder_data->half_extents()->x(),
                               cylinder_data->half_extents()->y(),
                               cylinder_data->half_extents()->z());
        physics_data->shape.reset(new btCylinderShape(half_extents));
        break;
      }
      case BulletShapeUnion_BulletCapsuleDef: {
        auto capsule_data =
            static_cast<const BulletCapsuleDef*>(physics_def->shape()->data());
        physics_data->shape.reset(
            new btCapsuleShape(capsule_data->radius(), capsule_data->height()));
        break;
      }
      case BulletShapeUnion_BulletConeDef: {
        auto cone_data =
            static_cast<const BulletConeDef*>(physics_def->shape()->data());
        physics_data->shape.reset(
            new btConeShape(cone_data->radius(), cone_data->height()));
        break;
      }
      case BulletShapeUnion_BulletStaticPlaneDef: {
        auto plane_data = static_cast<const BulletStaticPlaneDef*>(
            physics_def->shape()->data());
        btVector3 normal(plane_data->normal()->x(), plane_data->normal()->y(),
                         plane_data->normal()->z());
        physics_data->shape.reset(
            new btStaticPlaneShape(normal, plane_data->constant()));
        break;
      }
      case BulletShapeUnion_BulletNoShapeDef:
      default: {
        physics_data->shape.reset(new btEmptyShape());
        break;
      }
    }
  } else {
    physics_data->shape.reset(new btEmptyShape());
  }
  physics_data->motion_state.reset(new btDefaultMotionState());
  btScalar mass = physics_def->mass();
  btVector3 inertia(0.0f, 0.0f, 0.0f);
  physics_data->shape->calculateLocalInertia(mass, inertia);
  btRigidBody::btRigidBodyConstructionInfo rigid_body_builder(
      mass, physics_data->motion_state.get(), physics_data->shape.get(),
      inertia);
  rigid_body_builder.m_restitution = physics_def->restitution();
  physics_data->rigid_body.reset(new btRigidBody(rigid_body_builder));

  bullet_world_->addRigidBody(physics_data->rigid_body.get());
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
