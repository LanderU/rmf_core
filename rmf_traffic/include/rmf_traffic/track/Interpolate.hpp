/*
 * Copyright (C) 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef RMF_TRAFFIC__TRACK__INTERPOLATE_HPP
#define RMF_TRAFFIC__TRACK__INTERPOLATE_HPP

#include <rmf_traffic/Trajectory.hpp>

#include <rmf_traffic/track/VehicleTraits.hpp>

namespace rmf_traffic {
namespace track {

//==============================================================================
/// This exception is thrown by Interpolate functions when the VehicleTraits
/// that are provided cannot
class invalid_traits_error : public std::exception
{
public:

  const char* what() const noexcept override;

  class Implementation;
private:
  invalid_traits_error();
  rmf_utils::impl_ptr<Implementation> _pimpl;
};

//==============================================================================
class Interpolate
{
public:

  class Options
  {
  public:

    Options(
        bool always_stop = true,
        double translation_thresh = 1e-3,
        double rotation_thresh = 1.0 * M_PI/180.0,
        double corner_angle_thresh = 1.0 * M_PI/180.0);

    /// The robot must always come to a complete stop at every position. When
    /// this is true, all other properties in the options will have no effect.
    Options& set_always_stop(bool choice);
    bool always_stop() const;

    /// If a waypoint is closer than this distance to its prior or subsequent
    /// waypoint, then it is allowed to be skipped.
    Options& set_translation_threshold(double dist);
    double get_translation_threshold() const;

    /// If a waypoint .
    Options& set_rotation_threshold(double dist);
    double get_rotation_threshold() const;

    /// If two line segments make a corner that is greater than this angle,
    /// then the waypoint must not be ignored.
    Options& set_corner_angle_threshold(double angle);
    double get_corner_angle_threshold() const;

  private:
    class Implementation;
    rmf_utils::impl_ptr<Implementation> _pimpl;
  };

  static Trajectory positions(
      std::string map,
      Time start_time,
      const VehicleTraits& traits,
      const std::vector<Eigen::Vector3d>& input_positions,
      const Options& options = Options());

};

} // namespace track
} // namespace rmf_traffic

#endif // RMF_TRAFFIC__TRACK__INTERPOLATE_HPP