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

#include "modules/audio.h"

#include "event/event_system.h"
#include "event/node_interface.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"

using mathfu::vec3;

namespace fpl {
namespace fpl_project {

// Plays the given sound. Also takes a gain and location arguments.
// Returns the channel the sound is playing on as an output.
class PlaySoundNode : public event::NodeInterface {
 public:
  PlaySoundNode(pindrop::AudioEngine* audio_engine)
      : audio_engine_(audio_engine) {}

  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::SoundHandle>();
    node_sig->AddInput<vec3>();
    node_sig->AddInput<float>();
    node_sig->AddOutput<pindrop::Channel>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto handle = in->Get<pindrop::SoundHandle>(0);
    auto location = in->Get<vec3>(1);
    auto gain = in->Get<float>(2);
    pindrop::Channel channel =
        audio_engine_->PlaySound(*handle, *location, *gain);
    out->Set(0, channel);
  }

 private:
  pindrop::AudioEngine* audio_engine_;
};

// Checks if a given audio channel is playing.
class PlayingNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddOutput<pindrop::Channel>();
    node_sig->AddOutput<bool>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    out->Set(0, *channel);
    out->Set(1, channel->Playing());
  }
};

// Stops the given audio channel.
class StopNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddOutput<pindrop::Channel>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    channel->Stop();
    out->Set(0, *channel);
  }
};

// Set the gain on the given audio channel.
class SetGainNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddInput<float>();
    node_sig->AddOutput<pindrop::Channel>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    auto gain = in->Get<float>(0);
    channel->SetGain(*gain);
    out->Set(0, *channel);
  }
};

// Returns the gain of the given audio channel.
class GainNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddOutput<pindrop::Channel>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    out->Set(0, *channel);
    out->Set(1, channel->Gain());
  }
};

// Sets the location of the given sound channel.
class SetLocationNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddInput<vec3>();
    node_sig->AddOutput<pindrop::Channel>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    auto location = in->Get<vec3>(1);
    channel->SetLocation(*location);
    out->Set(0, *channel);
  }
};

// Returns the location of the given audio channel.
class LocationNode : public event::NodeInterface {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<pindrop::Channel>();
    node_sig->AddOutput<pindrop::Channel>();
    node_sig->AddOutput<vec3>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto channel = in->Get<pindrop::Channel>(0);
    out->Set(0, *channel);
    out->Set(1, channel->Location());
  }
};

void InitializeAudioModule(event::EventSystem* event_system,
                           pindrop::AudioEngine* audio_engine) {
  event::Module* module = event_system->AddModule("audio");
  auto PlaySoundCtor = [audio_engine]() {
    return new PlaySoundNode(audio_engine);
  };
  module->RegisterNode<PlaySoundNode>("play_sound", PlaySoundCtor);
  module->RegisterNode<PlayingNode>("playing");
  module->RegisterNode<StopNode>("stop");
  module->RegisterNode<SetGainNode>("set_gain");
  module->RegisterNode<GainNode>("gain");
  module->RegisterNode<SetLocationNode>("set_location");
  module->RegisterNode<LocationNode>("location");
}

}  // fpl_project
}  // fpl
