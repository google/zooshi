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

#include "anim_generated.h"
#include "assets_generated.h"
#include "audio_config_generated.h"
#include "breadboard/graph.h"
#include "breadboard/log.h"
#include "common.h"
#include "entity/entity.h"
#include "fplbase/input.h"
#include "fplbase/systrace.h"
#include "fplbase/utilities.h"
#include "graph_generated.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "modules/attributes.h"
#include "modules/audio.h"
#include "modules/debug.h"
#include "modules/entity.h"
#include "modules/logic.h"
#include "modules/math.h"
#include "modules/physics.h"
#include "modules/rail_denizen.h"
#include "modules/state.h"
#include "modules/string.h"
#include "modules/vec3.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/math/angle.h"
#include "motive/util/benchmark.h"
#include "pindrop/pindrop.h"
#include "world.h"
#include "zooshi_graph_factory.h"

// To be replaced with SDL threads:
#include "pthread.h"

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

// Mutexes/CVs used in synchronizing the render and update threads:
pthread_mutex_t Game::renderthread_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Game::updatethread_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Game::gameupdate_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Game::start_render_cv_ = PTHREAD_COND_INITIALIZER;
pthread_cond_t Game::start_update_cv_ = PTHREAD_COND_INITIALIZER;

static const int kQuadNumVertices = 4;
static const int kQuadNumIndices = 6;

static const unsigned short kQuadIndices[] = {0, 1, 2, 2, 1, 3};

static const Attribute kQuadMeshFormat[] = {kPosition3f, kTexCoord2f, kNormal3f,
                                            kTangent4f, kEND};

static const char kAssetsDir[] = "assets";

static const char kConfigFileName[] = "config.bin";

#ifdef __ANDROID__
static const int kAndroidMaxScreenWidth = 1280;
static const int kAndroidMaxScreenHeight = 720;
#endif

static const vec3 kCameraPos = vec3(4, 5, -10);
static const vec3 kCameraOrientation = vec3(0, 0, 0);
static const vec3 kLightPos = vec3(0, 20, 20);

static const int kMinUpdateTime = 1000 / 60;
static const int kMaxUpdateTime = 1000 / 30;

// Codes used in systrace logging.  Their values don't
// actually matter much as long as they're unique.
static const int kUpdateGameStateCode = 555;
static const int kUpdateRenderPrepCode = 556;

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
      graph_factory_(&event_system_, &LoadFile, &audio_engine_),
      shader_lit_textured_normal_(nullptr),
      shader_textured_(nullptr),
      prev_render_time_(0),
      game_exiting_(false),
      audio_config_(nullptr),
      world_(),
      version_(kVersion) {}

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

// In our game, `anim_name` is the same as the animation file. Use it directly
// to load the file into scratch_buf, and return the pointer to it.
const motive::RigAnimFb* LoadRigAnim(const char* anim_name,
                                     std::string* scratch_buf) {
  const bool load_ok = LoadFile(anim_name, scratch_buf);
  if (!load_ok) {
    LogError("Failed to load animation file %s.\n", anim_name);
    return nullptr;
  }
  return motive::GetRigAnimFb(scratch_buf->c_str());
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

  // Load the animation table and all animations it references.
  motive::AnimTable& anim_table = world_.animation_component.anim_table();
  const bool anim_ok =
      anim_table.InitFromFlatBuffers(*asset_manifest.anims(), LoadRigAnim);
  if (!anim_ok) return false;

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
  breadboard::RegisterLogFunc(
      [](const char* fmt, va_list args) { LogError(fmt, args); });

  breadboard::TypeRegistry<void>::RegisterType("Pulse");
  breadboard::TypeRegistry<bool>::RegisterType("Bool");
  breadboard::TypeRegistry<int>::RegisterType("Int");
  breadboard::TypeRegistry<float>::RegisterType("Float");
  breadboard::TypeRegistry<std::string>::RegisterType("String");
  breadboard::TypeRegistry<RailDenizenDataRef>::RegisterType(
      "RailDenizenDataRef");
  breadboard::TypeRegistry<AttributesDataRef>::RegisterType(
      "AttributesDataRef");
  breadboard::TypeRegistry<entity::EntityRef>::RegisterType("Entity");
  breadboard::TypeRegistry<mathfu::vec3>::RegisterType("Vec3");
  breadboard::TypeRegistry<pindrop::SoundHandle>::RegisterType("SoundHandle");
  breadboard::TypeRegistry<pindrop::Channel>::RegisterType("Channel");

  InitializeAttributesModule(&event_system_, &world_.attributes_component,
                             &world_.graph_component);
  InitializeAudioModule(&event_system_, &audio_engine_);
  InitializeDebugModule(&event_system_);
  InitializeEntityModule(&event_system_, &world_.services_component,
                         &world_.meta_component);
  InitializeLogicModule(&event_system_);
  InitializeMathModule(&event_system_);
  InitializePhysicsModule(&event_system_, &world_.physics_component);
  InitializeRailDenizenModule(&event_system_, &world_.rail_denizen_component,
                              &world_.graph_component);
  InitializeStateModule(&event_system_, gameplay_state_.requested_state());
  InitializeStringModule(&event_system_);
  InitializeVec3Module(&event_system_);
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool Game::Initialize(const char* const binary_directory) {
  LogInfo("Zooshi Initializing...");
  InitBenchmarks(10);

  input_.Initialize();
  SystraceInit();

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

  InitializeEventSystem();

  const auto& asset_manifest = GetAssetManifest();
  for (size_t i = 0; i < asset_manifest.font_list()->size(); i++) {
    font_manager_.Open(asset_manifest.font_list()->Get(i)->c_str());
  }
  font_manager_.SetRenderer(renderer_);

  SetRelativeMouseMode(true);

  input_controller_.set_input_system(&input_);
  input_controller_.set_input_config(&GetInputConfig());

  world_editor_.reset(new editor::WorldEditor());

  world_.Initialize(GetConfig(), &input_, &asset_manager_, &world_renderer_,
                    &font_manager_, &audio_engine_, &graph_factory_, &renderer_,
                    world_editor_.get());

  world_renderer_.Initialize(&world_);

  world_editor_->Initialize(GetConfig().world_editor_config(),
                            &world_.entity_manager, &font_manager_);

  world_editor_->AddComponentToUpdate(
      component_library::TransformComponent::GetComponentId());
  world_editor_->AddComponentToUpdate(
      ShadowControllerComponent::GetComponentId());
  world_editor_->AddComponentToUpdate(
      component_library::RenderMeshComponent::GetComponentId());

  gpg_manager_.Initialize(false);

  const Config* config = &GetConfig();
  pause_state_.Initialize(&input_, &world_, config, &asset_manager_,
                          &font_manager_, &audio_engine_);
  gameplay_state_.Initialize(&input_, &world_, config, &GetInputConfig(),
                             world_editor_.get(), &audio_engine_);
  game_menu_state_.Initialize(
      &input_, &world_, &input_controller_, &GetConfig(), &asset_manager_,
      &font_manager_, &GetAssetManifest(), &gpg_manager_, &audio_engine_);
  intro_state_.Initialize(&input_, &world_);
  world_editor_state_.Initialize(&renderer_, &input_, world_editor_.get(),
                                 &world_);

  state_machine_.AssignState(kGameStateGameplay, &gameplay_state_);
  state_machine_.AssignState(kGameStatePause, &pause_state_);
  state_machine_.AssignState(kGameStateGameMenu, &game_menu_state_);
  state_machine_.AssignState(kGameStateIntro, &intro_state_);
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

// Stuff the update thread needs to know about:
struct UpdateThreadData {
  UpdateThreadData(bool* exiting, World* world_ptr,
                   StateMachine<kGameStateCount>* statemachine_ptr,
                   Renderer* renderer_ptr,
                   pindrop::AudioEngine* audio_engine_ptr)
      : game_exiting(exiting),
        world(world_ptr),
        state_machine(statemachine_ptr),
        renderer(renderer_ptr),
        audio_engine(audio_engine_ptr) {}
  bool* game_exiting;
  World* world;
  StateMachine<kGameStateCount>* state_machine;
  Renderer* renderer;
  pindrop::AudioEngine* audio_engine;
};

// This is the therad that handles all of our actual game logic updates:
static void* UpdateThread(void* data) {
  UpdateThreadData* rt_data = static_cast<UpdateThreadData*>(data);
  int prev_update_time;
  prev_update_time = CurrentWorldTime() - kMinUpdateTime;
#ifdef __ANDROID__
  JavaVM* jvm;
  JNIEnv* env = AndroidGetJNIEnv();
  env->GetJavaVM(&jvm);

  JNIEnv* update_env;

  JavaVMAttachArgs args;
  args.version = JNI_VERSION_1_6;  // choose your JNI version
  args.name = "Zooshi Update";
  args.group =
      nullptr;  // you might want to assign the java thread to a ThreadGroup
  jvm->AttachCurrentThread(&update_env, &args);
#endif  //__ANDROID__

  pthread_mutex_lock(&Game::updatethread_mutex_);
  while (!*(rt_data->game_exiting)) {
    pthread_cond_wait(&Game::start_update_cv_, &Game::updatethread_mutex_);

    // -------------------------------------------
    // Step 5b.  (See comment at the start of Run()
    // Update everything.  This is only called once everything has been
    // safely loaded up into openGL and the renderthread is working its way
    // through actually putting everything on the screen.
    // -------------------------------------------
    pthread_mutex_lock(&Game::gameupdate_mutex_);
    const WorldTime world_time = CurrentWorldTime();
    const WorldTime delta_time =
        std::min(world_time - prev_update_time, kMaxUpdateTime);
    prev_update_time = world_time;

    SystraceAsyncBegin("UpdateGameState", kUpdateGameStateCode);
    rt_data->state_machine->AdvanceFrame(delta_time);
    SystraceAsyncEnd("UpdateGameState", kUpdateGameStateCode);

    SystraceAsyncBegin("UpdateRenderPrep", kUpdateRenderPrepCode);
    rt_data->state_machine->RenderPrep(rt_data->renderer);
    SystraceAsyncEnd("UpdateRenderPrep", kUpdateRenderPrepCode);

    rt_data->audio_engine->AdvanceFrame(delta_time / 1000.0f);

    if (rt_data->state_machine->done()) {
      *(rt_data->game_exiting) = true;
    }
    pthread_mutex_unlock(&Game::gameupdate_mutex_);
  }

#ifdef __ANDROID__
  jvm->DetachCurrentThread();
#endif  //__ANDROID__
  pthread_mutex_unlock(&Game::updatethread_mutex_);

  return nullptr;
}

void HandleVsync() { pthread_cond_broadcast(&Game::start_render_cv_); }

// Hack, to simulate vsync events on non-android devices.
static void* VsyncSimulatorThread(void* data) {
  (void)data;
  while (true) {
    HandleVsync();
    Delay(2);
  }
  return nullptr;
}

// For performance, we're using multiple threads so that the game state can
// be updating in the background while openGL renders.
// The general plan is:
// 1. Vsync happens.  Everything begins.
// 2. Renderthread activates.  (The update thread is currently blocked.)
// 3. Renderthread dumps everything into opengl, via RenderAllEntities.  (And
//    any other similar calls, such as calls to IMGUI)  Updatethread is
//    still blocked.  When this is complete, OpenGL now has its own copy of
//    everything, and we can safely change world state data.
// 4. Renderthread signals updatethread to wake up.
// 5a.Renderthread calls gl_flush, (via Renderer.advanceframe) and waits for
//    everything render.  Once complete, it goes to sleep and waits for the
//    next vsync event.
// 5b.Updatethread goes and updates the game state and gets us all ready for
//    next frame.  Once complete, it also goes to sleep and waits for the next
//    vsync event.
void Game::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  prev_render_time_ = CurrentWorldTime() - kMinUpdateTime;

  // Start the update thread:
  UpdateThreadData rt_data(&game_exiting_, &world_, &state_machine_, &renderer_,
                           &audio_engine_);

  input_.AdvanceFrame(&renderer_.window_size());
  state_machine_.AdvanceFrame(16);

  pthread_t update_thread;
  if (pthread_create(&update_thread, nullptr, UpdateThread, &rt_data)) {
    LogError("Error creating update thread.");
    assert(false);
  }

#ifdef __ANDROID__
  RegisterVsyncCallback(HandleVsync);
#else
  // We don't need this on android because we'll just get vsync events directly.
  pthread_t vsync_simulator_thread;
  if (pthread_create(&vsync_simulator_thread, nullptr, VsyncSimulatorThread,
                     nullptr)) {
    LogError("Error creating vsync simulator thread.");
    assert(false);
  }
#endif
  // We basically own the lock all the time, except when we're waiting
  // for a vsync event.
  pthread_mutex_lock(&renderthread_mutex_);
  while (!input_.exit_requested() && !game_exiting_) {
    // -------------------------------------------
    // Steps 1, 2.
    // Wait for start of frame.  (triggered at vsync start on android.)
    // -------------------------------------------
    pthread_cond_wait(&start_render_cv_, &renderthread_mutex_);

    // Grab the lock to make sure the game isn't still updating.
    pthread_mutex_lock(&gameupdate_mutex_);

    SystraceBegin("RenderFrame");

    // Input update must happen from the render thread.
    // From the SDL documentation on SDL_PollEvent(),
    // https://wiki.libsdl.org/SDL_PollEvent):
    // "As this function implicitly calls SDL_PumpEvents(), you can only call
    // this function in the thread that set the video mode."
    SystraceBegin("Input::AdvanceFrame()");
    input_.AdvanceFrame(&renderer_.window_size());
    SystraceEnd();

    // Milliseconds elapsed since last update.
    const WorldTime world_time = CurrentWorldTime();
    prev_render_time_ = world_time;

    // -------------------------------------------
    // Step 3.
    // Render everything.
    // -------------------------------------------
    SystraceBegin("StateMachine::Render()");

    RenderTarget::ScreenRenderTarget(renderer_).SetAsRenderTarget();
    renderer_.ClearDepthBuffer();
    renderer_.SetCulling(Renderer::kCullBack);

    state_machine_.Render(&renderer_);
    SystraceEnd();

    pthread_mutex_unlock(&gameupdate_mutex_);

    SystraceBegin("StateMachine::HandleUI()");
    state_machine_.HandleUI(&renderer_);
    SystraceEnd();

    // -------------------------------------------
    // Step 4.
    // Signal the update thread that it is safe to start messing with
    // data, now that we've already handed it all off to openGL.
    // -------------------------------------------
    pthread_cond_broadcast(&Game::start_update_cv_);

    // -------------------------------------------
    // Step 5a.
    // Start openGL actually rendering.  AdvanceFrame will (among other things)
    // trigger a gl_flush.  This thread will block until it is completed,
    // but that's ok because the update thread is humming in the background
    // preparing the worlds tate for next frame.
    // -------------------------------------------
    SystraceBegin("AdvanceFrame");
    renderer_.AdvanceFrame(input_.minimized(), input_.Time());
    SystraceEnd();  // AdvanceFrame

    SystraceEnd();  // RenderFrame

    gpg_manager_.Update();

    // Process input device messages since the last game loop.
    // Update render window size.
    if (input_.GetButton(fpl::FPLK_BACKQUOTE).went_down()) {
      ToggleRelativeMouseMode();
    }

    int new_time = CurrentWorldTime();
    int frame_time = new_time - world_time;

    SystraceCounter("FrameTime", frame_time);
  }
  pthread_mutex_unlock(&renderthread_mutex_);
}

}  // fpl_project
}  // fpl
