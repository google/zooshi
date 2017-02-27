// Copyright 2016 Google Inc. All rights reserved.
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

#ifndef FPL_ZOOSHI_COMPONENTS_RENDER_3D_TEXT_H_
#define FPL_ZOOSHI_COMPONENTS_RENDER_3D_TEXT_H_

#include "components/services.h"
#include "corgi/component.h"
#include "corgi_component_library/camera_interface.h"

namespace fpl {
namespace zooshi {

/// @brief A struct to hold all of the data for an
/// entity that is registered with the Render3dTextComponent.
struct Render3dTextData {
  Render3dTextData()
      : animation_bone(0),
        canvas_size(0),
        font(),
        label_size(0.0f),
        translation(mathfu::kZeros3f),
        rotation(mathfu::kZeros3f),
        scale(mathfu::kZeros3f),
        text() {}

  /// @brief For animated entities, this is the index of the bone to render the
  /// text onto.
  int animation_bone;

  /// @brief The base size of the desired FlatUI canvas to render the text on,
  /// in pixels.
  /// @note The size will be adjusted to reflect the current aspect ratio.
  int canvas_size;

  /// @brief The relative path to the font file used for the text that will be
  /// rendered on the entity.
  std::string font;

  /// @brief The vertical size (y-size) of the text, in virtual resolution.
  /// @note The horizontal size (x-size) will be derived automatically based on
  /// the text length.
  float label_size;

  /// @brief The translation of the text from the camera origin.
  mathfu::vec3_packed translation;

  /// @brief The rotation to orient the text, in degrees.
  mathfu::vec3_packed rotation;

  /// @brief The scale transform, to adjust the size of the text.
  mathfu::vec3_packed scale;

  /// @brief The text string to be rendered in 3D on the entity.
  std::string text;
};

/// @brief A Component that handles the rendering of text on an entity
/// in 3D space.
class Render3dTextComponent : public corgi::Component<Render3dTextData> {
 public:
  /// @brief Destructor for Render3dTextComponent.
  virtual ~Render3dTextComponent() {}

  /// @brief Deserialize a flat binary buffer to create and populate an entity
  /// from raw data.
  ///
  /// @param[in,out] entity An EntityRef reference that points to the entity
  /// that is being added from the raw data.
  /// @param[in] raw_data A void pointer to the raw FlatBuffer data.
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* raw_data);

  /// @brief Calculates the animation transform to use with the world transform
  /// for an entity.
  ///
  /// @note Returns the Identity transform (`mathfu::Identity()`) if there is no
  /// animation transform for the entity's bone at index `animation_bone`.
  ///
  /// @param[in] entity A `corgi::EntityRef` reference to the entity whose
  /// rendermesh and animation are used to calculate the animation transform.
  /// @param[in] animation_bone An `int` index of the bone to calculate the
  /// animation transform for.
  /// @return Returns a `mathfu::mat4` containing the animation transform.
  const mathfu::mat4 CalculateAnimationTransform(
      const corgi::EntityRef& entity, const int animation_bone) const;

  /// @brief Calculate the ModelViewProjection (MVP) for the renderer to
  /// correctly project the text into 3D space.
  ///
  /// @param[in] entity A `corgi::EntityRef` reference to the entity that
  /// should have the text rendered on it.
  /// @param[in] camera A `corgi::CameraInterface` reference to the camera
  /// that should be used when rendering the text within the field of view.
  /// @return Returns a `mathfu::mat4` containing the MVP matrix.
  const mathfu::mat4 CalculateModelViewProjection(
      const corgi::EntityRef& entity,
      const corgi::CameraInterface& camera) const;

  /// @brief Called whenever this Component is added to the EntityManager.
  virtual void Init();

  /// @brief Called whenever an entity is added to this Component to ensure
  /// it is also registered with all dependency Components.
  /// @param[in] entity The entity that is being added to this Component.
  virtual void InitEntity(corgi::EntityRef& entity);

  /// @cond FPL_ZOOSHI_COMPONENTS_INTERNAL
  // Currently only exists in prototypes, so no ExportRawData method is needed.
  virtual RawDataUniquePtr ExportRawData(
      const corgi::EntityRef& /*entity*/) const {
    return nullptr;
  }
  /// @endcond

  /// @brief Renders the text on a given entity.
  ///
  /// @note If the text would not be visible by the camera, then it is not
  /// rendered.
  ///
  /// @param[in] entity A `corgi::EntityRef` reference to the entity that
  /// should have the text rendered on it.
  /// @param[in] camera A `corgi::CameraInterface` reference to the camera
  /// that should be used when rendering the text within the field of view.
  void Render(const corgi::EntityRef& entity,
              const corgi::CameraInterface& camera);

  /// @brief Goes through and renders text on every entity that
  /// is registered with the Render3dTextComponent.
  ///
  /// It is equivalent to iterating through all of the entities individually
  /// and calling `Render()` on them.
  ///
  /// @note If the text would not be visible by the camera, then it is not
  /// rendered.
  ///
  /// @param[in] camera A `corgi::CameraInterface` reference to the camera
  /// that should be used when rendering the text within the field of view.
  void RenderAllEntities(const corgi::CameraInterface& camera);

  /// @brief Set the ModelViewProjection (MVP) for the renderer to correctly
  /// project the text into 3D space.
  ///
  /// @param[in] entity A `corgi::EntityRef` reference to the entity that
  /// should have the text rendered on it.
  /// @param[in] camera A `corgi::CameraInterface` reference to the camera
  /// that should be used when rendering the text within the field of view.
  void SetModelViewProjectionMatrix(const corgi::EntityRef& entity,
                                    const corgi::CameraInterface& camera);

  /// @cond FPL_ZOOSHI_COMPONENTS_INTERNAL
  // There is no per-frame update to the entities that needs to be done.
  virtual void UpdateAllEntities(corgi::WorldTime /*delta_time*/){};
  /// @endcond

  /// @brief Get the current text, and update a C-string with this value.
  /// @param[out] text A C-string to capture the string output.
  /// @param[in] text_length The length of the `text` buffer to capture the
  /// C-string.
  void SetText(const char* text, const int text_length);

 private:
  ServicesComponent* services_;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::Render3dTextComponent,
                         fpl::zooshi::Render3dTextData)

#endif  // FPL_ZOOSHI_COMPONENTS_RENDER_3D_TEXT_H_
