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

#include "world_editor.h"

#include <set>
#include <string>
#include "components_generated.h"
#include "components/physics.h"
#include "components/rendermesh.h"
#include "components/services.h"
#include "components/shadow_controller.h"
#include "components/transform.h"
#include "entity/entity_manager.h"
#include "events_generated.h"
#include "events/editor_event.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/reflection.h"
#include "fplbase/utilities.h"
#include "mathfu/utilities.h"

namespace fpl {
namespace editor {

using fpl_project::EditorComponent;
using fpl_project::EditorData;
using fpl_project::PhysicsComponent;
using fpl_project::RenderMeshComponent;
using fpl_project::ServicesComponent;
using fpl_project::TransformComponent;
using fpl_project::ShadowControllerComponent;

static const float kRaycastDistance = 100.0f;

void WorldEditor::Initialize(const WorldEditorConfig* config,
                             InputSystem* input_system,
                             entity::EntityManager* entity_manager) {
  config_ = config;
  input_system_ = input_system;
  entity_manager_ = entity_manager;
  entity_cycler_.reset(
      new entity::EntityManager::EntityStorageContainer::Iterator{
          entity_manager->end()});
  input_controller_.set_input_system(input_system_);
  input_controller_.set_input_config(config_->input_config());
  LoadSchemaFiles();
}

void WorldEditor::AdvanceFrame(WorldTime delta_time) {
  if (input_mode_ == kMoving) {
    // Allow the camera to look around and move.
    input_controller_.Update();
    camera_.set_facing(input_controller_.facing().Value());
    camera_.set_up(input_controller_.up().Value());

    mathfu::vec3 movement = GetMovement();
    camera_.set_position(camera_.position() + movement * (float)delta_time);

    if (input_controller_.Button(0).Value() && selected_entity_.IsValid()) {
      // TODO: if the mouse is down, switch the editing mode
      // input_mode_ = kEditing;
    }
  } else if (input_mode_ == kEditing) {
    // We have an object selected to edit, so input now moves the object.
    input_controller_.Update();
    if (!input_controller_.Button(0).Value()) {
      input_controller_.facing().SetValue(camera_.facing());
      input_mode_ = kMoving;
    } else {
      // dragging the mouse moves the object up/down/left/right
    }
  }

  // TODO(jsimantov): Only cycle entities that have components we can edit
  // (specifically a TransformComponent for now)
  bool entity_changed = false;
  do {
    if (input_system_->GetButton(FPLK_RIGHTBRACKET).went_down()) {
      // select next entity to edit
      if (*entity_cycler_ != entity_manager_->end()) {
        (*entity_cycler_)++;
      }
      if (*entity_cycler_ == entity_manager_->end()) {
        *entity_cycler_ = entity_manager_->begin();
      }
      entity_changed = true;
    }
    if (input_system_->GetButton(FPLK_LEFTBRACKET).went_down()) {
      if (*entity_cycler_ == entity_manager_->begin()) {
        *entity_cycler_ = entity_manager_->end();
      }
      (*entity_cycler_)--;
      entity_changed = true;
    }
    if (entity_changed) {
      SelectEntity(entity_cycler_->ToReference());
    }
  } while (entity_changed && entity_cycler_->ToReference() != selected_entity_);
  if (input_controller_.Button(0).Value() &&
      input_controller_.Button(0).HasChanged()) {
    mathfu::vec3 start = camera_.position();
    mathfu::vec3 end = start + camera_.facing() * kRaycastDistance;
    entity::EntityRef result =
        entity_manager_->GetComponent<PhysicsComponent>()->RaycastSingle(start,
                                                                         end);
    if (result.IsValid()) {
      *entity_cycler_ = result.ToIterator();
      entity_changed = true;
    }
  }
  if (entity_changed) {
    SelectEntity(entity_cycler_->ToReference());
    LogInfo("Selected entity at %x\n", &(**entity_cycler_));
  }

  auto transform_component =
      entity_manager_->GetComponent<TransformComponent>();
  if (selected_entity_.IsValid()) {
    // We have an entity selected, let's allow it to be modified
    auto raw_data = transform_component->ExportRawData(selected_entity_);
    if (raw_data != nullptr) {
      // TODO(jsimantov): in the future, don't just assume the type of this
      auto component_data =
          flatbuffers::GetMutableRoot<ComponentDefInstance>(raw_data.get());
      TransformDef* transform =
          reinterpret_cast<TransformDef*>(component_data->mutable_data());
      if (ModifyTransformBasedOnInput(transform)) {
        transform_component->AddFromRawData(selected_entity_, component_data);
        entity_manager_->GetComponent<fpl_project::PhysicsComponent>()
            ->UpdatePhysicsFromTransform(selected_entity_);
      }
    }

    if (input_system_->GetButton(FPLK_INSERT).went_down() ||
        input_system_->GetButton(FPLK_v).went_down()) {
      entity::EntityRef new_entity = DuplicateEntity(selected_entity_);
      *entity_cycler_ = new_entity.ToIterator();
      SelectEntity(entity_cycler_->ToReference());
    }
    if (input_system_->GetButton(FPLK_DELETE).went_down() ||
        input_system_->GetButton(FPLK_x).went_down()) {
      entity::EntityRef entity = selected_entity_;
      *entity_cycler_ = entity_manager_->end();
      selected_entity_ = entity::EntityRef();
      DestroyEntity(entity);
    }
  }

  transform_component->PostLoadFixup();

  // Any components we specifically still want to update should be updated here.
  transform_component->UpdateAllEntities(0);
  entity_manager_->GetComponent<ShadowControllerComponent>()->UpdateAllEntities(
      0);
  entity_manager_->GetComponent<RenderMeshComponent>()->UpdateAllEntities(0);

  entity_manager_->DeleteMarkedEntities();
}

void WorldEditor::HighlightEntity(const entity::EntityRef& entity, float tint) {
  if (!entity.IsValid()) return;
  auto render_data =
      entity_manager_->GetComponentData<fpl_project::RenderMeshData>(entity);
  if (render_data != nullptr) {
    render_data->tint = vec4(tint, tint, tint, 1);
  }
  // Highlight the node's children as well.
  auto transform_data =
      entity_manager_->GetComponentData<fpl_project::TransformData>(entity);

  if (transform_data != nullptr) {
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      // Highlight the child, but slightly less brightly.
      HighlightEntity(iter->owner, 1 + ((tint - 1) * .8));
    }
  }
}

void WorldEditor::SelectEntity(const entity::EntityRef& entity_ref) {
  if (entity_ref.IsValid()) {
    auto data = entity_manager_->GetComponentData<EditorData>(entity_ref);
    if (data != nullptr) {
      if (data->ignore_selection > 0) return;
      LogInfo("Highlighting entity '%s' with prototype '%s'",
              data->entity_id.c_str(), data->prototype.c_str());
    }
    HighlightEntity(entity_ref, 2);
  }
  if (selected_entity_.IsValid() && selected_entity_ != entity_ref) {
    // un-highlight the old one
    HighlightEntity(selected_entity_, 1);
  }
  selected_entity_ = entity_ref;
}

void WorldEditor::Render(Renderer* /*renderer*/) {
  // Render any editor-specific things
}

void WorldEditor::SetInitialCamera(const fpl_project::Camera& initial_camera) {
  camera_ = initial_camera;
  camera_.set_viewport_angle(config_->viewport_angle_degrees() *
                             (M_PI / 180.0f));
}

void WorldEditor::Activate() {
  // Set up the initial camera position.
  input_controller_.facing().SetValue(camera_.facing());
  input_controller_.up().SetValue(camera_.up());

  input_mode_ = kMoving;
  *entity_cycler_ = entity_manager_->end();

  input_system_->SetRelativeMouseMode(true);

  // Raise the EditorStart event.
  auto event_manager =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  if (event_manager != nullptr) {
    event_manager->BroadcastEvent(
        fpl_project::EditorEventPayload(fpl_project::EditorEventAction_Enter));
  }
}

void WorldEditor::Deactivate() {
  // Raise the EditorExit event.
  auto event_manager =
      entity_manager_->GetComponent<ServicesComponent>()->event_manager();
  if (event_manager != nullptr) {
    event_manager->BroadcastEvent(
        fpl_project::EditorEventPayload(fpl_project::EditorEventAction_Exit));
  }

  // de-select all entities
  SelectEntity(entity::EntityRef());

  *entity_cycler_ = entity_manager_->end();
}

void WorldEditor::SaveWorld() {
  auto editor_component = entity_manager_->GetComponent<EditorComponent>();
  // get a list of all filenames in the world
  std::set<std::string> filenames;
  for (auto entity = editor_component->begin();
       entity != editor_component->end(); ++entity) {
    filenames.insert(entity->data.source_file);
  }
  for (auto iter = filenames.begin(); iter != filenames.end(); ++iter) {
    SaveEntitiesInFile(*iter);
  }
}

flatbuffers::Offset<EntityDef> WorldEditor::SerializeEntity(
    entity::EntityRef& entity, flatbuffers::FlatBufferBuilder* builder,
    const reflection::Schema* schema) {
  auto editor_component = entity_manager_->GetComponent<EditorComponent>();

  std::vector<flatbuffers::Offset<ComponentDefInstance>> component_vector;

  for (int component_id = 0; component_id < entity::kMaxComponentCount;
       component_id++) {
    const EditorData* editor_data = editor_component->GetComponentData(entity);
    if (entity_manager_->GetComponent(component_id) != nullptr &&
        entity->IsRegisteredForComponent(component_id) &&
        editor_data->components_from_prototype.find(
            (ComponentDataUnion)component_id) ==
            editor_data->components_from_prototype.end()) {
#ifdef USE_COPYTABLE
      auto exported =
          entity_manager_->GetComponent(component_id)->ExportRawData(entity);
      auto table_def =
          schema != nullptr
              ? schema->objects()->LookupByKey("ComponentDefInstance")
              : nullptr;

      if (exported != nullptr) {
        flatbuffers::Offset<ComponentDefInstance> component =
            exported != nullptr && table_def != nullptr
                ? flatbuffers::Offset<ComponentDefInstance>(
                      flatbuffers::CopyTable(
                          *builder, *schema, *table_def,
                          (const flatbuffers::Table&)(*exported))
                          .o)
                : 0;

        component_vector.push_back(component);
      }
#else
      auto exported = entity_manager_->GetComponent(component_id)
                          ->PopulateRawData(entity, builder);
      if (exported != nullptr) {
        flatbuffers::Offset<ComponentDefInstance> component;
        component.o = reinterpret_cast<uint64_t>(exported);
        component_vector.push_back(component);
      }
      (void)schema;
#endif
    }
  }
  return CreateEntityDef(*builder, builder->CreateVector(component_vector));
}

entity::EntityRef WorldEditor::DuplicateEntity(entity::EntityRef& entity) {
  if (schema_data_ == "") {
    LogInfo("No binary schema loaded, can't duplicate entity.");
    return entity::EntityRef();
  }

  auto schema = reflection::GetSchema(schema_data_.c_str());

  flatbuffers::FlatBufferBuilder builder;
  auto entity_def_offset = SerializeEntity(entity, &builder, schema);
  builder.Finish(entity_def_offset);
  const EntityDef* entity_def =
      flatbuffers::GetRoot<EntityDef>(builder.GetBufferPointer());
  entity::EntityRef new_entity = entity_manager_->CreateEntityFromData(
      static_cast<const void*>(entity_def));
  // remove the entity ID
  EditorData* old_editor_data =
      entity_manager_->GetComponentData<EditorData>(entity);
  EditorData* editor_data =
      entity_manager_->GetComponentData<EditorData>(new_entity);
  if (editor_data != nullptr) {
    editor_data->entity_id = "";
    if (old_editor_data != nullptr)
      editor_data->source_file = old_editor_data->source_file;
  }
  entity_manager_->GetComponent<TransformComponent>()->PostLoadFixup();
  return new_entity;
}

void WorldEditor::DestroyEntity(entity::EntityRef& entity) {
  entity_manager_->DeleteEntity(entity);
}

void WorldEditor::LoadSchemaFiles() {
  const char* schema_file_text = config_->schema_file_text()->c_str();
  const char* schema_file_binary = config_->schema_file_binary()->c_str();

  if (!LoadFile(schema_file_binary, &schema_data_)) {
    LogInfo("Failed to open binary schema file: %s", schema_file_binary);
    return;
  }
  auto schema = reflection::GetSchema(schema_data_.c_str());
  if (schema != nullptr) {
    LogInfo("WorldEditor: Binary schema %s loaded", schema_file_binary);
  }
  if (LoadFile(schema_file_text, &schema_text_)) {
    LogInfo("WorldEditor: Text schema %s loaded", schema_file_text);
  }
}

void WorldEditor::SaveEntitiesInFile(const std::string& filename) {
  auto editor_component = entity_manager_->GetComponent<EditorComponent>();
  if (schema_data_ == "") {
    LogInfo("No binary schema loaded, can't save entities.");
    return;
  }
  auto schema = reflection::GetSchema(schema_data_.c_str());

  if (filename == "") {
    LogInfo("Skipping saving entities to blank filename.");
    return;
  }
  LogInfo("Saving entities in file: '%s'", filename.c_str());
  // We know the FlatBuffer format we're using: components
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<fpl::EntityDef>> entity_vector;

  // loop through all entities
  for (auto entityiter = entity_manager_->begin();
       entityiter != entity_manager_->end(); ++entityiter) {
    entity::EntityRef entity = entityiter.ToReference();
    const EditorData* editor_data = editor_component->GetComponentData(entity);
    if (editor_data != nullptr && editor_data->source_file == filename) {
      entity_vector.push_back(SerializeEntity(entity, &builder, schema));
    }
  }

  auto entity_list =
      CreateEntityListDef(builder, builder.CreateVector(entity_vector));
  builder.Finish(entity_list);
  if (SaveFile((filename + ".bin").c_str(),
               static_cast<const void*>(builder.GetBufferPointer()),
               static_cast<size_t>(builder.GetSize()))) {
    LogInfo("Save (binary) successful.");
  } else {
    LogInfo("Save (binary) failed.");
  }
  // Now save to JSON file.
  // First load and parse the flatbuffer schema, then generate text.
  if (schema_text_ != "") {
    // Make a list of include paths that parser.Parse can parse.
    // char** with nullptr termination.
    std::unique_ptr<const char*> include_paths;
    size_t num_paths = config_->schema_include_paths()->size();
    include_paths.reset(new const char*[num_paths + 1]);
    for (size_t i = 0; i < num_paths; i++) {
      include_paths.get()[i] = config_->schema_include_paths()->Get(i)->c_str();
    }
    include_paths.get()[num_paths] = nullptr;
    flatbuffers::Parser parser;
    if (parser.Parse(schema_text_.c_str(), include_paths.get(),
                     config_->schema_file_text()->c_str())) {
      std::string json;
      GenerateText(parser, builder.GetBufferPointer(),
                   flatbuffers::GeneratorOptions(), &json);
      if (SaveFile((filename + ".json").c_str(), json)) {
        LogInfo("Save (JSON) successful");
      } else {
        LogInfo("Save (JSON) failed.");
      }
    } else {
      LogInfo("Couldn't parse schema file: %s", parser.error_.c_str());
    }
  } else {
    LogInfo("No text schema loaded, can't save JSON file.");
  }
}

mathfu::vec3 WorldEditor::GetMovement() {
  // Get a movement vector to move the user forward, up, or right.
  // Movement is always relative to the camera facing, but parallel to
  // ground.
  float forward_speed = 0;
  float up_speed = 0;
  float right_speed = 0;
  // TODO(jsimantov): make the specific keys configurable?
  if (input_system_->GetButton(FPLK_w).is_down()) {
    forward_speed += config_->camera_movement_speed();
  }
  if (input_system_->GetButton(FPLK_s).is_down()) {
    forward_speed -= config_->camera_movement_speed();
  }
  if (input_system_->GetButton(FPLK_d).is_down()) {
    right_speed += config_->camera_movement_speed();
  }
  if (input_system_->GetButton(FPLK_a).is_down()) {
    right_speed -= config_->camera_movement_speed();
  }
  if (input_system_->GetButton(FPLK_r).is_down()) {
    up_speed += config_->camera_movement_speed();
  }
  if (input_system_->GetButton(FPLK_f).is_down()) {
    up_speed -= config_->camera_movement_speed();
  }

  // Translate the keypresses into movement parallel to the ground plane.
  mathfu::vec3 movement = mathfu::kZeros3f;
  // TODO(jsimantov): Currently assumes Z is up. This should be
  // configurable.
  const float epsilon = 0.00001;
  mathfu::vec3 forward = {camera_.facing().xy(), 0};

  // If we're in gimbal lock, don't allow movement.
  if (forward.LengthSquared() > epsilon) {
    forward.Normalize();
    mathfu::vec3 up = mathfu::kAxisZ3f;
    mathfu::vec3 right =
        mathfu::vec3::CrossProduct(camera_.facing(), camera_.up());
    if (right.LengthSquared() > epsilon) {
      right.Normalize();
      movement += forward * forward_speed;
      movement += up * up_speed;
      movement += right * right_speed;
    }
  }
  return movement;
}

bool WorldEditor::ModifyTransformBasedOnInput(TransformDef* transform) {
  // IJKL = move x/y axis
  float x_speed = 0, y_speed = 0, z_speed = 0;
  float roll_speed = 0, pitch_speed = 0, yaw_speed = 0;
  float scale_speed = 1;
  if (input_system_->GetButton(FPLK_i).is_down()) {
    x_speed += config_->object_movement_speed();
  }
  if (input_system_->GetButton(FPLK_k).is_down()) {
    x_speed -= config_->object_movement_speed();
  }
  if (input_system_->GetButton(FPLK_j).is_down()) {
    y_speed += config_->object_movement_speed();
  }
  if (input_system_->GetButton(FPLK_l).is_down()) {
    y_speed -= config_->object_movement_speed();
  }
  // P; = move z axis
  if (input_system_->GetButton(FPLK_p).is_down()) {
    z_speed += config_->object_movement_speed();
  }
  if (input_system_->GetButton(FPLK_SEMICOLON).is_down()) {
    z_speed -= config_->object_movement_speed();
  }
  // UO = roll
  if (input_system_->GetButton(FPLK_u).is_down()) {
    roll_speed += config_->object_angular_speed();
  }
  if (input_system_->GetButton(FPLK_o).is_down()) {
    roll_speed -= config_->object_angular_speed();
  }
  // YH = pitch
  if (input_system_->GetButton(FPLK_y).is_down()) {
    pitch_speed += config_->object_angular_speed();
  }
  if (input_system_->GetButton(FPLK_h).is_down()) {
    pitch_speed -= config_->object_angular_speed();
  }
  // NM = yaw
  if (input_system_->GetButton(FPLK_n).is_down()) {
    yaw_speed += config_->object_angular_speed();
  }
  if (input_system_->GetButton(FPLK_m).is_down()) {
    yaw_speed -= config_->object_angular_speed();
  }
  // +- = scale
  if (input_system_->GetButton(FPLK_EQUALS).is_down()) {
    scale_speed = config_->object_scale_speed();
  } else if (input_system_->GetButton(FPLK_MINUS).is_down()) {
    scale_speed = 1.0f / config_->object_scale_speed();
  }
  Vec3 position = *(transform->mutable_position());
  position = Vec3{position.x() + x_speed, position.y() + y_speed,
                  position.z() + z_speed};

  Vec3 orientation = *transform->mutable_orientation();
  orientation = Vec3{orientation.x() + pitch_speed,
                     orientation.y() + roll_speed, orientation.z() + yaw_speed};
  Vec3 scale = *transform->scale();
  if (input_system_->GetButton(FPLK_0).is_down()) {
    scale = Vec3{1.0f, 1.0f, 1.0f};
    scale_speed = 0;  // to trigger returning true
  } else {
    scale = Vec3{scale.x() * scale_speed, scale.y() * scale_speed,
                 scale.z() * scale_speed};
  }
  if (x_speed != 0 || y_speed != 0 || z_speed != 0 || yaw_speed != 0 ||
      roll_speed != 0 || pitch_speed != 0 || scale_speed != 1) {
    *(transform->mutable_position()) = position;
    *(transform->mutable_orientation()) = orientation;
    *(transform->mutable_scale()) = scale;
    return true;
  }
  return false;
}

}  // namespace editor
}  // namespace fpl
