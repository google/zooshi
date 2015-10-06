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

#include "SDL.h"
#include "SDL_events.h"
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
#include "modules/animation.h"
#include "modules/attributes.h"
#include "modules/audio.h"
#include "modules/debug.h"
#include "modules/entity.h"
#include "modules/gpg.h"
#include "modules/logic.h"
#include "modules/math.h"
#include "modules/physics.h"
#include "modules/patron_module.h"
#include "modules/player_module.h"
#include "modules/rail_denizen.h"
#include "modules/state.h"
#include "modules/string.h"
#include "modules/transform.h"
#include "modules/vec3.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/math/angle.h"
#include "motive/util/benchmark.h"
#include "pindrop/pindrop.h"
#include "world.h"
#include "zooshi_graph_factory.h"

#ifdef __ANDROID__
#include "fplbase/renderer_android.h"
#endif

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

#define ZOOSHI_OVERDRAW_DEBUG 0

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

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

GameSynchronization::GameSynchronization()
    : renderthread_mutex_(SDL_CreateMutex()),
      updatethread_mutex_(SDL_CreateMutex()),
      gameupdate_mutex_(SDL_CreateMutex()),
      start_render_cv_(SDL_CreateCond()),
      start_update_cv_(SDL_CreateCond()) {}

Game::Game()
    : asset_manager_(renderer_),
      graph_factory_(&event_system_, &LoadFile, &audio_engine_),
      shader_lit_textured_normal_(nullptr),
      shader_textured_(nullptr),
      game_exiting_(false),
      audio_config_(nullptr),
      world_(),
      fader_(),
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

  renderer_.set_color(mathfu::kOnes4f);
  // Initialize the first frame as black.
  renderer_.ClearFrameBuffer(mathfu::kZeros4f);

#ifdef ANDROID_CARDBOARD
  vec2i size = AndroidGetScalerResolution();
  const vec2i viewport_size =
      size.x() && size.y() ? size : renderer_.window_size();
  InitializeUndistortFramebuffer(viewport_size.x(), viewport_size.y());
#endif

#if ZOOSHI_OVERDRAW_DEBUG
  renderer_.SetBlendMode(BlendMode::kBlendModeAdd);
  renderer_.force_blend_mode() = BlendMode::kBlendModeAdd;
  renderer_.override_pixel_shader() =
      "void main() { gl_FragColor = vec4(0.2, 0.2, 0.2, 1); }";
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
  // Load the loading-material first, since we display that while the others
  // load.
  const AssetManifest& asset_manifest = GetAssetManifest();
  asset_manager_.LoadMaterial(asset_manifest.loading_material()->c_str());
  asset_manager_.LoadMaterial(asset_manifest.fader_material()->c_str());
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
  return *fpl::zooshi::GetConfig(config_source_.c_str());
}

const InputConfig& Game::GetInputConfig() const {
  return *fpl::zooshi::GetInputConfig(input_config_source_.c_str());
}

const AssetManifest& Game::GetAssetManifest() const {
  return *fpl::zooshi::GetAssetManifest(asset_manifest_source_.c_str());
}

void EventSystemLogFunc(const char* fmt, va_list args) { LogError(fmt, args); }

void Game::InitializeEventSystem() {
  breadboard::RegisterLogFunc(EventSystemLogFunc);

  breadboard::TypeRegistry<void>::RegisterType("Pulse");
  breadboard::TypeRegistry<bool>::RegisterType("Bool");
  breadboard::TypeRegistry<int>::RegisterType("Int");
  breadboard::TypeRegistry<float>::RegisterType("Float");
  breadboard::TypeRegistry<std::string>::RegisterType("String");

  breadboard::TypeRegistry<AttributesDataRef>::RegisterType("AttributesData");
  breadboard::TypeRegistry<RailDenizenDataRef>::RegisterType("RailDenizenData");
  breadboard::TypeRegistry<TransformDataRef>::RegisterType("TransformData");
  breadboard::TypeRegistry<entity::EntityRef>::RegisterType("Entity");
  breadboard::TypeRegistry<mathfu::vec3>::RegisterType("Vec3");
  breadboard::TypeRegistry<pindrop::Channel>::RegisterType("Channel");
  breadboard::TypeRegistry<pindrop::SoundHandle>::RegisterType("SoundHandle");

  InitializeAnimationModule(&event_system_, &world_.graph_component);
  InitializeAttributesModule(&event_system_, &world_.attributes_component,
                             &world_.graph_component);
  InitializeAudioModule(&event_system_, &audio_engine_);
  InitializeDebugModule(&event_system_);
  InitializeEntityModule(&event_system_, &world_.services_component,
                         &world_.meta_component, &world_.graph_component);
  InitializeGpgModule(&event_system_, &GetConfig(), &gpg_manager_);
  InitializeLogicModule(&event_system_);
  InitializeMathModule(&event_system_);
  InitializePatronModule(&event_system_, &world_.patron_component);
  InitializePlayerModule(&event_system_, &world_.player_component,
                         &world_.graph_component);
  InitializePhysicsModule(&event_system_, &world_.physics_component,
                          &world_.graph_component);
  InitializeRailDenizenModule(&event_system_, &world_.rail_denizen_component,
                              &world_.graph_component);
  InitializeStateModule(&event_system_, gameplay_state_.requested_state());
  InitializeStringModule(&event_system_);
  InitializeTransformModule(&event_system_, &world_.transform_component);
  InitializeVec3Module(&event_system_);
}

// Pause the audio when the game loses focus.
class AudioEngineVolumeControl {
 public:
  AudioEngineVolumeControl(pindrop::AudioEngine* audio) : audio_(audio) {}
  void operator()(void* userdata) {
    SDL_Event* event = static_cast<SDL_Event*>(userdata);
    switch (event->type) {
      case SDL_APP_WILLENTERBACKGROUND:
        audio_->Pause(true);
        break;
      case SDL_APP_DIDENTERFOREGROUND:
        audio_->Pause(false);
        break;
      default:
        break;
    }
  }

 private:
  pindrop::AudioEngine* audio_;
};

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool Game::Initialize(const char* const binary_directory) {
  LogInfo("Zooshi Initializing...");
  InitBenchmarks(10);

  input_.Initialize();
  input_.AddAppEventCallback(AudioEngineVolumeControl(&audio_engine_));

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
  const auto& asset_manifest = GetAssetManifest();

  if (!InitializeAssets()) return false;

  // Some people are having trouble loading the audio engine, and it's not
  // strictly necessary for gameplay, so don't die if the audio engine fails to
  // initialize.
  if (!audio_engine_.Initialize(GetConfig().audio_config()->c_str())) {
    LogError("Failed to initialize audio engine.\n");
  }
  audio_engine_.LoadSoundBank(asset_manifest.sound_bank()->c_str());
  audio_engine_.StartLoadingSoundFiles();

  InitializeEventSystem();

  for (size_t i = 0; i < asset_manifest.font_list()->size(); i++) {
    font_manager_.Open(asset_manifest.font_list()->Get(i)->c_str());
  }
  font_manager_.SetRenderer(renderer_);

  SetRelativeMouseMode(true);

  SetPerformanceMode(kHighPerformance);

  scene_lab_.reset(new scene_lab::SceneLab());

  world_.Initialize(GetConfig(), &input_, &asset_manager_, &world_renderer_,
                    &font_manager_, &audio_engine_, &graph_factory_, &renderer_,
                    scene_lab_.get());
#ifdef __ANDROID__
  BasePlayerController* controller = new AndroidCardboardController();
#else
  BasePlayerController* controller = new MouseController();
#endif
  controller->set_input_config(&GetInputConfig());
  controller->set_input_system(&input_);
  world_.AddController(controller);
#ifdef ANDROID_GAMEPAD
  controller = new GamepadController();
  controller->set_input_config(&GetInputConfig());
  controller->set_input_system(&input_);
  world_.AddController(controller);
#endif

  world_renderer_.Initialize(&world_);

  scene_lab_->Initialize(GetConfig().scene_lab_config(), &world_.entity_manager,
                         &font_manager_);

  scene_lab_->AddComponentToUpdate(
      component_library::TransformComponent::GetComponentId());
  scene_lab_->AddComponentToUpdate(ShadowControllerComponent::GetComponentId());
  scene_lab_->AddComponentToUpdate(
      component_library::RenderMeshComponent::GetComponentId());

  gpg_manager_.Initialize(false);

  auto fader_material =
      asset_manager_.FindMaterial(asset_manifest.fader_material()->c_str());
  assert(fader_material);
  fader_.Init(fader_material, shader_textured_);

  const Config* config = &GetConfig();
  loading_state_.Initialize(asset_manifest, &asset_manager_, &audio_engine_,
                            shader_textured_, &fader_);
  pause_state_.Initialize(&input_, &world_, config, &asset_manager_,
                          &font_manager_, &audio_engine_);
  gameplay_state_.Initialize(&input_, &world_, config, &GetInputConfig(),
                             &world_.entity_manager, scene_lab_.get(),
                             &gpg_manager_, &audio_engine_, &fader_);
  game_menu_state_.Initialize(&input_, &world_, config, &asset_manager_,
                              &font_manager_, &GetAssetManifest(),
                              &gpg_manager_, &audio_engine_);
  game_over_state_.Initialize(&input_, &world_, config, &asset_manager_,
                              &font_manager_, &gpg_manager_, &audio_engine_);
  intro_state_.Initialize(&input_, &world_, config, &fader_, &audio_engine_);
  scene_lab_state_.Initialize(&renderer_, &input_, scene_lab_.get(), &world_);

  state_machine_.AssignState(kGameStateLoading, &loading_state_);
  state_machine_.AssignState(kGameStateGameplay, &gameplay_state_);
  state_machine_.AssignState(kGameStatePause, &pause_state_);
  state_machine_.AssignState(kGameStateGameMenu, &game_menu_state_);
  state_machine_.AssignState(kGameStateIntro, &intro_state_);
  state_machine_.AssignState(kGameStateGameOver, &game_over_state_);
  state_machine_.AssignState(kGameStateSceneLab, &scene_lab_state_);
  state_machine_.SetCurrentStateId(kGameStateLoading);

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
                   pindrop::AudioEngine* audio_engine_ptr,
                   GameSynchronization* sync_ptr)
      : game_exiting(exiting),
        world(world_ptr),
        state_machine(statemachine_ptr),
        renderer(renderer_ptr),
        audio_engine(audio_engine_ptr),
        sync(sync_ptr) {}
  bool* game_exiting;
  World* world;
  StateMachine<kGameStateCount>* state_machine;
  Renderer* renderer;
  pindrop::AudioEngine* audio_engine;
  GameSynchronization* sync;
  WorldTime frame_start;
};

// This is the thread that handles all of our actual game logic updates:
static int UpdateThread(void* data) {
  UpdateThreadData* rt_data = static_cast<UpdateThreadData*>(data);
  GameSynchronization& sync = *rt_data->sync;
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

  WorldTime last_android_refresh = CurrentWorldTime();
#endif  //__ANDROID__

  SDL_LockMutex(sync.updatethread_mutex_);
  while (!*(rt_data->game_exiting)) {
    SDL_CondWait(sync.start_update_cv_, sync.updatethread_mutex_);

    // -------------------------------------------
    // Step 5b.  (See comment at the start of Run()
    // Update everything.  This is only called once everything has been
    // safely loaded up into openGL and the renderthread is working its way
    // through actually putting everything on the screen.
    // -------------------------------------------
    SDL_LockMutex(sync.gameupdate_mutex_);
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
    SDL_UnlockMutex(sync.gameupdate_mutex_);
  }

#ifdef __ANDROID__
  jvm->DetachCurrentThread();
#endif  //__ANDROID__
  SDL_UnlockMutex(sync.updatethread_mutex_);

  return 0;
}

// TODO: Modify vsync callback API to take a context parameter.
static GameSynchronization* global_vsync_context = nullptr;
void HandleVsync() {
  SDL_CondBroadcast(global_vsync_context->start_render_cv_);
}

// Hack, to simulate vsync events on non-android devices.
static int VsyncSimulatorThread(void* /*data*/) {
  while (true) {
    HandleVsync();
    Delay(2);
  }
  return 0;
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
  // Start the update thread:
  UpdateThreadData rt_data(&game_exiting_, &world_, &state_machine_, &renderer_,
                           &audio_engine_, &sync_);

  input_.AdvanceFrame(&renderer_.window_size());
  state_machine_.AdvanceFrame(16);

  SDL_Thread* update_thread =
      SDL_CreateThread(UpdateThread, "Zooshi Update Thread", &rt_data);
  if (!update_thread) {
    LogError("Error creating update thread.");
    assert(false);
  }

#if DISPLAY_FRAMERATE_HISTOGRAM
  for (int i = 0; i < kHistogramSize; i++) {
    histogram[i] = 0;
  }
  last_printout = 0;
#endif

  global_vsync_context = &sync_;
#ifdef __ANDROID__
  RegisterVsyncCallback(HandleVsync);
#else
  // We don't need this on android because we'll just get vsync events directly.
  SDL_Thread* vsync_simulator_thread = SDL_CreateThread(
      VsyncSimulatorThread, "Zooshi Simulated Vsync Thread", nullptr);
  if (!vsync_simulator_thread) {
    LogError("Error creating vsync simulator thread.");
    assert(false);
  }
#endif
  // We basically own the lock all the time, except when we're waiting
  // for a vsync event.
  SDL_LockMutex(sync_.renderthread_mutex_);
  while (!input_.exit_requested() && !game_exiting_) {
    // -------------------------------------------
    // Steps 1, 2.
    // Wait for start of frame.  (triggered at vsync start on android.)
    // -------------------------------------------
    SDL_CondWait(sync_.start_render_cv_, sync_.renderthread_mutex_);

    // Grab the lock to make sure the game isn't still updating.
    SDL_LockMutex(sync_.gameupdate_mutex_);

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
    rt_data.frame_start = CurrentWorldTime();

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

    SDL_UnlockMutex(sync_.gameupdate_mutex_);

    SystraceBegin("StateMachine::HandleUI()");
    state_machine_.HandleUI(&renderer_);
    SystraceEnd();

    // -------------------------------------------
    // Step 4.
    // Signal the update thread that it is safe to start messing with
    // data, now that we've already handed it all off to openGL.
    // -------------------------------------------
    SDL_CondBroadcast(sync_.start_update_cv_);

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
    int frame_time = new_time - rt_data.frame_start;
#if DISPLAY_FRAMERATE_HISTOGRAM
    UpdateProfiling(frame_time);
#endif

    SystraceCounter("FrameTime", frame_time);
  }
  SDL_UnlockMutex(sync_.renderthread_mutex_);
// Clean up asynchronous callbacks to prevent crashing on garbage data.
#ifdef __ANDROID__
  RegisterVsyncCallback(nullptr);
#endif
  input_.AddAppEventCallback(nullptr);
  // TODO: Move this call to SDL_Quit to FPLBase.
  SDL_Quit();
}


#if DISPLAY_FRAMERATE_HISTOGRAM
static const int kSampleDuration = 5; // in seconds
static const int kTargetFPS = 60; // Used for calculating dropped frames
static const int kTargetFramesPerSample = kSampleDuration * kTargetFPS;

// Collect framerate sample data, and print out nice histograms and statistics
// every five seconds.
void Game::UpdateProfiling(WorldTime frame_time) {
  if (frame_time >= 0 && frame_time < kHistogramSize) {
    histogram[frame_time]++;
  }
  int current_time = CurrentWorldTime();
  if (current_time > last_printout + kMillisecondsPerSecond * kSampleDuration) {
    int highest = -1;
    int lowest = kHistogramSize;
    for (int i = 0; i < kHistogramSize; i++) {
      if (histogram[i] != 0) {
        if (i > highest) highest = i;
        if (i < lowest) lowest = i;
      }
    }

    last_printout = current_time;
    LogInfo("Framerate Breakdown:");
    LogInfo("---------------------------------");
    WorldTime total = 0;
    int total_count = 0;
    WorldTime median = -1;
    int median_value = -1;

    for (int i = lowest; i <= highest; i++) {
      std::string s = "%d:";
      for (int j = 0; j < histogram[i]; j+=4) {
        s += "*";
      }
      s += " (%d)";
      LogInfo(s.c_str(), i, histogram[i]);
      total += histogram[i] * i;
      total_count += histogram[i];
      if (median == -1 || histogram[i] > median_value) {
        median = i;
        median_value = histogram[i];
      }
      histogram[i] = 0;
    }
    LogInfo("---------------------------------");
    LogInfo("total frames: %d", total_count);
    LogInfo("average: %f", (float)total/(float)total_count);
    LogInfo("median: %d", median);
    LogInfo("dropped frames: %d / %d (%d%%)",
            (kTargetFramesPerSample - total_count), kTargetFramesPerSample,
            (100 * (kTargetFramesPerSample - total_count)) /
                kTargetFramesPerSample);
    LogInfo("---------------------------------");
  }
}
#endif

}  // zooshi
}  // fpl
