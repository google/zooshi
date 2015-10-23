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

#include "inputcontrollers/onscreen_controller.h"

#include "flatui/flatui.h"
#include "flatui/flatui_common.h"
#include "fplbase/asset_manager.h"
#include "fplbase/material.h"
#include "fplbase/utilities.h"
#include "inputcontrollers/gamepad_controller.h"
#include "mathfu/glsl_mappings.h"

using mathfu::vec2;
using mathfu::vec2i;

namespace fpl {
namespace zooshi {

void OnscreenController::UpdateButtons() {
  // Save the position of the last touch of the last pointer (last finger down).
  bool fire = false;
  const std::vector<InputPointer>& pointers = input_system_->get_pointers();
  for (size_t i = 0; i < pointers.size(); ++i) {
    const InputPointer& pointer = pointers[i];
    if (pointer.used &&
        input_system_->GetPointerButton(pointer.id).went_down()) {
      last_position_ = pointer.mousepos;
      fire = true;
      break;
    }
  }
  buttons_[kFireProjectile].SetValue(fire);
  delta_ = mathfu::kZeros2f;
}

void OnscreenControllerUI::Update(AssetManager* asset_manager,
                                  FontManager* font_manager,
                                  const vec2i& window_size) {
  if (controller_) {
    // Size of the controller.
    static const vec2 kSize(200.0f);
    static const vec2 kHalfSize(100.0f);
    // Size of the pointer within the controller.
    static const vec2 kPointerPositionSize(50.0f);
    static const vec2 kHalfPointerPositionSize(25.0f);
    // Dead zone of the controller in the range
    // 0.01 (pretty much no dead zone - can't be 0) ...
    // 1.0 (all dead zone / disabled)
    static const vec2 kDeadZoneTolerance(0.15f);
    // Sensitivity of the controller when it's at the clamped location.
    static const vec2 kSensitivity(0.7f);
    // Color of the controller background and foreground.
    static const vec4 kBackgroundColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);
    static const vec4 kForegroundColor = vec4(1.0f, 1.0f, 1.0f, 0.7f);
    // Whether to clamp the controller to a circular area
    // (default behavior clamps to a square).
    static const bool kClampToCircle = true;
    // Name of the group used to track interaction with the controller.
    static const char* kOnscreenController = "onscreen_controller";
    assert(base_texture_);
    assert(top_texture_);

    bool* visible = &visible_;
    const bool was_visible = visible_;
    Texture* base_texture = base_texture_;
    Texture* top_texture = top_texture_;
    InputSystem* input_system = controller_->input_system_;
    vec2* delta = &controller_->delta_;
    vec2* location = &location_;
    gui::Run(
        *asset_manager, *font_manager, *input_system,
        [&window_size, base_texture, top_texture, input_system, was_visible,
         visible, location, delta]() {
          vec2 pointer_position;
          gui::StartGroup(gui::kLayoutOverlay);
          {
            gui::StartGroup(gui::kLayoutHorizontalTop, 0, kOnscreenController);
            {
              vec2 virtual_window_size = gui::PhysicalToVirtual(window_size);
              gui::PositionGroup(gui::kAlignLeft, gui::kAlignTop, kHalfSize);
              auto event = gui::CheckEvent(false);
              if (event & (gui::kEventStartDrag | gui::kEventIsDragging)) {
                pointer_position = gui::PhysicalToVirtual(
                    input_system->get_pointers()[gui::GetCapturedPointerIndex()]
                        .mousepos);
              }
              if (event & gui::kEventStartDrag) {
                gui::CapturePointer(kOnscreenController);
                *visible = true;
                *location = pointer_position - kHalfSize;
              }

              if (event & gui::kEventIsDragging) {
                const vec2 extent = *location + kSize;
                vec2 direction;
                if (kClampToCircle) {
                  // Calculate the direction vector relative to the center of
                  // the controller to the extents of the controller.
                  direction =
                      (pointer_position - (*location + kHalfSize)) / kHalfSize;
                  // Calculate the location on the unit circle to clamp to.
                  float direction_magnitude = direction.Length();
                  if (direction_magnitude > 1.0f) {
                    direction /= direction_magnitude;
                  }
                  // Calculate the pointer position from the direction vector.
                  pointer_position =
                      (direction * kHalfSize) + *location + kHalfSize;
                } else {
                  // Clamp the location of the pointer within the bounds of the
                  // control.
                  pointer_position =
                      vec2(mathfu::Clamp(pointer_position.x(), location->x(),
                                         extent.x()),
                           mathfu::Clamp(pointer_position.y(), location->y(),
                                         extent.y()));

                  // Calculate position of the pointer relative to the middle of
                  // the control (the direction vector scaled -1.0 .. 1.0 in
                  // both directions).
                  direction =
                      (pointer_position - (*location + kHalfSize)) / kHalfSize;
                }

                // Start moving from the dead-zone.
                // Each direction is negated as screen coordinates go from zero
                // to positive top->bottom, left->right where directional
                // controls are inverted, i.e
                // x positive = left, x negative = right,
                // y positive = up, y negative = down.
                delta->x() = CalculateDelta(
                    direction.x(), kDeadZoneTolerance.x(), kSensitivity.x());
                delta->y() = CalculateDelta(
                    direction.y(), kDeadZoneTolerance.y(), kSensitivity.y());
              } else if (event & gui::kEventEndDrag) {
                *visible = false;
                *location = mathfu::kZeros2f;
                gui::ReleasePointer();
              }
              gui::CustomElement(
                  virtual_window_size - kSize, "mouse_capture",
                  [base_texture](const vec2i& pos, const vec2i& size) {
#if ZOOSHI_RENDERTOUCH_AREA
                    gui::RenderTexture(*base_texture, pos, size, vec4(0.1f));
#else
                    (void)pos;
                    (void)size;
#endif  // ZOOSHI_RENDERTOUCH_AREA
                  });
            }
            gui::EndGroup();
            if (was_visible && *visible) {
              gui::StartGroup(gui::kLayoutVerticalLeft);
              {
                gui::PositionGroup(gui::kAlignLeft, gui::kAlignTop, *location);
                gui::CustomElement(
                    kSize, "controller",
                    [&pointer_position, location, base_texture, top_texture](
                        const vec2i& pos, const vec2i& size) {
                      // Render the background.
                      gui::RenderTexture(*base_texture, pos, size,
                                         kBackgroundColor);
                      // Render the pointer location.
                      const vec2i pointer_render_location =
                          gui::VirtualToPhysical(pointer_position -
                                                 kHalfPointerPositionSize);
                      gui::RenderTexture(
                          *top_texture, pointer_render_location,
                          gui::VirtualToPhysical(kPointerPositionSize),
                          kForegroundColor);
                    });
              }
              gui::EndGroup();
            }
          }
          gui::EndGroup();
        });
  }
}

// Calculate the range of movement given the current magnitude,
// dead-zone along an axis and sensitivity applied to movement.
float OnscreenControllerUI::CalculateDelta(const float magnitude,
                                           const float dead_zone,
                                           const float sensitivity) {
  return fabs(magnitude) < dead_zone
             ? 0.0f
             : ((magnitude - dead_zone) / (1.0f - dead_zone)) * sensitivity *
                   -1.0f;
}

}  // zooshi
}  // fpl
