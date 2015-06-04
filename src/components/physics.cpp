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
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
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
  AssetManager* asset_manager =
      entity_manager_->GetComponent<ServicesComponent>()->asset_manager();

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
  debug_drawer_.set_shader(asset_manager->LoadShader(kPhysicsShader));
}

PhysicsComponent::~PhysicsComponent() { ClearComponentData(); }

void PhysicsComponent::AddFromRawData(entity::EntityRef& entity,
                                      const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_PhysicsDef);
  auto physics_def = static_cast<const PhysicsDef*>(component_data->data());
  PhysicsData* physics_data = AddEntity(entity);

  if (physics_def->shapes() && physics_def->shapes()->Length() > 0) {
    int shape_count = physics_def->shapes()->Length() > kMaxPhysicsBodies
                          ? kMaxPhysicsBodies
                          : physics_def->shapes()->Length();
    physics_data->body_count = shape_count;
    for (int index = 0; index < shape_count; ++index) {
      auto shape_def = physics_def->shapes()->Get(index);
      auto rb_data = &physics_data->rigid_bodies[index];
      switch (shape_def->data_type()) {
        case BulletShapeUnion_BulletSphereDef: {
          auto sphere_data =
              static_cast<const BulletSphereDef*>(shape_def->data());
          rb_data->shape.reset(new btSphereShape(sphere_data->radius()));
          break;
        }
        case BulletShapeUnion_BulletBoxDef: {
          auto box_data = static_cast<const BulletBoxDef*>(shape_def->data());
          btVector3 half_extents(box_data->half_extents()->x(),
                                 box_data->half_extents()->y(),
                                 box_data->half_extents()->z());
          rb_data->shape.reset(new btBoxShape(btVector3(half_extents)));
          break;
        }
        case BulletShapeUnion_BulletCylinderDef: {
          auto cylinder_data =
              static_cast<const BulletCylinderDef*>(shape_def->data());
          btVector3 half_extents(cylinder_data->half_extents()->x(),
                                 cylinder_data->half_extents()->y(),
                                 cylinder_data->half_extents()->z());
          rb_data->shape.reset(new btCylinderShape(half_extents));
          break;
        }
        case BulletShapeUnion_BulletCapsuleDef: {
          auto capsule_data =
              static_cast<const BulletCapsuleDef*>(shape_def->data());
          rb_data->shape.reset(new btCapsuleShape(capsule_data->radius(),
                                                  capsule_data->height()));
          break;
        }
        case BulletShapeUnion_BulletConeDef: {
          auto cone_data = static_cast<const BulletConeDef*>(shape_def->data());
          rb_data->shape.reset(
              new btConeShape(cone_data->radius(), cone_data->height()));
          break;
        }
        case BulletShapeUnion_BulletStaticPlaneDef: {
          auto plane_data =
              static_cast<const BulletStaticPlaneDef*>(shape_def->data());
          btVector3 normal(plane_data->normal()->x(), plane_data->normal()->y(),
                           plane_data->normal()->z());
          rb_data->shape.reset(
              new btStaticPlaneShape(normal, plane_data->constant()));
          break;
        }
        case BulletShapeUnion_BulletNoShapeDef:
        default: {
          rb_data->shape.reset(new btEmptyShape());
          break;
        }
      }
      rb_data->motion_state.reset(new btDefaultMotionState());
      btScalar mass = shape_def->mass();
      btVector3 inertia(0.0f, 0.0f, 0.0f);
      if (shape_def->data_type() != BulletShapeUnion_BulletNoShapeDef) {
        rb_data->shape->calculateLocalInertia(mass, inertia);
      }
      btRigidBody::btRigidBodyConstructionInfo rigid_body_builder(
          mass, rb_data->motion_state.get(), rb_data->shape.get(), inertia);
      rigid_body_builder.m_restitution = shape_def->restitution();
      rb_data->rigid_body.reset(new btRigidBody(rigid_body_builder));
      rb_data->rigid_body->setUserIndex(entity.index());
      rb_data->rigid_body->setUserPointer(entity.container());

      // Only the first shape can be non-kinematic.
      if (index > 0 || physics_def->kinematic()) {
        rb_data->rigid_body->setCollisionFlags(
            rb_data->rigid_body->getCollisionFlags() |
            btCollisionObject::CF_KINEMATIC_OBJECT);
      }
      if (shape_def->offset()) {
        rb_data->offset = LoadVec3(shape_def->offset());
      } else {
        rb_data->offset = mathfu::kZeros3f;
      }
      rb_data->collision_type = static_cast<short>(shape_def->collision_type());
      rb_data->collides_with = 0;
      if (shape_def->collides_with()) {
        for (auto collides = shape_def->collides_with()->begin();
             collides != shape_def->collides_with()->end(); ++collides) {
          rb_data->collides_with |= static_cast<short>(*collides);
        }
      }

      bullet_world_->addRigidBody(rb_data->rigid_body.get(),
                                  rb_data->collision_type,
                                  rb_data->collides_with);
    }
  }

  physics_data->enabled = true;
  UpdatePhysicsFromTransform(entity);
}

entity::ComponentInterface::RawDataUniquePtr PhysicsComponent::ExportRawData(
    entity::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder builder;
  auto result = PopulateRawData(entity, reinterpret_cast<void*>(&builder));
  flatbuffers::Offset<ComponentDefInstance> component;
  component.o = reinterpret_cast<uint64_t>(result);

  builder.Finish(component);
  return builder.ReleaseBufferPointer();
}

void* PhysicsComponent::PopulateRawData(entity::EntityRef& entity,
                                        void* helper) const {
  const PhysicsData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder* fbb =
      reinterpret_cast<flatbuffers::FlatBufferBuilder*>(helper);
  PhysicsDefBuilder builder(*fbb);

  // TODO(amaurice): populate the PhysicsDef

  auto component = CreateComponentDefInstance(
      *fbb, ComponentDataUnion_PhysicsDef, builder.Finish().Union());
  return reinterpret_cast<void*>(component.o);
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

    if (physics_data->body_count == 0) {
      continue;
    }
    if (!physics_data->rigid_bodies[0].rigid_body->isKinematicObject()) {
      auto trans =
          physics_data->rigid_bodies[0].rigid_body->getWorldTransform();
      // The quaternion provided by Bullet is using a right-handed coordinate
      // system, while mathfu assumes left. Thus the axes need to be negated.
      // It also needs to be normalized, as the provided value is not.
      transform_data->orientation = mathfu::quat(
          trans.getRotation().getW(), -trans.getRotation().getX(),
          -trans.getRotation().getY(), -trans.getRotation().getZ());
      transform_data->orientation.Normalize();

      vec3 offset = transform_data->orientation.Inverse() *
                    physics_data->rigid_bodies[0].offset;
      transform_data->position =
          mathfu::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(),
                       trans.getOrigin().getZ()) -
          offset;
    }
    // Update any kinematic objects with the current transform.
    UpdatePhysicsObjectsTransform(iter->entity, true);
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
  DisablePhysics(entity);
}

void PhysicsComponent::EnablePhysics(const entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  if (!physics_data->enabled) {
    physics_data->enabled = true;
    for (int i = 0; i < physics_data->body_count; i++) {
      auto rb_data = &physics_data->rigid_bodies[i];
      bullet_world_->addRigidBody(rb_data->rigid_body.get(),
                                  rb_data->collision_type,
                                  rb_data->collides_with);
    }
  }
}

void PhysicsComponent::DisablePhysics(const entity::EntityRef& entity) {
  PhysicsData* physics_data = Data<PhysicsData>(entity);
  if (physics_data->enabled) {
    physics_data->enabled = false;
    for (int i = 0; i < physics_data->body_count; i++) {
      auto rb_data = &physics_data->rigid_bodies[i];
      bullet_world_->removeRigidBody(rb_data->rigid_body.get());
    }
  }
}

void PhysicsComponent::UpdatePhysicsFromTransform(entity::EntityRef& entity) {
  // Update all objects on the entity, not just kinematic ones.
  UpdatePhysicsObjectsTransform(entity, false);
}

void PhysicsComponent::UpdatePhysicsObjectsTransform(entity::EntityRef& entity,
                                                     bool kinematic_only) {
  if (Data<PhysicsData>(entity) == nullptr) return;

  PhysicsData* physics_data = Data<PhysicsData>(entity);
  TransformData* transform_data = Data<TransformData>(entity);

  // Bullet assumes a right handed system, while mathfu is left, so the axes
  // need to be negated.
  btQuaternion orientation(-transform_data->orientation.vector().x(),
                           -transform_data->orientation.vector().y(),
                           -transform_data->orientation.vector().z(),
                           transform_data->orientation.scalar());

  for (int i = 0; i < physics_data->body_count; i++) {
    auto rb_data = &physics_data->rigid_bodies[i];
    if (kinematic_only && !rb_data->rigid_body->isKinematicObject()) {
      continue;
    }
    vec3 offset = transform_data->orientation.Inverse() * rb_data->offset;
    btVector3 position(transform_data->position.x() + offset.x(),
                       transform_data->position.y() + offset.y(),
                       transform_data->position.z() + offset.z());
    btTransform transform(orientation, position);
    rb_data->rigid_body->setWorldTransform(transform);
    rb_data->motion_state->setWorldTransform(transform);
  }
}

entity::EntityRef PhysicsComponent::RaycastSingle(mathfu::vec3& start,
                                                  mathfu::vec3& end) {
  btVector3 bt_start = btVector3(start.x(), start.y(), start.z());
  btVector3 bt_end = btVector3(end.x(), end.y(), end.z());
  btCollisionWorld::ClosestRayResultCallback ray_results(bt_start, bt_end);

  bullet_world_->rayTest(bt_start, bt_end, ray_results);
  if (ray_results.hasHit()) {
    auto container = static_cast<VectorPool<entity::Entity>*>(
        ray_results.m_collisionObject->getUserPointer());
    if (container != nullptr) {
      return entity::EntityRef(container,
                               ray_results.m_collisionObject->getUserIndex());
    }
  }
  return entity::EntityRef();
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
