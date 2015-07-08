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

#include <math.h>
#include "transform.h"
#include "components/physics.h"
#include "components/editor.h"
#include "components/services.h"
#include "fplbase/utilities.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::TransformComponent,
                            fpl::fpl_project::TransformData)

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
    entity::EntityRef entity = iter->entity;
    TransformData* transform_data = Data<TransformData>(entity);
    // Go through and start updating everything that has no parent:
    if (!transform_data->parent) {
      UpdateWorldPosition(entity, mathfu::mat4::Identity());
    }
  }
}

void TransformComponent::PostLoadFixup() {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef entity = iter->entity;
    UpdateChildLinks(entity);
  }
}

void TransformComponent::UpdateChildLinks(entity::EntityRef& entity) {
  TransformData* transform_data = Data<TransformData>(entity);
  if (transform_data->pending_child_ids.size() != 0) {
    // Now connect up the children we had listed, as they should
    // all be loaded.
    std::vector<std::string> pending_child_ids =
        transform_data->pending_child_ids;
    // Clear out the original list.
    transform_data->pending_child_ids.clear();

    for (size_t i = 0; i < pending_child_ids.size(); ++i) {
      std::string child_id = pending_child_ids[i];

      // Check if there's an active entity with the matching child_id.
      // If so, just connect it up as a child.
      // Otherwise, instantiate a prototype with that entity ID and map it
      // as a child.
      entity::EntityRef child = entity_manager_->GetComponent<EditorComponent>()
                                    ->GetEntityFromDictionary(child_id);

      if (!child.IsValid()) {
        // Do some FlatBuffering to create a nearly-empty EntityDef
        // which only contains one component: an EditorDef specifying
        // the prototype to use for the child.
        child =
            entity_manager_->GetComponent<ServicesComponent>()
                ->entity_factory()
                ->CreateEntityFromPrototype(child_id.c_str(), entity_manager_);
      }
      // If we got an existing child (or created a new one)...
      if (child.IsValid()) {
        AddEntity(child);
        AddChild(child, entity);
        // transform_data->children.push_back(*child_transform_data);
        // child_transform_data->parent = entity;
      }
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
  auto transform_def = static_cast<const TransformDef*>(raw_data);
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

  if (transform_def->child_ids() != nullptr) {
    for (size_t i = 0; i < transform_def->child_ids()->size(); i++) {
      auto child_id = transform_def->child_ids()->Get(i);
      // We don't actually add the children until the first update,
      // to give the other entities in our list time to be loaded.
      if (transform_data->child_ids.find(child_id->c_str()) ==
          transform_data->child_ids.end()) {
        transform_data->pending_child_ids.push_back(child_id->c_str());
        transform_data->child_ids.insert(child_id->c_str());
      }
    }
  }
}

entity::ComponentInterface::RawDataUniquePtr TransformComponent::ExportRawData(
    entity::EntityRef& entity) const {
  const TransformData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  mathfu::vec3 euler = data->orientation.ToEulerAngles() / kDegreesToRadians;
  fpl::Vec3 position{data->position.x(), data->position.y(),
                     data->position.z()};
  fpl::Vec3 scale{data->scale.x(), data->scale.y(), data->scale.z()};
  fpl::Vec3 orientation{euler.x(), euler.y(), euler.z()};

  std::vector<flatbuffers::Offset<flatbuffers::String>> child_ids_vector;
  for (auto iter = data->child_ids.begin(); iter != data->child_ids.end();
       ++iter) {
    child_ids_vector.push_back(fbb.CreateString(*iter));
  }

  auto child_ids =
      (child_ids_vector.size() > 0) ? fbb.CreateVector(child_ids_vector) : 0;

  TransformDefBuilder builder(fbb);
  builder.add_position(&position);
  builder.add_scale(&scale);
  builder.add_orientation(&orientation);
  if (child_ids_vector.size() > 0) {
    builder.add_child_ids(child_ids);
  }

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
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
