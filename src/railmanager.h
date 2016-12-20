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

#ifndef RAILMANAGER_H
#define RAILMANAGER_H

#include <memory>
#include <unordered_map>
#include "components_generated.h"
#include "corgi/entity_manager.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/compact_spline.h"
#include "rail_def_generated.h"

namespace fpl {
namespace zooshi {

typedef const char* RailId;

class Rail {
 public:
  Rail() : splines_(nullptr), wraps_(true) {}
  ~Rail() { motive::CompactSpline::DestroyArray(splines_, kDimensions); }

  void Initialize(const RailDef* rail_def, float spline_granularity);

  /// Return vector of `positions` that is the rail evaluated every `delta_time`
  /// for the entire course of the rail. This calculation is much faster than
  /// calling PositionCalculatedSlowly() multiple times.
  void Positions(float delta_time,
                 std::vector<mathfu::vec3_packed>* positions) const;

  /// Return the rail position at `time`. This calculation is fairly slow
  /// so only use outside a loop. If you need a series of positions, consider
  /// calling Positions() above instead.
  mathfu::vec3 PositionCalculatedSlowly(float time) const;

  /// Length of the rail.
  float EndTime() const { return splines_->EndX(); }

  /// Internal structure representing the rails.
  const motive::CompactSpline* Splines() const { return splines_; }

  void InitializeFromPositions(
      const std::vector<mathfu::vec3_packed>& positions,
      float spline_granularity, float reliable_distance, float total_time,
      bool wraps);

  /// Does the rail wrap around to itself at the end.
  bool wraps() const { return wraps_; }

 private:
  static const motive::MotiveDimension kDimensions = 3;

  motive::CompactSpline* Spline(int idx) { return splines_->NextAtIdx(idx); }
  const motive::CompactSpline* Spline(int idx) const {
    return const_cast<Rail*>(this)->Spline(idx);
  }

  // Points to the first of kDimension splines in contiguous memory.
  motive::CompactSpline* splines_;

  // Does the rail wrap around to itself at the end.
  bool wraps_;
};

// Class for handling loading and storing of rails.
class RailManager {
 public:
  // Returns the data for the rail specified in the supplied filename.
  // If that data has already been loaded, it just returns the data directly.
  // Otherwise, it loads the data and caches it.
  Rail* GetRail(RailId rail_file);

  // Returns the data for a rail specified by RailNodeComponent entities.
  Rail* GetRailFromComponents(const char* rail_name,
                              corgi::EntityManager* entity_manager);

  void Clear();

 private:
  std::unordered_map<RailId, std::unique_ptr<Rail>> rail_map;
};

}  // zooshi
}  // fpl

#endif  // RAILMANAGER_H
