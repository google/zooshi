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

#ifndef COMPONENTS_RENDERMESH_H_
#define COMPONENTS_RENDERMESH_H_

#include "camera.h"
#include "components_generated.h"
#include "components/transform.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"
#include "fplbase/mesh.h"
#include "fplbase/renderer.h"
#include "fplbase/shader.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct RenderMeshData {
 public:
  RenderMeshData() : mesh(nullptr) {}
  Mesh* mesh;
  Shader* shader;
  mathfu::mat4 tint;
};

class RenderMeshComponent : public entity::Component<RenderMeshData> {
 public:
  RenderMeshComponent() {}

  // todo(ccornell): Write this so we can specify things from
  // flatbuffer data files!  Either via a flatbuffer mesh
  // format, or loading things up from disk once the importer
  // pipeline is working!
  virtual void AddFromRawData(entity::EntityRef& /*entity*/,
                              const void* /*raw_data*/) {}

  virtual void InitEntity(entity::EntityRef& /*entity*/);

  virtual void Init() { assert(entity_manager_ != nullptr); }

  // Nothing really happens per-update to these things.
  virtual void UpdateAllEntities(entity::WorldTime /*delta_time*/) {}

  // Here's where the real stuff goes on.  Called directly by
  // the render function.
  void RenderEntity(entity::EntityRef& entity, mat4 transform,
                    Renderer& renderer, const Camera& camera);
  void RenderAllEntities(Renderer& renderer, const Camera& camera);

  // Get and set the light position.  This is a special uniform that is sent
  // to all shaders without having to declare it explicitly in the
  // shader_instance variable.
  mathfu::vec3 light_position() { return light_position_; }
  void set_light_position(mathfu::vec3 light_position) {
    light_position_ = light_position;
  }

 private:
  // todo(ccornell) expand this if needed - make an array for multiple lights.
  // Also maybe make this into a full fledged struct to store things like
  // intensity, color, etc.  (Low priority - none of our shaders support
  // these.)
  mathfu::vec3 light_position_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RenderMeshComponent,
                              fpl::fpl_project::RenderMeshData,
                              ComponentDataUnion_RenderMeshDef)

#endif  // COMPONENTS_RENDERMESH_H_
