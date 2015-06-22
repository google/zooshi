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
#include "components/services.h"
#include "components/transform.h"
#include "event/event_manager.h"
#include "events/collision.h"
#include "events_generated.h"
#include "events/play_sound.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/mesh.h"
#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"

using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const char* kPhysicsShader = "shaders/color";

void PhysicsComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();
  event_manager_ =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  MaterialManager* material_manager =
      entity_manager_->GetComponent<ServicesComponent>()->material_manager();

  broadphase_.reset(new btDbvtBroadphase());
  collision_configuration_.reset(new btDefaultCollisionConfiguration());
  collision_dispatcher_.reset(
      new btCollisionDispatcher(collision_configuration_.get()));
  constraint_solver_.reset(new btSequentialImpulseConstraintSolver());
  bullet_world_.reset(new btDiscreteDynamicsWorld(
      collision_dispatcher_.get(), broadphase_.get(), constraint_solver_.get(),
      collision_configuration_.get()));
  bullet_world_->setGravity(btVector3(0.0f, 0.0f, config_->gravity()));
  bullet_world_->setDebugDrawer(&debug_drawer_);
  bullet_world_->setInternalTickCallback(BulletTickCallback,
                                         static_cast<void*>(this));
  debug_drawer_.set_shader(material_manager->LoadShader(kPhysicsShader));
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
  physics_data->rigid_body->setUserIndex(entity.index());
  physics_data->rigid_body->setUserPointer(entity.container());
  if (physics_def->kinematic()) {
    physics_data->rigid_body->setCollisionFlags(
        physics_data->rigid_body->getCollisionFlags() |
        btCollisionObject::CF_KINEMATIC_OBJECT);
  }

  if (physics_def->offset()) {
    physics_data->offset = LoadVec3(physics_def->offset());
  } else {
    physics_data->offset = mathfu::kZeros3f;
  }

  physics_data->enabled = true;
  bullet_world_->addRigidBody(physics_data->rigid_body.get());

  UpdatePhysicsFromTransform(entity);
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

    if (!physics_data->rigid_body->isKinematicObject()) {
      auto trans = physics_data->rigid_body->getWorldTransform();
      // The quaternion provided by Bullet is using a right-handed coordinate
      // system, while mathfu assumes left. Thus the axes need to be negated.
      // It also needs to be normalized, as the provided value is not.
      transform_data->orientation = mathfu::quat(
          trans.getRotation().getW(), -trans.getRotation().getX(),
          -trans.getRotation().getY(), -trans.getRotation().getZ());
      transform_data->orientation.Normalize();

      vec3 offset =
          transform_data->orientation.Inverse() * physics_data->offset;
      transform_data->position =
          mathfu::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(),
                       trans.getOrigin().getZ()) -
          offset;
    } else {
      // If it is kinematic, instead copy from the transform into physics.
      UpdatePhysicsFromTransform(iter->entity);
    }
  }
}

void BulletTickCallback(btDynamicsWorld* world, btScalar /* time_step */) {
  PhysicsComponent* pc =
      static_cast<PhysicsComponent*>(world->getWorldUserInfo());
  pc->ProcessBulletTickCallback();
}

void PhysicsComponent::ProcessBulletTickCallback() {
  // Check for collisions
  int num_manifolds = collision_dispatcher_->getNumManifolds();
  for (int manifold_index = 0; manifold_index < num_manifolds;
       manifold_index++) {
    btPersistentManifold* contact_manifold =
        collision_dispatcher_->getManifoldByIndexInternal(manifold_index);

    int num_contacts = contact_manifold->getNumContacts();
    for (int contact_index = 0; contact_index < num_contacts; contact_index++) {
      btManifoldPoint& pt = contact_manifold->getContactPoint(contact_index);
      if (pt.getDistance() < 0.0f) {
        auto body_a = contact_manifold->getBody0();
        auto body_b = contact_manifold->getBody1();
        auto container_a =
            static_cast<VectorPool<entity::Entity>*>(body_a->getUserPointer());
        auto container_b =
            static_cast<VectorPool<entity::Entity>*>(body_b->getUserPointer());
        // Only generate events if both containers were defined
        if (container_a == nullptr || container_b == nullptr) {
          continue;
        }

        entity::EntityRef entity_a(container_a, body_a->getUserIndex());
        entity::EntityRef entity_b(container_b, body_b->getUserIndex());
        vec3 position_a(pt.getPositionWorldOnA().x(),
                        pt.getPositionWorldOnA().y(),
                        pt.getPositionWorldOnA().z());
        vec3 position_b(pt.getPositionWorldOnB().x(),
                        pt.getPositionWorldOnB().y(),
                        pt.getPositionWorldOnB().z());

        event_manager_->BroadcastEvent(
            CollisionPayload(entity_a, position_a, entity_b, position_b));
      }
    }
  }
}

// Physics component requires that you have a transform component:
void PhysicsComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void PhysicsComponent::CleanupEntity(entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  if (physics_data->enabled) {
    physics_data->enabled = false;
    bullet_world_->removeRigidBody(physics_data->rigid_body.get());
  }
}

void PhysicsComponent::EnablePhysics(const entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  if (!physics_data->enabled) {
    physics_data->enabled = true;
    bullet_world_->addRigidBody(physics_data->rigid_body.get());
  }
}

void PhysicsComponent::DisablePhysics(const entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  if (physics_data->enabled) {
    physics_data->enabled = false;
    bullet_world_->removeRigidBody(physics_data->rigid_body.get());
  }
}

void PhysicsComponent::UpdatePhysicsFromTransform(entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  TransformData* transform_data = Data<TransformData>(entity);

  // Bullet assumes a right handed system, while mathfu is left, so the axes
  // need to be negated.
  btQuaternion orientation(-transform_data->orientation.vector().x(),
                           -transform_data->orientation.vector().y(),
                           -transform_data->orientation.vector().z(),
                           transform_data->orientation.scalar());
  vec3 offset = transform_data->orientation.Inverse() * physics_data->offset;
  btVector3 position(transform_data->position.x() + offset.x(),
                     transform_data->position.y() + offset.y(),
                     transform_data->position.z() + offset.z());
  btTransform transform(orientation, position);
  physics_data->rigid_body->setWorldTransform(transform);
  physics_data->motion_state->setWorldTransform(transform);
}

void PhysicsComponent::DebugDrawWorld(Renderer* renderer,
                                      const mathfu::mat4& camera_transform) {
  renderer->model_view_projection() = camera_transform;
  debug_drawer_.set_renderer(renderer);
  bullet_world_->debugDrawWorld();
}

void PhysicsDebugDrawer::drawLine(const btVector3& from, const btVector3& to,
                                  const btVector3& color) {
  if (renderer_ != nullptr) {
    renderer_->color() = vec4(color.x(), color.y(), color.z(), 1.0f);
    if (shader_ != nullptr) {
      shader_->Set(*renderer_);
    }
  }

  static const Attribute attributes[] = {kPosition3f, kEND};
  static const unsigned short indices[] = {0, 1};
  const btVector3 vertices[] = {from, to};
  Mesh::RenderArray(GL_LINES, 2, attributes, sizeof(btVector3),
                    reinterpret_cast<const char*>(vertices), indices);
}

}  // fpl_project
}  // fpl
