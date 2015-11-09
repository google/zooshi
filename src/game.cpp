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
#include "breadboard/modules/common.h"
#include "common.h"
#include "entity/entity.h"
#include "fplbase/input.h"
#include "fplbase/systrace.h"
#include "fplbase/utilities.h"
#include "graph_generated.h"
#include "input_config_generated.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/vector.h"
#include "module_library/animation.h"
#include "module_library/audio.h"
#include "module_library/default_graph_factory.h"
#include "module_library/entity.h"
#include "module_library/physics.h"
#include "module_library/transform.h"
#include "module_library/vec3.h"
#include "modules/attributes.h"
#include "modules/gpg.h"
#include "modules/patron.h"
#include "modules/player.h"
#include "modules/rail_denizen.h"
#include "modules/state.h"
#include "modules/zooshi.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/math/angle.h"
#include "motive/util/benchmark.h"
#include "pindrop/pindrop.h"
#include "world.h"

#ifdef __ANDROID__
#include "fplbase/renderer_android.h"
#endif

#ifdef ANDROID_HMD
#include "fplbase/renderer_hmd.h"
#endif

#define ZOOSHI_OVERDRAW_DEBUG 0
// ZOOSHI_FORCE_ONSCREEN_CONTROLLER is useful when testing the on-screen
// controller on desktop and mobile platforms.
#define ZOOSHI_FORCE_ONSCREEN_CONTROLLER 0

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

static const char kAssetsDir[] = "assets";

static const char kConfigFileName[] = "config.zooconfig";

#ifdef __ANDROID__
static const int kAndroidMaxScreenWidth = 1280;
static const int kAndroidMaxScreenHeight = 720;
#endif

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
static const char kVersion[] = "Zooshi";

GameSynchronization::GameSynchronization()
    : renderthread_mutex_(SDL_CreateMutex()),
      updatethread_mutex_(SDL_CreateMutex()),
      gameupdate_mutex_(SDL_CreateMutex()),
      start_render_cv_(SDL_CreateCond()),
      start_update_cv_(SDL_CreateCond()) {}

Game::Game()
    : asset_manager_(renderer_),
      graph_factory_(&module_registry_, &LoadFile),
      shader_lit_textured_normal_(nullptr),
      shader_textured_(nullptr),
      game_exiting_(false),
      audio_config_(nullptr),
      world_(),
      fader_(),
      version_(kVersion) {}

Game::~Game() {
  asset_manager_.ClearAllAssets();
  renderer_.ShutDown();
}

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

#ifdef ANDROID_HMD
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

  if (GetSystemRamSize() <= kLowRamProfileThreshold) {
    // Reduce material size.
    asset_manager_.SetTextureScale(kLowRamDeviceTextureScale);
  }

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

void BreadboardLogFunc(const char* fmt, va_list args) { LogError(fmt, args); }

void Game::InitializeBreadboardModules() {
  breadboard::RegisterLogFunc(BreadboardLogFunc);
  graph_factory_.set_audio_engine(&audio_engine_);

  // Common module initialization.
  breadboard::InitializeCommonModules(&module_registry_);

  // Module library initialization.
  module_library::InitializeAnimationModule(
      &module_registry_, &world_.graph_component, &world_.animation_component,
      &world_.transform_component);
  module_library::InitializeAudioModule(&module_registry_, &audio_engine_);
  module_library::InitializeEntityModule(
      &module_registry_, &world_.entity_manager, &world_.meta_component,
      &world_.graph_component);
  module_library::InitializePhysicsModule(
      &module_registry_, &world_.physics_component, &world_.graph_component);
  module_library::InitializeTransformModule(&module_registry_,
                                            &world_.transform_component);
  module_library::InitializeVec3Module(&module_registry_);

  // Zooshi module initialization.
  InitializeAttributesModule(&module_registry_, &world_.attributes_component,
                             &world_.graph_component);
  InitializeGpgModule(&module_registry_, &GetConfig(), &gpg_manager_);
  InitializePatronModule(&module_registry_, &world_.patron_component);
  InitializePlayerModule(&module_registry_, &world_.player_component,
                         &world_.graph_component);
  InitializeRailDenizenModule(&module_registry_, &world_.rail_denizen_component,
                              &world_.graph_component);
  InitializeStateModule(&module_registry_, gameplay_state_.requested_state());
  InitializeZooshiModule(&module_registry_, &world_.services_component,
                         &world_.graph_component, &world_.scenery_component);
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
#ifdef ANDROID_HMD
  input_.head_mounted_display_input().EnableDeviceOrientationCorrection();
#endif  // ANDROID_HMD

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

  if (!audio_engine_.Initialize(GetConfig().audio_config()->c_str())) {
    return false;
  }
  audio_engine_.LoadSoundBank(asset_manifest.sound_bank()->c_str());
  audio_engine_.StartLoadingSoundFiles();

  InitializeBreadboardModules();

  for (size_t i = 0; i < asset_manifest.font_list()->size(); i++) {
    font_manager_.Open(asset_manifest.font_list()->Get(i)->c_str());
  }
  font_manager_.SetRenderer(renderer_);

  SetPerformanceMode(kHighPerformance);

  scene_lab_.reset(new scene_lab::SceneLab());

  world_.Initialize(GetConfig(), &input_, &asset_manager_, &world_renderer_,
                    &font_manager_, &audio_engine_, &graph_factory_, &renderer_,
                    scene_lab_.get());

#ifdef __ANDROID__
  if (SupportsHeadMountedDisplay()) {
    BasePlayerController* controller = new AndroidCardboardController();
    controller->set_input_config(&GetInputConfig());
    controller->set_input_system(&input_);
    controller->set_enabled(ZOOSHI_FORCE_ONSCREEN_CONTROLLER == 0);
    world_.AddController(controller);
    world_.hmd_controller = controller;
  }
#endif  // __ANDROID__

// If this is a mobile platform or the onscreen controller is forced on
// instance the onscreen controller.
#if defined(PLATFORM_MOBILE) || ZOOSHI_FORCE_ONSCREEN_CONTROLLER
  {
    OnscreenController* onscreen_controller = new OnscreenController();
    onscreen_controller->set_input_config(&GetInputConfig());
    onscreen_controller->set_input_system(&input_);
    onscreen_controller->set_enabled(!SupportsHeadMountedDisplay() ||
                                     ZOOSHI_FORCE_ONSCREEN_CONTROLLER);
    world_.AddController(onscreen_controller);
    world_.onscreen_controller_ui.set_controller(onscreen_controller);
#ifdef ANDROID_HMD
    world_.onscreen_controller = onscreen_controller;
#endif  // ANDROID_HMD
  }
#endif  // defined(PLATFORM_MOBILE) || ZOOSHI_FORCE_ONSCREEN_CONTROLLER

// If this is a desktop platform and we're not forcing the onscreen controller
// instance the mouse controller.
#if !defined(PLATFORM_MOBILE) && !ZOOSHI_FORCE_ONSCREEN_CONTROLLER
  {
    BasePlayerController* controller = new MouseController();
    controller->set_input_config(&GetInputConfig());
    controller->set_input_system(&input_);
    world_.AddController(controller);
  }
#endif  // !ZOOSHI_FORCE_ONSCREEN_CONTROLLER && !defined(PLATFORM_MOBILE)

#ifdef ANDROID_GAMEPAD
  {
    GamepadController* controller = new GamepadController();
    controller->set_input_config(&GetInputConfig());
    controller->set_input_system(&input_);
    world_.AddController(controller);
  }
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
  loading_state_.Initialize(&input_, &world_, asset_manifest, &asset_manager_,
                            &audio_engine_, shader_textured_, &fader_);
  pause_state_.Initialize(&input_, &world_, config, &asset_manager_,
                          &font_manager_, &audio_engine_);
  gameplay_state_.Initialize(&input_, &world_, config, &GetInputConfig(),
                             &world_.entity_manager, scene_lab_.get(),
                             &gpg_manager_, &audio_engine_, &fader_);
  game_menu_state_.Initialize(&input_, &world_, config, &asset_manager_,
                              &font_manager_, &GetAssetManifest(),
                              &gpg_manager_, &audio_engine_, &fader_);
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

#ifdef ANDROID_HMD
  if (AndroidGetActivityName() == "com.google.fpl.zooshi.ZooshiHmdActivity") {
    world_.SetIsInCardboard(true);
  }
#endif  // ANDROID_HMD

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

static inline entity::WorldTime CurrentWorldTime(const InputSystem& input) {
  return static_cast<entity::WorldTime>(input.Time() * 1000);
}

static inline entity::WorldTime CurrentWorldTimeSubFrame(
    const InputSystem& input) {
  return static_cast<entity::WorldTime>(input.RealTime() * 1000);
}

// Stuff the update thread needs to know about:
struct UpdateThreadData {
  UpdateThreadData(bool* exiting, World* world_ptr,
                   StateMachine<kGameStateCount>* statemachine_ptr,
                   Renderer* renderer_ptr, InputSystem* input_ptr,
                   pindrop::AudioEngine* audio_engine_ptr,
                   GameSynchronization* sync_ptr)
      : game_exiting(exiting),
        world(world_ptr),
        state_machine(statemachine_ptr),
        renderer(renderer_ptr),
        input(input_ptr),
        audio_engine(audio_engine_ptr),
        sync(sync_ptr) {}
  bool* game_exiting;
  World* world;
  StateMachine<kGameStateCount>* state_machine;
  Renderer* renderer;
  InputSystem* input;
  pindrop::AudioEngine* audio_engine;
  GameSynchronization* sync;
  entity::WorldTime frame_start;
};

// This is the thread that handles all of our actual game logic updates:
static int UpdateThread(void* data) {
  UpdateThreadData* rt_data = static_cast<UpdateThreadData*>(data);
  GameSynchronization& sync = *rt_data->sync;
  int prev_update_time;
  prev_update_time = CurrentWorldTime(*rt_data->input) - kMinUpdateTime;
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
    const entity::WorldTime world_time = CurrentWorldTime(*rt_data->input);
    const entity::WorldTime delta_time =
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
    SDL_Delay(2);
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
                           &input_, &audio_engine_, &sync_);

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

  // variables used for regulating our framerate:
  // Total size of our history, in frames:
  const int kHistorySize = 60 * 5;
  // Max number of frames we can have dropped in our history, before we
  // switch to queue-stuffing mode, and ignore vsync pauses.
  const int kMaxDroppedFrames = 3;
  // Variable
  bool missed_frame_history[kHistorySize];
  for (int i = 0; i < kHistorySize; i++) {
    missed_frame_history[i] = false;
  }
  int history_index = 0;
  int total_dropped_frames = 0;

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
  int last_frame_id = 0;

  // We basically own the lock all the time, except when we're waiting
  // for a vsync event.
  SDL_LockMutex(sync_.renderthread_mutex_);
  while (!input_.exit_requested() && !game_exiting_) {
#ifdef __ANDROID__
    int current_frame_id = GetVsyncFrameId();
#else
    int current_frame_id = 0;
#endif
    // Update our framerate history:
    // The oldest value falls off and is replaced with the most recent frame.
    // Also, we update our counts.
    if (missed_frame_history[history_index]) {
      total_dropped_frames--;
    }
    // We count it as a dropped frame if more than one vsync event passed since
    // we started rendering it.  The check is implemented via equality
    // comparisons because current_frame_id will eventually wrap.)
    missed_frame_history[history_index] =
        (current_frame_id != last_frame_id + 1) &&
        (current_frame_id != last_frame_id);
    if (missed_frame_history[history_index]) {
      total_dropped_frames++;
    }
    history_index = (history_index + 1) % kHistorySize;
    last_frame_id = current_frame_id;

    // -------------------------------------------
    // Steps 1, 2.
    // Wait for start of frame.  (triggered at vsync start on android.)
    // For performance, we only wait if we're not dropping frames.  Otherwise,
    // we just keep rendering as fast as we can and stuff the render queue.
    if (total_dropped_frames <= kMaxDroppedFrames) {
      SDL_CondWait(sync_.start_render_cv_, sync_.renderthread_mutex_);
    }

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
    rt_data.frame_start = CurrentWorldTimeSubFrame(input_);

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

    int new_time = CurrentWorldTimeSubFrame(input_);
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
static const int kSampleDuration = 5;  // in seconds
static const int kTargetFPS = 60;      // Used for calculating dropped frames
static const int kTargetFramesPerSample = kSampleDuration * kTargetFPS;

// Collect framerate sample data, and print out nice histograms and statistics
// every five seconds.
void Game::UpdateProfiling(entity::WorldTime frame_time) {
  if (frame_time >= 0 && frame_time < kHistogramSize) {
    histogram[frame_time]++;
  }
  int current_time = CurrentWorldTime(input_);
  if (current_time >
      last_printout + entity::kMillisecondsPerSecond * kSampleDuration) {
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
    entity::WorldTime total = 0;
    int total_count = 0;
    entity::WorldTime median = -1;
    int median_value = -1;

    for (int i = lowest; i <= highest; i++) {
      std::string s = "%d:";
      for (int j = 0; j < histogram[i]; j += 4) {
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
    LogInfo("average: %f", (float)total / (float)total_count);
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
