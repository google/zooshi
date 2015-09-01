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

#include "game.h"

#include <math.h>
#include <stdarg.h>

#include "assets_generated.h"
#include "audio_config_generated.h"
#include "common.h"
#include "entity/entity.h"
#include "event/graph.h"
#include "event/log.h"
#include "events/play_sound.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "graph_factory.h"
#include "graph_generated.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "modules/audio.h"
#include "modules/debug.h"
#include "modules/entity.h"
#include "modules/logic.h"
#include "modules/math.h"
#include "modules/rail_denizen.h"
#include "modules/string.h"
#include "modules/vec3.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/math/angle.h"
#include "pindrop/pindrop.h"
#include "world.h"

#ifdef __ANDROID__
#include "fplbase/renderer_android.h"
#endif

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace fpl_project {

static const int kQuadNumVertices = 4;
static const int kQuadNumIndices = 6;

static const unsigned short kQuadIndices[] = {0, 1, 2, 2, 1, 3};

static const Attribute kQuadMeshFormat[] = {kPosition3f, kTexCoord2f, kNormal3f,
                                            kTangent4f, kEND};

static const char kAssetsDir[] = "assets";

static const char kConfigFileName[] = "config.bin";

#ifdef __ANDROID__
static const int kAndroidMaxScreenWidth = 1920;
static const int kAndroidMaxScreenHeight = 1080;
#endif

static const vec3 kCameraPos = vec3(4, 5, -10);
static const vec3 kCameraOrientation = vec3(0, 0, 0);
static const vec3 kLightPos = vec3(0, 20, 20);

static const int kMinUpdateTime = 1000 / 60;
static const int kMaxUpdateTime = 1000 / 30;

/// kVersion is used by Google developers to identify which
/// applications uploaded to Google Play are derived from this application.
/// This allows the development team at Google to determine the popularity of
/// this application.
/// How it works: Applications that are uploaded to the Google Play Store are
/// scanned for this version string.  We track which applications are using it
/// to measure popularity.  You are free to remove it (of course) but we would
/// appreciate if you left it in.
static const char kVersion[] = "FPL Project 0.0.1";

Game::Game()
    : asset_manager_(renderer_),
      event_manager_(EventSinkUnion_Size),
      shader_lit_textured_normal_(nullptr),
      shader_textured_(nullptr),
      prev_world_time_(0),
      audio_config_(nullptr),
      world_(),
      version_(kVersion),
      fps_frame_counter_(0),
      fps_time_counter_(0) {}

// Initialize the 'renderer_' member. No other members have been initialized at
// this point.
bool Game::InitializeRenderer() {
#ifdef __ANDROID__
  const vec2i window_size(kAndroidMaxScreenWidth, kAndroidMaxScreenHeight);
#else
  const vec2i window_size(1200, 800);
#endif
  if (!renderer_.Initialize(window_size, "Window Title!")) {
    LogError("Renderer initialization error: %s\n",
             renderer_.last_error().c_str());
    return false;
  }

  renderer_.color() = mathfu::kOnes4f;
  // Initialize the first frame as black.
  renderer_.ClearFrameBuffer(mathfu::kZeros4f);

#ifdef ANDROID_CARDBOARD
  vec2i size = AndroidGetScalerResolution();
  const vec2i viewport_size =
      size.x() && size.y() ? size : renderer_.window_size();
  InitializeUndistortFramebuffer(viewport_size.x(), viewport_size.y());
#endif
  return true;
}

// Initializes 'vertices' at the specified position, aligned up-and-down.
// 'vertices' must be an array of length kQuadNumVertices.
static void CreateVerticalQuad(const vec3& offset, const vec2& geo_size,
                               const vec2& texture_coord_size,
                               NormalMappedVertex* vertices) {
  const float half_width = geo_size[0] * 0.5f;
  const vec3 bottom_left = offset + vec3(-half_width, 0.0f, 0.0f);
  const vec3 top_right = offset + vec3(half_width, 0.0f, geo_size[1]);

  vertices[0].pos = bottom_left;
  vertices[1].pos = vec3(top_right[0], offset[1], bottom_left[2]);
  vertices[2].pos = vec3(bottom_left[0], offset[1], top_right[2]);
  vertices[3].pos = top_right;

  const float coord_half_width = texture_coord_size[0] * 0.5f;
  const vec2 coord_bottom_left(0.5f - coord_half_width, 1.0f);
  const vec2 coord_top_right(0.5f + coord_half_width,
                             1.0f - texture_coord_size[1]);

  vertices[0].tc = coord_bottom_left;
  vertices[1].tc = vec2(coord_top_right[0], coord_bottom_left[1]);
  vertices[2].tc = vec2(coord_bottom_left[0], coord_top_right[1]);
  vertices[3].tc = coord_top_right;

  Mesh::ComputeNormalsTangents(vertices, &kQuadIndices[0], kQuadNumVertices,
                               kQuadNumIndices);
}

// Creates a mesh of a single quad (two triangles) vertically upright.
// The quad's has x and y size determined by the size of the texture.
// The quad is offset in (x,y,z) space by the 'offset' variable.
// Returns a mesh with the quad and texture, or nullptr if anything went wrong.
Mesh* Game::CreateVerticalQuadMesh(const char* material_name,
                                   const vec3& offset, const vec2& pixel_bounds,
                                   float pixel_to_world_scale) {
  // Don't try to load obviously invalid materials. Suppresses error logs from
  // the material manager.
  if (material_name == nullptr || material_name[0] == '\0') return nullptr;

  // Load the material from file, and check validity.
  Material* material = asset_manager_.LoadMaterial(material_name);
  bool material_valid = material != nullptr && material->textures().size() > 0;
  if (!material_valid) return nullptr;

  // Create vertex geometry in proportion to the texture size.
  // This is nice for the artist since everything is at the scale of the
  // original artwork.
  assert(pixel_bounds.x() && pixel_bounds.y());
  const vec2 texture_size = vec2(mathfu::RoundUpToPowerOf2(pixel_bounds.x()),
                                 mathfu::RoundUpToPowerOf2(pixel_bounds.y()));
  const vec2 texture_coord_size = pixel_bounds / texture_size;
  const vec2 geo_size = pixel_bounds * vec2(pixel_to_world_scale);

  // Initialize a vertex array in the requested position.
  NormalMappedVertex vertices[kQuadNumVertices];
  CreateVerticalQuad(offset, geo_size, texture_coord_size, vertices);

  // Create mesh and add in quad indices.
  Mesh* mesh = new Mesh(vertices, kQuadNumVertices, sizeof(NormalMappedVertex),
                        kQuadMeshFormat);
  mesh->AddIndices(kQuadIndices, kQuadNumIndices, material);
  return mesh;
}

// Load textures for cardboard into 'materials_'. The 'renderer_' and
// 'asset_manager_' members have been initialized at this point.
bool Game::InitializeAssets() {
  // Load up all of our assets, as defined in the manifest.
  // TODO - put this into an asynchronous loding function, like we
  // had in pie noon.
  const AssetManifest& asset_manifest = GetAssetManifest();
  for (size_t i = 0; i < asset_manifest.mesh_list()->size(); i++) {
    asset_manager_.LoadMesh(asset_manifest.mesh_list()->Get(i)->c_str());
  }
  for (size_t i = 0; i < asset_manifest.shader_list()->size(); i++) {
    asset_manager_.LoadShader(asset_manifest.shader_list()->Get(i)->c_str());
  }
  for (size_t i = 0; i < asset_manifest.material_list()->size(); i++) {
    asset_manager_.LoadMaterial(
        asset_manifest.material_list()->Get(i)->c_str());
  }
  asset_manager_.StartLoadingTextures();

  shader_lit_textured_normal_ =
      asset_manager_.LoadShader("shaders/lit_textured_normal");
  shader_textured_ = asset_manager_.LoadShader("shaders/textured");

  // Load animation table:
  motive::AnimTable& anim_table = world_.animation_component.anim_table();
  anim_table.InitFromFlatBuffers(*asset_manifest.anims());

  // Load all animations referenced by the animation table:
  const char* anim_not_found = LoadAnimTableAnimations(&anim_table, LoadFile);
  if (anim_not_found != nullptr) {
    LogError("Failed to load animation file %s.\n", anim_not_found);
    return false;
  }

  return true;
}

const Config& Game::GetConfig() const {
  return *fpl::fpl_project::GetConfig(config_source_.c_str());
}

const InputConfig& Game::GetInputConfig() const {
  return *fpl::fpl_project::GetInputConfig(input_config_source_.c_str());
}

const AssetManifest& Game::GetAssetManifest() const {
  return *fpl::fpl_project::GetAssetManifest(asset_manifest_source_.c_str());
}

void Game::InitializeEventSystem() {
  event::RegisterLogFunc(
      [](const char* fmt, va_list args) { LogError(fmt, args); });

  event::TypeRegistry<bool>::RegisterType("Bool");
  event::TypeRegistry<int>::RegisterType("Int");
  event::TypeRegistry<float>::RegisterType("Float");
  event::TypeRegistry<std::string>::RegisterType("String");
  event::TypeRegistry<RailDenizenDataRef>::RegisterType("RailDenizenDataRef");
  event::TypeRegistry<entity::EntityRef>::RegisterType("Entity");
  event::TypeRegistry<mathfu::vec3>::RegisterType("Vec3");
  event::TypeRegistry<pindrop::Channel>::RegisterType("Channel");

  InitializeAudioModule(&event_system_, &audio_engine_);
  InitializeDebugModule(&event_system_);
  InitializeEntityModule(&event_system_, &world_.services_component,
                         &world_.meta_component);
  InitializeLogicModule(&event_system_);
  InitializeMathModule(&event_system_);
  InitializeRailDenizenModule(&event_system_, &world_.rail_denizen_component);
  InitializeStringModule(&event_system_);
  InitializeVec3Module(&event_system_);
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool Game::Initialize(const char* const binary_directory) {
  LogInfo("Zooshi Initializing...");

  input_.Initialize();

  if (!ChangeToUpstreamDir(binary_directory, kAssetsDir)) return false;

  if (!LoadFile(kConfigFileName, &config_source_)) return false;

  if (!InitializeRenderer()) return false;

  if (!LoadFile(GetConfig().input_config()->c_str(), &input_config_source_))
    return false;

  if (!LoadFile(GetConfig().assets_filename()->c_str(),
                &asset_manifest_source_)) {
    return false;
  }

  if (!InitializeAssets()) return false;

  // Some people are having trouble loading the audio engine, and it's not
  // strictly necessary for gameplay, so don't die if the audio engine fails to
  // initialize.
  if (!audio_engine_.Initialize(GetConfig().audio_config()->c_str())) {
    LogError("Failed to initialize audio engine.\n");
  }

  if (!audio_engine_.LoadSoundBank("sound_banks/sound_assets.bin")) {
    LogError("Failed to load sound bank.\n");
  }

  // Wait for everything to finish loading...
  while (!asset_manager_.TryFinalize()) {
  }

  event_manager_.RegisterListener(EventSinkUnion_PlaySound, this);

  InitializeEventSystem();

  const auto &asset_manifest = GetAssetManifest();
  for (size_t i = 0; i < asset_manifest.font_list()->size(); i++) {
    font_manager_.Open(asset_manifest.font_list()->Get(i)->c_str());
  }
  font_manager_.SetRenderer(renderer_);

  SetRelativeMouseMode(true);

  input_controller_.set_input_system(&input_);
  input_controller_.set_input_config(&GetInputConfig());

  world_.Initialize(GetConfig(), &input_, &asset_manager_, &world_renderer_,
                    &font_manager_, &audio_engine_, &event_manager_,
                    &event_system_, &graph_dictionary_, &renderer_);

  world_renderer_.Initialize(&world_);

  world_editor_.reset(new editor::WorldEditor());
  world_editor_->Initialize(GetConfig().world_editor_config(),
                            &world_.entity_manager);

  world_editor_->AddComponentToUpdate(
      component_library::TransformComponent::GetComponentId());
  world_editor_->AddComponentToUpdate(
      ShadowControllerComponent::GetComponentId());
  world_editor_->AddComponentToUpdate(
      component_library::RenderMeshComponent::GetComponentId());

  const Config* config = &GetConfig();
  pause_state_.Initialize(&input_, &world_, config, &asset_manager_,
                          &font_manager_, &audio_engine_);
  gameplay_state_.Initialize(&input_, &world_, config, &GetInputConfig(),
                             world_editor_.get(), &audio_engine_);
  game_menu_state_.Initialize(&input_, &world_, &input_controller_,
                              &GetConfig(), &asset_manager_, &font_manager_,
                              &GetAssetManifest(),
                              &audio_engine_);
  world_editor_state_.Initialize(&renderer_, &input_, world_editor_.get(),
                                 &world_);

  state_machine_.AssignState(kGameStateGameplay, &gameplay_state_);
  state_machine_.AssignState(kGameStatePause, &pause_state_);
  state_machine_.AssignState(kGameStateGameMenu, &game_menu_state_);
  state_machine_.AssignState(kGameStateWorldEditor, &world_editor_state_);
  state_machine_.SetCurrentStateId(kGameStateGameMenu);

  LogInfo("Initialization complete\n");
  return true;
}

void Game::SetRelativeMouseMode(bool relative_mouse_mode) {
  relative_mouse_mode_ = relative_mouse_mode;
  input_.SetRelativeMouseMode(relative_mouse_mode);
}

void Game::ToggleRelativeMouseMode() {
  relative_mouse_mode_ = !relative_mouse_mode_;
  input_.SetRelativeMouseMode(relative_mouse_mode_);
}

static inline WorldTime CurrentWorldTime() { return GetTicks(); }

void Game::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  prev_world_time_ = CurrentWorldTime() - kMinUpdateTime;

  while (!input_.exit_requested() && !state_machine_.done()) {
    // Milliseconds elapsed since last update. To avoid burning through the
    // CPU, enforce a minimum time between updates. For example, if
    // min_update_time is 1, we will not exceed 1000Hz update time.
    const WorldTime world_time = CurrentWorldTime();
    const WorldTime delta_time =
        std::min(world_time - prev_world_time_, kMaxUpdateTime);
    fps_frame_counter_++;
    fps_time_counter_ += world_time - prev_world_time_;

    prev_world_time_ = world_time;
    if (delta_time < kMinUpdateTime) {
      Delay(kMinUpdateTime - delta_time);
    }

    if (fps_time_counter_ >= 1000) {
      // Show a count of how many frames we actually rendered during the
      // previous second.
      //LogInfo("Running at %d FPS", fps_frame_counter_);
      // Set it as an attribute on the player so we can show it on screen.
      if (world_.active_player_entity) {
        AttributesData* attrib_data =
            (world_.entity_manager.GetComponent<AttributesComponent>() !=
             nullptr)
                ? world_.entity_manager.GetComponentData<AttributesData>(
                      world_.active_player_entity)
                : nullptr;

        if (attrib_data != nullptr)
          attrib_data->attributes[AttributeDef_FramesPerSecond] =
              fps_frame_counter_;
      }
      fps_time_counter_ -= 1000;
      fps_frame_counter_ = 0;
    }

    input_.AdvanceFrame(&renderer_.window_size());

    state_machine_.AdvanceFrame(delta_time);

    state_machine_.Render(&renderer_);

    renderer_.AdvanceFrame(input_.minimized(), input_.Time());

    audio_engine_.AdvanceFrame(delta_time / 1000.0f);

    // Process input device messages since the last game loop.
    // Update render window size.
    if (input_.GetButton(fpl::FPLK_BACKQUOTE).went_down()) {
      ToggleRelativeMouseMode();
    }
  }
}

void Game::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_PlaySound: {
      auto* payload = event_payload.ToData<PlaySoundPayload>();
      audio_engine_.PlaySound(payload->sound_name, payload->location);
      break;
    }
    default: { assert(false); }
  }
}

}  // fpl_project
}  // fpl
