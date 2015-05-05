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

#include "audio_config_generated.h"
#include "entity/entity.h"
#include "fplbase/input.h"
#include "fplbase/input.h"
#include "fplbase/utilities.h"
#include "fplbase/utilities.h"
#include "game_state.h"
#include "gui.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/math/angle.h"
#include "pindrop/pindrop.h"

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
static const int kCubeNumVertices = 24;
static const int kCubeNumIndices = 36;

static const unsigned int kQuadIndices[] = {0, 1, 2, 2, 1, 3};
// clang-format off
static const unsigned int kCubeIndices[] = {
    0,  3,  2,  2,  1,  0,
    9,  10, 6,  6,  5,  9,
    11, 7,  14, 14, 18, 11,
    13, 22, 15, 15, 4,  13,
    12, 23, 19, 19, 8,  12,
    16, 17, 21, 21, 20, 16 };
// clang-format on

static const Attribute kQuadMeshFormat[] = {kPosition3f, kTexCoord2f, kNormal3f,
                                            kTangent4f, kEND};

static const char kAssetsDir[] = "assets";

static const char kConfigFileName[] = "config.bin";

#ifdef __ANDROID__
static const int kAndroidMaxScreenWidth = 1920;
static const int kAndroidMaxScreenHeight = 1080;
#endif

// static const float kPixelToWorldScale = 0.008f;
static const float kPixelToWorldScale = 4.0f / 256.0f;

static const vec3 kCameraPos = vec3(4, 5, -10);
static const vec3 kCameraOrientation = vec3(0, 0, 0);
static const vec3 kLightPos = vec3(0, 20, 20);

static const vec3 kCardboardAmbient = vec3(0.75f);
static const vec3 kCardboardDiffuse = vec3(0.85f);
static const vec3 kCardboardSpecular = vec3(0.3f);
static const float kCardboardShininess = 32.0f;
static const float kCardboardNormalMapScale = 0.3f;

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

// Factory method for the entity manager, for converting data (in our case.
// flatbuffer definitions) into entities and sticking them into the system.
entity::EntityRef ZooshiEntityFactory::CreateEntityFromData(
    const void* data, entity::EntityManager* entity_manager) {
  const EntityDef* def = static_cast<const EntityDef*>(data);
  assert(def != nullptr);
  entity::EntityRef entity = entity_manager->AllocateNewEntity();
  for (size_t i = 0; i < def->component_list()->size(); i++) {
    const ComponentDefInstance* currentInstance = def->component_list()->Get(i);
    entity::ComponentInterface* component =
        entity_manager->GetComponent(currentInstance->data_type());
    assert(component != nullptr);
    component->AddFromRawData(entity, currentInstance);
  }
  return entity;
}

Game::Game()
    : matman_(renderer_),
      shader_lit_textured_normal_(nullptr),
      shader_textured_(nullptr),
      prev_world_time_(0),
      game_state_() {
  version_ = kVersion;
}

Game::~Game() {}

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
  Material* material = matman_.LoadMaterial(material_name);
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

static void CreateCube(const vec3& offset, const float size, const vec2&,
                       NormalMappedVertex* vertices) {
  const float half_width = size * 0.5f;
  const vec3 low = offset + vec3(-half_width, -half_width, 0.0f);
  const vec3 high = offset + vec3(half_width, half_width, size);

  for (int i = 0; i < 3; i++) {
    vertices[0 + (i * 8)].pos = vec3(low[0], low[1], low[2]);
    vertices[1 + (i * 8)].pos = vec3(high[0], low[1], low[2]);
    vertices[2 + (i * 8)].pos = vec3(high[0], high[1], low[2]);
    vertices[3 + (i * 8)].pos = vec3(low[0], high[1], low[2]);
    vertices[4 + (i * 8)].pos = vec3(low[0], low[1], high[2]);
    vertices[5 + (i * 8)].pos = vec3(high[0], low[1], high[2]);
    vertices[6 + (i * 8)].pos = vec3(high[0], high[1], high[2]);
    vertices[7 + (i * 8)].pos = vec3(low[0], high[1], high[2]);
  }

  for (int i = 0; i < 36; i += 6) {
    vertices[kCubeIndices[i]].tc = vec2(1, 1);
    vertices[kCubeIndices[i + 1]].tc = vec2(1, 0);
    vertices[kCubeIndices[i + 2]].tc = vec2(0, 0);
    vertices[kCubeIndices[i + 4]].tc = vec2(0, 1);
  }

  Mesh::ComputeNormalsTangents(vertices, &kCubeIndices[0], kCubeNumVertices,
                               kCubeNumIndices);
}

// Creates a mesh of a single quad (two triangles) vertically upright.
// The quad's has x and y size determined by the size of the texture.
// The quad is offset in (x,y,z) space by the 'offset' variable.
// Returns a mesh with the quad and texture, or nullptr if anything went wrong.
Mesh* Game::CreateCubeMesh(const char* material_name, const vec3& offset,
                           const float pixel_bounds,
                           float pixel_to_world_scale) {
  // Don't try to load obviously invalid materials. Suppresses error logs from
  // the material manager.
  if (material_name == nullptr || material_name[0] == '\0') return nullptr;

  // Load the material from file, and check validity.
  Material* material = matman_.LoadMaterial(material_name);
  bool material_valid = material != nullptr && material->textures().size() > 0;
  if (!material_valid) return nullptr;

  // Create vertex geometry in proportion to the texture size.
  // This is nice for the artist since everything is at the scale of the
  // original artwork.
  assert(pixel_bounds);
  const vec2 texture_size = vec2(mathfu::RoundUpToPowerOf2(pixel_bounds),
                                 mathfu::RoundUpToPowerOf2(pixel_bounds));
  const vec2 texture_coord_size = pixel_bounds / texture_size;
  const float geo_size = pixel_bounds * pixel_to_world_scale;

  // Initialize a vertex array in the requested position.
  NormalMappedVertex vertices[kCubeNumVertices];
  CreateCube(offset, geo_size, texture_coord_size, vertices);

  // Create mesh and add in quad indices.
  Mesh* mesh = new Mesh(vertices, kCubeNumVertices, sizeof(NormalMappedVertex),
                        kQuadMeshFormat);
  mesh->AddIndices(kCubeIndices, kCubeNumIndices, material);
  return mesh;
}

static const char* kCubeMaterial = "materials/pixel1x1.bin";
static const char* kGuyMaterial = "materials/guy.bin";
static const char* kGuyBackMaterial = "materials/guy_back.bin";

// Load textures for cardboard into 'materials_'. The 'renderer_' and 'matman_'
// members have been initialized at this point.
bool Game::InitializeAssets() {
  matman_.LoadMaterial(kCubeMaterial);
  matman_.LoadMaterial(kGuyMaterial);
  matman_.LoadMaterial(kGuyBackMaterial);
  matman_.StartLoadingTextures();

  shader_lit_textured_normal_ =
      matman_.LoadShader("shaders/lit_textured_normal");
  shader_cardboard_ = matman_.LoadShader("shaders/cardboard");
  shader_textured_ = matman_.LoadShader("shaders/textured");

  // uniforms:
  shader_cardboard_->SetUniform("ambient_material", kCardboardAmbient);
  shader_cardboard_->SetUniform("diffuse_material", kCardboardDiffuse);
  shader_cardboard_->SetUniform("specular_material", kCardboardSpecular);
  shader_cardboard_->SetUniform("shininess", kCardboardShininess);
  shader_cardboard_->SetUniform("normalmap_scale", kCardboardNormalMapScale);

  return true;
}

const Config& Game::GetConfig() const {
  return *fpl::fpl_project::GetConfig(config_source_.c_str());
}

const InputConfig& Game::GetInputConfig() const {
  return *fpl::fpl_project::GetInputConfig(input_config_source_.c_str());
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool Game::Initialize(const char* const binary_directory) {
  // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Zooshi initializing...\n");

  if (!ChangeToUpstreamDir(binary_directory, kAssetsDir)) return false;

  if (!LoadFile(kConfigFileName, &config_source_)) return false;

  if (!InitializeRenderer()) return false;

  if (!InitializeAssets()) return false;

  if (!LoadFile(GetConfig().input_config()->c_str(), &input_config_source_))
    return false;

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
  while (!matman_.TryFinalize()) {
  }

  cube_ = CreateCubeMesh(kCubeMaterial, vec3(0, 0, 0), 128, kPixelToWorldScale);

  billboard_ = CreateVerticalQuadMesh(kGuyMaterial, vec3(0, 0, 0),
                                      vec2(256, 256), kPixelToWorldScale);

  input_.SetRelativeMouseMode(true);

  game_state_.Initialize(renderer_.window_size(), GetConfig(), cube_, shader_cardboard_);

  LogInfo("Initialization complete\n");
  return true;
}

void Game::Render() {
  renderer_.AdvanceFrame(input_.minimized_);
  renderer_.ClearFrameBuffer(mathfu::kZeros4f);
  game_state_.Render(&renderer_);
}

void Game::Render2DElements(mathfu::vec2i resolution) {
  // Set up an ortho camera for all 2D elements, with (0, 0) in the top left,
  // and the bottom right the windows size in pixels.
  auto res = renderer_.window_size();
  auto ortho_mat = mathfu::OrthoHelper<float>(0.0f, static_cast<float>(res.x()),
                                              static_cast<float>(res.y()), 0.0f,
                                              -1.0f, 1.0f);
  renderer_.model_view_projection() = ortho_mat;

  // Update the currently drawing Google Play Games image. Displays "Sign In"
  // when currently signed-out, and "Sign Out" when currently signed in.

  Material* material = matman_.LoadMaterial(kGuyMaterial);
  const vec2 window_size = vec2(resolution);
  const float texture_scale = 1.0f;
  const vec2 texture_size =
      texture_scale * vec2(material->textures()[0]->size());
  const vec2 position_percent = vec2(0.08f, 0.80f);
  const vec2 position = window_size * position_percent;

  const vec3 position3d(position.x(), position.y(), -0.5f);
  const vec3 texture_size3d(texture_size.x(), -texture_size.y(), 0.0f);

  shader_textured_->Set(renderer_);
  material->Set(renderer_);

  Mesh::RenderAAQuadAlongX(position3d - texture_size3d * 0.5f,
                           position3d + texture_size3d * 0.5f, vec2(0, 1),
                           vec2(1, 0));
}

static inline WorldTime CurrentWorldTime() { return GetTicks(); }

void Game::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  prev_world_time_ = CurrentWorldTime() - kMinUpdateTime;

  while (!input_.exit_requested_ &&
         !input_.GetButton(FPLK_ESCAPE).went_down()) {
    // Milliseconds elapsed since last update. To avoid burning through the
    // CPU, enforce a minimum time between updates. For example, if
    // min_update_time is 1, we will not exceed 1000Hz update time.
    const WorldTime world_time = CurrentWorldTime();
    const WorldTime delta_time =
        std::min(world_time - prev_world_time_, kMaxUpdateTime);
    prev_world_time_ = world_time;
    if (delta_time < kMinUpdateTime) {
      Delay(kMinUpdateTime - delta_time);
    }

    game_state_.Update(delta_time, input_, GetInputConfig());

    Render();
    Render2DElements(renderer_.window_size());
    audio_engine_.AdvanceFrame(delta_time / 1000.0f);
    // Process input device messages since the last game loop.
    // Update render window size.
    input_.AdvanceFrame(&renderer_.window_size());

// TEMP: testing GUI on top of everything else.
#if (IMGUI_TEST)
    // Open OpenType font
    static FontManager fontman;
    if (!fontman.FontLoaded()) {
      fontman.Open("fonts/NotoSansCJKjp-Bold.otf");
      fontman.SetRenderer(renderer_);
    }
    gui::TestGUI(matman_, fontman, input_);
#endif  // IMGUI_TEST
  }
}

}  // fpl_project
}  // fpl

