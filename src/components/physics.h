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

#include "btBulletDynamicsCommon.h"
#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "event_system/event_manager.h"
#include "fplbase/material_manager.h"
#include "fplbase/renderer.h"
#include "fplbase/shader.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct PhysicsData {
 public:
  std::unique_ptr<btCollisionShape> shape;
  std::unique_ptr<btMotionState> motion_state;
  std::unique_ptr<btRigidBody> rigid_body;

  mathfu::vec3 Velocity() const {
    const btVector3 vel = rigid_body->getLinearVelocity();
    return mathfu::vec3(vel.x(), vel.y(), vel.z());
  }
  void SetVelocity(mathfu::vec3 velocity) {
    const btVector3 vel(velocity.x(), velocity.y(), velocity.z());
    rigid_body->setLinearVelocity(vel);
  }
  mathfu::vec3 AngularVelocity() const {
    const btVector3 vel = rigid_body->getAngularVelocity();
    return mathfu::vec3(vel.x(), vel.y(), vel.z());
  }
  void SetAngularVelocity(mathfu::vec3 velocity) {
    const btVector3 vel(velocity.x(), velocity.y(), velocity.z());
    rigid_body->setAngularVelocity(vel);
  }
};

// Used by Bullet to render the physics scene as a wireframe.
class PhysicsDebugDrawer : public btIDebugDraw {
 public:
  virtual void drawLine(const btVector3& from, const btVector3& to,
                        const btVector3& color);
  virtual int getDebugMode() const { return DBG_DrawWireframe; }

  virtual void drawContactPoint(const btVector3& /*pointOnB*/,
                                const btVector3& /*normalOnB*/,
                                btScalar /*distance*/, int /*lifeTime*/,
                                const btVector3& /*color*/) {}
  virtual void reportErrorWarning(const char* /*warningString*/) {}
  virtual void draw3dText(const btVector3& /*location*/,
                          const char* /*textString*/) {}
  virtual void setDebugMode(int /*debugMode*/) {}

  Shader* shader() { return shader_; }
  void set_shader(Shader* shader) { shader_ = shader; }

  Renderer* renderer() { return renderer_; }
  void set_renderer(Renderer* renderer) { renderer_ = renderer; }

 private:
  Shader* shader_;
  Renderer* renderer_;
};

class PhysicsComponent : public entity::Component<PhysicsData> {
 public:
  PhysicsComponent() {}
  virtual ~PhysicsComponent();

  void Initialize(event::EventManager* event_manager, const Config* config,
                  MaterialManager* material_manager);

  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);

  // TODO: Implement ExportRawData function for editor (b/21589546)

  virtual void InitEntity(entity::EntityRef& /*entity*/);

  virtual void CleanupEntity(entity::EntityRef& entity);

  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  void UpdatePhysicsFromTransform(entity::EntityRef& entity);

  void DebugDrawWorld(Renderer* renderer, const mathfu::mat4& camera_transform);

  btDiscreteDynamicsWorld* bullet_world() { return bullet_world_.get(); }

 private:
  event::EventManager* event_manager_;

  const Config* config_;

  std::unique_ptr<btDiscreteDynamicsWorld> bullet_world_;
  std::unique_ptr<btBroadphaseInterface> broadphase_;
  std::unique_ptr<btDefaultCollisionConfiguration> collision_configuration_;
  std::unique_ptr<btCollisionDispatcher> collision_dispatcher_;
  std::unique_ptr<btSequentialImpulseConstraintSolver> constraint_solver_;

  PhysicsDebugDrawer debug_drawer_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::PhysicsComponent,
                              fpl::fpl_project::PhysicsData,
                              ComponentDataUnion_PhysicsDef)

#endif  // COMPONENTS_PHYSICS_H_
