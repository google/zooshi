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

#include "transform.h"
#include "components/physics.h"
#include <math.h>

namespace fpl {
namespace fpl_project {

static const float kDegreesToRadians = M_PI / 180.0f;

mathfu::vec3 TransformComponent::WorldPosition(entity::EntityRef entity) {
  TransformData* transform_data = Data<TransformData>(entity);
  if (transform_data->parent) {
    return WorldTransform(transform_data->parent) * transform_data->position;
  } else {
    return transform_data->position;
  }
}

mathfu::quat TransformComponent::WorldOrientation(entity::EntityRef entity) {
  TransformData* transform_data = Data<TransformData>(entity);
  if (transform_data->parent) {
    return transform_data->orientation *
           WorldOrientation(transform_data->parent);
  } else {
    return transform_data->orientation;
  }
}

mathfu::mat4 TransformComponent::WorldTransform(entity::EntityRef entity) {
  TransformData* transform_data = Data<TransformData>(entity);
  if (transform_data->parent) {
    return WorldTransform(transform_data->parent) *
           transform_data->GetTransformMatrix();
  } else {
    return transform_data->GetTransformMatrix();
  }
}

void TransformComponent::InitEntity(entity::EntityRef& entity) {
  TransformData* transform_data = Data<TransformData>(entity);
  transform_data->owner = entity;
}

void TransformComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    TransformData* transform_data = Data<TransformData>(iter->entity);
    // Go through and start updating everything that has no parent:
    if (!transform_data->parent) {
      UpdateWorldPosition(iter->entity, mathfu::mat4::Identity());
    }
  }
}

void TransformComponent::UpdateWorldPosition(entity::EntityRef& entity,
                                             mathfu::mat4 transform) {
  TransformData* transform_data = GetComponentData(entity);
  transform_data->world_transform =
      transform * transform_data->GetTransformMatrix();

  for (auto iter = transform_data->children.begin();
       iter != transform_data->children.end(); ++iter) {
    UpdateWorldPosition(iter->owner, transform_data->world_transform);
  }
}

void TransformComponent::CleanupEntity(entity::EntityRef& entity) {
  // Remove and cleanup children, if any exist:
  TransformData* transform_data = GetComponentData(entity);
  if (transform_data) {
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      entity_manager_->DeleteEntity(iter->owner);
    }
  }
}

void TransformComponent::AddFromRawData(entity::EntityRef& entity,
                                        const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_TransformDef);
  auto transform_def = static_cast<const TransformDef*>(component_data->data());
  auto pos = transform_def->position();
  auto orientation = transform_def->orientation();
  auto scale = transform_def->scale();
  TransformData* transform_data = AddEntity(entity);
  // TODO: Move vector loading into a function in fplbase.
  if (pos != nullptr) {
    transform_data->position = mathfu::vec3(pos->x(), pos->y(), pos->z());
  }
  if (orientation != nullptr) {
    transform_data->orientation = mathfu::quat::FromEulerAngles(
        mathfu::vec3(orientation->x(), orientation->y(), orientation->z()) *
        kDegreesToRadians);
  }
  if (scale != nullptr) {
    transform_data->scale = mathfu::vec3(scale->x(), scale->y(), scale->z());
  }

  // The physics component is initialized first, so it needs to be updated with
  // the correct initial transform.
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  if (entity->IsRegisteredForComponent(physics_component->GetComponentId())) {
    physics_component->UpdatePhysicsFromTransform(entity);
  }

  if (transform_def->children() != nullptr) {
    for (size_t i = 0; i < transform_def->children()->size(); i++) {
      entity::EntityRef child = entity_factory_->CreateEntityFromData(
          transform_def->children()->Get(i), entity_manager_);
      // CreateEntityFromData can resize the underlying data, invalidating the
      // pointer to the transform data of the parent entity, so refresh it.
      transform_data = GetComponentData(entity);
      TransformData* child_transform_data = AddEntity(child);
      transform_data->children.push_back(*child_transform_data);
      child_transform_data->parent = entity;
    }
  }
}

entity::ComponentInterface::RawDataUniquePtr TransformComponent::ExportRawData(
    entity::EntityRef& entity) const {
  flatbuffers::FlatBufferBuilder builder;
  const TransformData* data = GetComponentData(entity);
  mathfu::vec3 euler = data->orientation.ToEulerAngles();
  fpl::Vec3 position{data->position.x(), data->position.y(),
                     data->position.z()};
  fpl::Vec3 scale{data->scale.x(), data->scale.y(), data->scale.z()};
  fpl::Vec3 orientation{euler.x(), euler.y(), euler.z()};
  auto transform_def =
      CreateTransformDef(builder, &position, &scale, &orientation);
  builder.Finish(transform_def);

  return builder.ReleaseBufferPointer();
}

void TransformComponent::AddChild(entity::EntityRef& child,
                                  entity::EntityRef& parent) {
  TransformData* child_data = GetComponentData(child);
  TransformData* parent_data = GetComponentData(parent);

  // If child is already someone else's child, break that link first.
  if (child_data->parent) {
    RemoveChild(child);
  }
  parent_data->children.push_back(*child_data);
  child_data->parent = parent;
}

void TransformComponent::RemoveChild(entity::EntityRef& child) {
  TransformData* child_data = GetComponentData(child);
  assert(child_data->parent);

  child_data->parent = entity::EntityRef();
  child_data->child_node.remove();
}

}  // fpl_project
}  // fpl
