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

#include "components/sound.h"
#include "components/transform.h"
#include "config_generated.h"
#include "events/play_sound.h"
#include "events_generated.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif
#include "game_state.h"
#include "input_config_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "rail_def_generated.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const float kViewportAngle = M_PI / 4.0f;
#ifdef ANDROID_CARDBOARD
static const float kViewportAngleCardboard = M_PI / 2.0f;
#endif  // ANDROID_CARDBOARD
static const float kViewportNearPlane = 1.0f;
static const float kViewportFarPlane = 500.0f;
static const vec4 kGreenishColor(0.05f, 0.2f, 0.1f, 1.0f);

GameState::GameState()
    : main_camera_(),
#ifdef ANDROID_CARDBOARD
      cardboard_camera_(),
#endif
      audio_engine_(),
      event_manager_(EventSinkUnion_Size),
      entity_manager_(),
      entity_factory_(),
      motive_engine_(),
      transform_component_(&entity_factory_),
      rail_denizen_component_(),
      player_component_(),
      player_projectile_component_(),
      render_mesh_component_(),
      physics_component_(),
      active_player_entity_(),
      is_in_cardboard_(false),
      draw_debug_physics_(false) {
}

// count = 5  ==>  low = -2, high = 3  ==> range -2..2
// count = 6  ==>  low = -3, high = 3  ==> range -2..3
static RangeInt GridRange(int count) {
  return RangeInt(-count / 2, (count + 1) / 2);
}

void GameState::Initialize(const vec2i& window_size, const Config& config,
                           const InputConfig& input_config,
                           InputSystem* input_system,
                           MaterialManager* material_manager,
                           FontManager* font_manager,
                           pindrop::AudioEngine* audio_engine) {
  main_camera_.Initialize(kViewportAngle, vec2(window_size), kViewportNearPlane,
                          kViewportFarPlane);
#ifdef ANDROID_CARDBOARD
  int half_width = window_size.x() / 2;
  cardboard_camera_.Initialize(kViewportAngleCardboard,
                               vec2(half_width, window_size.y()),
                               kViewportNearPlane, kViewportFarPlane);
#endif

  motive::SmoothInit::Register();

  config_ = &config;
  input_config_ = &input_config;

  audio_engine_ = audio_engine;
  input_system_ = input_system;
  material_manager_ = material_manager;

  event_manager_.RegisterListener(EventSinkUnion_PlaySound, this);

  entity_manager_.RegisterComponent(&transform_component_);
  entity_manager_.RegisterComponent(&rail_denizen_component_);
  entity_manager_.RegisterComponent(&player_component_);
  entity_manager_.RegisterComponent(&player_projectile_component_);
  entity_manager_.RegisterComponent(&render_mesh_component_);
  entity_manager_.RegisterComponent(&physics_component_);
  entity_manager_.RegisterComponent(&patron_component_);
  entity_manager_.RegisterComponent(&time_limit_component_);
  entity_manager_.RegisterComponent(&audio_listener_component_);
  entity_manager_.RegisterComponent(&sound_component_);
  entity_manager_.RegisterComponent(&attributes_component_);

  std::string rail_def_source;
  if (!LoadFile(config.rail_filename()->c_str(), &rail_def_source)) {
    return;
  }
  const RailDef* rail_def = GetRailDef(rail_def_source.c_str());

  entity_manager_.set_entity_factory(&entity_factory_);

  input_controller_.set_input_config(input_config_);
  input_controller_.set_input_system(input_system_);

  audio_listener_component_.Initialize(audio_engine);
  patron_component_.Initialize(config_, &event_manager_);
  physics_component_.Initialize(&event_manager_, config_, material_manager_);
  player_component_.Initialize(&event_manager_, config_);
  player_projectile_component_.Initialize(&event_manager_);
  rail_denizen_component_.Initialize(&motive_engine_, rail_def,
                                     &event_manager_);
  render_mesh_component_.Initialize(vec3(-10, -20, 20), material_manager);
  attributes_component_.Initialize(input_system_, material_manager_,
                                   font_manager, &event_manager_);
  sound_component_.Initialize(audio_engine);

  // Create entities that are explicitly detailed in `entity_list`.
  for (size_t i = 0; i < config.entity_list()->size(); i++) {
    entity_manager_.CreateEntityFromData(config.entity_list()->Get(i));
  }

  // Create entities in a grid formation, as specified by `entity_grid_list`.
  for (size_t i = 0; i < config.entity_grid_list()->size(); i++) {
    auto grid = config.entity_grid_list()->Get(i);
    auto entity_def = config.entity_defs()->Get(grid->entity());

    const vec3i count = LoadVec3i(grid->count());
    const RangeInt x_range = GridRange(count.x());
    const RangeInt y_range = GridRange(count.y());
    const RangeInt z_range = GridRange(count.z());
    const vec3 grid_position = LoadVec3(grid->position());
    const vec3 grid_separation = LoadVec3(grid->separation());
    const vec3 grid_scale = LoadVec3(grid->scale());

    for (int x = x_range.start(); x < x_range.end(); ++x) {
      for (int y = y_range.start(); y < y_range.end(); ++y) {
        for (int z = z_range.start(); z < z_range.end(); ++z) {
          entity::EntityRef entity =
              entity_manager_.CreateEntityFromData(entity_def);

          TransformData* transform_data =
              entity_manager_.GetComponentData<TransformData>(entity);

          const vec3 coord(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(z));
          transform_data->position = grid_position + coord * grid_separation;
          transform_data->scale = grid_scale;
        }
      }
    }
  }

  // Create entities in a ring formation, as specified by `entity_ring_list`.
  for (size_t i = 0; i < config.entity_ring_list()->size(); i++) {
    auto ring = config.entity_ring_list()->Get(i);
    auto entity_def = config.entity_defs()->Get(ring->entity());
    const vec3 ring_position = LoadVec3(ring->position());
    const vec3 ring_scale = LoadVec3(ring->scale());

    for (int j = 0; j < ring->count(); ++j) {
      entity::EntityRef entity =
          entity_manager_.CreateEntityFromData(entity_def);

      TransformData* transform_data =
          entity_manager_.GetComponentData<TransformData>(entity);

      const Angle position_angle =
          Angle::FromDegrees(ring->position_offset_angle()) +
          Angle::FromWithinThreePi(j * kTwoPi / ring->count());
      const Angle orientation_angle =
          Angle::FromDegrees(ring->orientation_offset_angle()) - position_angle;
      transform_data->position =
          ring_position + ring->radius() * position_angle.ToXYVector();
      transform_data->scale = ring_scale;
      transform_data->orientation =
          quat::FromAngleAxis(orientation_angle.ToRadians(), mathfu::kAxisZ3f);

      if (entity->IsRegisteredForComponent(
              physics_component_.GetComponentId())) {
        physics_component_.UpdatePhysicsFromTransform(entity);
      }
    }
  }

  for (auto iter = player_component_.begin(); iter != player_component_.end();
       ++iter) {
    entity_manager_.GetComponentData<PlayerData>(iter->entity)
        ->set_input_controller(&input_controller_);
  }
  active_player_entity_ = player_component_.begin()->entity;

  world_editor_.reset(new editor::WorldEditor());
  world_editor_->Initialize(config.world_editor_config(), input_system_);
}

void GameState::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_PlaySound: {
      auto* payload = event_payload.ToData<PlaySoundPayload>();
      audio_engine_->PlaySound(payload->sound_name, payload->location);
      break;
    }
    default: { assert(false); }
  }
}

static vec2 AdjustedMouseDelta(const vec2i& raw_delta,
                               const InputConfig& input_config) {
  vec2 delta(raw_delta);
  delta *= input_config.mouse_sensitivity();
  if (!input_config.invert_x()) {
    delta.x() *= -1.0f;
  }
  if (!input_config.invert_y()) {
    delta.y() *= -1.0f;
  }
  return delta;
}

void GameState::UpdateMainCamera() {
  PlayerData* player =
      player_component_.GetComponentData(active_player_entity_);
  RailDenizenData* rail_denizen =
      rail_denizen_component_.GetComponentData(active_player_entity_);

  main_camera_.set_position(rail_denizen->Position());
  main_camera_.set_facing(player->GetFacing());
  main_camera_.set_up(player->GetUp());
}

void GameState::Update(WorldTime delta_time) {
// TODO: Determine cardboard mode thru a menu item, instead of inserting it
#ifdef ANDROID_CARDBOARD
  set_is_in_cardboard(input_system_->cardboard_input().is_in_cardboard());
#endif

  if (world_editor_->is_active()) {
    world_editor_->Update(delta_time);  // also updates camera

    if (input_system_->GetButton(fpl::FPLK_F10).went_down()) {
      world_editor_->Deactivate();
    }
  } else {
    motive_engine_.AdvanceFrame(delta_time);
    entity_manager_.UpdateComponents(delta_time);
    UpdateMainCamera();

    if (input_system_->GetButton(fpl::FPLK_F10).went_down()) {
      world_editor_->Activate(main_camera_);
    }
  }
  if (input_system_->GetButton(fpl::FPLK_F9).went_down()) {
    draw_debug_physics_ = !draw_debug_physics_;
  }
}

void GameState::Render(Renderer* renderer) {
  if (is_in_cardboard_) {
    // If in Cardboard, use the specialized Render function instead
    RenderForCardboard(renderer);
    return;
  }

  const Camera* render_camera =
      world_editor_->is_active() ? world_editor_->GetCamera() : &main_camera_;

  renderer->ClearFrameBuffer(kGreenishColor);
  mat4 camera_transform = render_camera->GetTransformMatrix();
  renderer->model_view_projection() = camera_transform;
  renderer->color() = mathfu::kOnes4f;
  renderer->DepthTest(true);
  renderer->model_view_projection() = camera_transform;

  render_mesh_component_.RenderAllEntities(*renderer, *render_camera);

  if (world_editor_->is_active()) {
    world_editor_->Render(renderer);
  }

  if (draw_debug_physics_) {
    physics_component_.DebugDrawWorld(renderer, camera_transform);
  }
}

void GameState::RenderForCardboard(Renderer* renderer) {
#ifdef ANDROID_CARDBOARD
  auto render_callback = [this, &renderer](const mat4& hmd_viewport_transform) {
    // Update the Cardboard camera with the translation changes from the given
    // transform, which contains the shifts for the eyes.
    const vec3 hmd_translation =
        (hmd_viewport_transform * mathfu::kAxisW4f * hmd_viewport_transform)
            .xyz();
    const vec3 corrected_translation =
        vec3(hmd_translation.x(), -hmd_translation.z(), hmd_translation.y());
    cardboard_camera_.set_position(main_camera_.position() +
                                   corrected_translation);
    cardboard_camera_.set_facing(main_camera_.facing());
    cardboard_camera_.set_up(main_camera_.up());

    auto camera_transform = cardboard_camera_.GetTransformMatrix();
    renderer->model_view_projection() = camera_transform;
    render_mesh_component_.RenderAllEntities(*renderer, cardboard_camera_);
  };

  HeadMountedDisplayRender(input_system_, renderer, kGreenishColor,
                           render_callback);
#else
  (void)renderer;
#endif  // ANDROID_CARDBOARD
}

}  // fpl_project
}  // fpl
