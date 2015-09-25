// Copyright 2014 Google Inc. All rights reserved.
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

#ifndef ZOOSHI_FULL_SCREEN_FADER_H
#define ZOOSHI_FULL_SCREEN_FADER_H

#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {

class Renderer;
class Material;
class Shader;

namespace fpl_project {

// Renders a fullscreen overlay fading effect that transitions to
// opaque then back to transparent.
class FullScreenFader {
 public:
  FullScreenFader();
  ~FullScreenFader() {}

  // Initialize the fader's internal state. Call before Start().
  void Init(Material* material, Shader* shader);

  // Start the fullscreen fading effect with a duration of the given
  // fade_time and the given overlay color.
  void Start(WorldTime fade_time, const mathfu::vec4& color);

  // Update the fade color returning true on the frame the overlay
  // is fully opaque.
  bool AdvanceFrame(int delta_time);
  // Renders the fullscreen fading overlay.
  void Render(Renderer* renderer);
  // Returns true when the fullscreen fading effect is complete.
  bool Finished() const;

 private:
  // Current fade time, variable state, increments with each call to
  // AdvanceFrame().
  WorldTime current_fade_time_;
  // Total fade time, constant, set with Start().
  WorldTime total_fade_time_;
  // Color of the overlay (the alpha component is ignored), constant, set with
  // Start().
  mathfu::vec4 color_;
  // Material used to render the overlay, set with Init().
  Material* material_;
  // Shader used to render the overlay material, set with Init().
  Shader* shader_;
  // Opaque flag, variable state, true once the effect transition back
  // from opaque.
  bool opaque_;
};

}  // namespace fpl_project
}  // namespace fpl

#endif  // ZOOSHI_FULL_SCREEN_FADER_H
