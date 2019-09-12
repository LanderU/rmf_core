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

#ifndef RMF_TRAFFIC__TRAJECTORY_HPP
#define RMF_TRAFFIC__TRAJECTORY_HPP

#include <rmf_traffic/geometry/ConvexShape.hpp>

#include <rmf_utils/impl_ptr.hpp>

#include <Eigen/Geometry>

#include <chrono>
#include <memory>
#include <vector>

namespace rmf_traffic {

class Trajectory
{
public:

  /// Data structure for storing time.
  using Time = std::chrono::steady_clock::time_point;
  using Duration = std::chrono::steady_clock::duration;

  class Profile;
  using ProfilePtr = std::shared_ptr<Profile>;
  using ConstProfilePtr = std::shared_ptr<const Profile>;

  /// The Trajectory::Profile class describes how the trajectory will take up
  /// space at a single instant in time. A profile description is assigned to
  /// each segment of a trajectory, so that the profile of a trajectory may
  /// change over time, for example as the robot changes its mode of movement.
  ///
  /// There are two factors to describe the profile:
  ///  * shape - Describes the occupied space
  ///  * movement - Describes how the robot is moving
  ///
  /// For Queued movement, the profile has an additional parameter:
  ///  * queue_id
  ///
  /// The queue_id indicates which queue the robot will be waiting in.
  class Profile
  {
  public:

    /// The Trajectory::Profile::Movement enum describes how the robot intends
    /// to move while following its trajectory.
    enum Movement {

      /// This movement type is illegal and will always be rejected by the
      /// schedule verifier. Having this movement type implies a major bug in
      /// the code and should be reported immediately.
      Unspecified = 0,

      /// The robot will follow the specified trajectory exactly.
      Strict,

      /// The robot will autonomously navigate within the specified space.
      Autonomous,

      /// The robot is waiting in a queue, and will wait to traverse the
      /// trajectory segment until the rmf_traffic_monitor tells it to proceed.
      Queued,
    };

    // Collision table:
    // |=============================================|
    // | Movement   | Strict  | Autonomous | Queued  |
    // |------------+---------+------------+---------|
    // | Strict     | COLLIDE |   okay     | COLLIDE |
    // |------------+---------+------------+---------|
    // | Autonomous |  okay   |  COLLIDE   |  okay   |
    // |------------+---------+------------+---------|
    // | Queued     | COLLIDE |   okay     | COLLIDE |
    // |=============================================|


    /// Create a profile with Strict movement
    static ProfilePtr make_strict(geometry::ConstConvexShapePtr shape);

    /// Create a profile with Autonomous movement
    static ProfilePtr make_autonomous(geometry::ConstConvexShapePtr shape);

    /// Create a profile for a Queued segment
    static ProfilePtr make_queued(
        geometry::ConstConvexShapePtr shape,
        const std::string& queue_id);

    /// Get the shape being used for this profile
    geometry::ConstConvexShapePtr get_shape() const;

    /// Set the shape that will be used by this profile
    void set_shape(geometry::ConstConvexShapePtr new_shape);

    /// Get the movement being used for this profile
    Movement get_movement() const;

    /// Set the movement of this profile to Strict
    void set_to_strict();

    /// Set the movement of this profile to Autonomous
    void set_to_autonomous();

    /// Set the movement of this profile to queued
    void set_to_queued(const std::string& queue_id);

    class QueueInfo
    {
    public:

      /// Get the id of the queue that this profile is waiting in
      std::string get_queue_id() const;

    private:
      QueueInfo(void* pimpl);
      friend class Profile;
      const void* const _pimpl;
    };

    /// If this Profile is queued, this will return a pointer to its queue
    /// information. If it is not in a queue, this will return a nullptr.
    ///
    /// This pointer is invalidated any time a modification is made to the
    /// Profile object that provided it.
    //
    // TODO(MXG): Change this to a std::optional when we can have C++17 support
    const QueueInfo* get_queue_info() const;

  private:

    Profile(geometry::ConstConvexShapePtr shape);

    class Implementation;
    rmf_utils::impl_ptr<Implementation> _pimpl;
  };

  class Segment
  {
  public:

    /// Get the profile of this Trajectory Segment
    ConstProfilePtr get_profile() const;

    /// Change the profile of this Trajectory Segment
    ///
    /// \param[in] new_profile
    ///   The new profile for this Trajectory Segment.
    void set_profile(ConstProfilePtr new_profile);

    /// Get the intended physical location of the robot at the end of this
    /// Trajectory Segment.
    ///
    /// This is a 2D homogeneous position. The first two values in the vector
    /// are x and y coordinates, while the third is rotation about the z-axis.
    Eigen::Vector3d get_position() const;

    /// Set the intended physical location of the robot at the end of this
    /// Trajectory Segment.
    ///
    /// This is a 2D homogeneous position. The first two values in the vector
    /// are x and y coordinates, while the third is rotation about the z-axis.
    ///
    /// \param[in] new_position
    ///   The new finishing position for this Trajectory Segment.
    void set_position(Eigen::Vector3d new_position);

    /// Get the intended velocity of the robot at the end of this Trajectory
    /// Segment.
    ///
    /// This is a 2D homogeneous position. The first two values in the vector
    /// are x and y velocities, while the third is rotational velocity about the
    /// z-axis.
    Eigen::Vector3d get_velocity() const;

    /// Set the intended velocity of the robot at the end of this Trajectory
    /// Segment.
    ///
    /// This is a 2D homogeneous position. The first two values in the vector
    /// are x and y coordinates, while the third is rotation about the z-axis.
    ///
    /// \param[in] new_velocity
    ///   The new finishing velocity for this Trajectory Segment.
    void set_velocity(Eigen::Vector3d new_velocity);

    /// Get the time that this Trajectory Segment is meant to finish.
    Time get_finish_time() const;

    /// Change the finish time of this Trajectory Segment. Note that this
    /// function will only affect this segment, and may cause this Segment to be
    /// reordered within the Trajectory.
    ///
    /// To change the finish time for this segment while preserving the relative
    /// times of all subsequent Trajectory segments, use adjust_finish_times()
    /// instead.
    ///
    /// \warning This function will invalidate
    ///
    /// \warning If you change the finishing time value of this Segment such
    /// that it falls directly on another Segment's finish time, you will get a
    /// std::invalid_argument exception, because discontinuous jumps are not
    /// supported, and indicate a significant mishandling of trajectory data,
    /// which is most likely a serious bug that should be remedied.
    ///
    /// \note If this Segment's finish time crosses past another Segment's
    /// finish time, that signficantly changes the topology of the Trajectory,
    /// because it will change the order in which the positions are passed
    /// through.
    ///
    /// \param[in] new_time
    ///   The new finishing time for this Trajectory Segment.
    ///
    /// \sa adjust_finish_times(Time new_time)
    void set_finish_time(Time new_time);

    /// Push back the finishing time of this segment and all subsequent
    /// segments by the given duration. This is guaranteed to maintain the
    /// ordering of the Trajectory Segments, and is more efficient than changing
    /// all the times directly.
    ///
    /// \param[in] delta_t
    ///   How much to change the finishing time of this segment and all later
    ///   segments. If negative, it must not cross over the finish time of the
    ///   previous segment, or else a std::invalid_argument will be thrown.
    ///
    /// \sa set_finish_time(Time new_time)
    void adjust_finish_times(Duration delta_t);

  private:

    /// \internal Private constructor. Use Trajectory::add_segment() to create
    /// a new Trajectory Segment.
    Segment(ConstProfilePtr profile);
    friend class Trajectory;
    class Implementation;
    rmf_utils::impl_ptr<Implementation> _pimpl;
  };

  template<typename SegT>
  class base_iterator;
  using iterator = base_iterator<Segment>;
  using const_iterator = base_iterator<const Segment>;

  /// Add a Segment to this Trajectory.
  ///
  /// The Segment will be inserted into the Trajectory according to its
  /// finish_time, ensuring correct ordering of all Segments.
  iterator insert(
      Time finish_time,
      ConstProfilePtr profile,
      Eigen::Vector3d position,
      Eigen::Vector3d velocity);

  /// Find the Segment of this Trajectory that is active during the given time.
  ///
  /// \note This will return Trajectory::end() if the time is before the
  /// Trajectory starts or after the Trajectory finishes.
  ///
  /// \param[in] time
  ///   The time of interest.
  ///
  /// \return the Segment that is active during the given time, or
  /// Trajectory::end() if the time falls outside the range of the Trajectory.
  iterator find(Time time);

  /// const-qualified version of find()
  const_iterator find(Time intersection_time) const;

  /// Erase the specified segment.
  ///
  /// \return an iterator following the last removed element
  iterator erase(iterator segment);

  /// Erase the range of elements: [first, last).
  ///
  /// \note The `last` element is not included in the range.
  ///
  /// \return an iterator following the last removed element
  iterator erase(iterator first, iterator last);

private:

  class Implementation;
  rmf_utils::impl_ptr<Implementation> _pimpl;

};

//==============================================================================
template<typename SegT>
class Trajectory::base_iterator
{
public:

  /// Dereference operator
  SegT& operator*() const;

  /// Drill-down operator
  SegT* operator->() const;

  /// Pre-increment operator
  base_iterator& operator++();

  /// Pre-decrement operator
  base_iterator& operator--();

  /// Post-increment operator
  base_iterator operator++(int);

  /// Post-decrement operator
  base_iterator operator--(int);


  // TODO(MXG): Switch to the spaceship operator when we can use C++20

  /// Equality comparison operator
  bool operator==(const base_iterator& other) const;

  /// Inequality comparison operator
  bool operator!=(const base_iterator& other) const;

  /// Less-than comparison operator
  bool operator<(const base_iterator& other) const;

  /// Greater-than comparison operator
  bool operator>(const base_iterator& other) const;

  /// Less-than-or-equal comparison operator
  bool operator<=(const base_iterator& other) const;

  /// Greater-than-or-equal comparison operator
  bool operator>=(const base_iterator& other) const;

private:
  base_iterator();
  friend class Trajectory;
  class Implementation;
  rmf_utils::impl_ptr<Implementation> _pimpl;
};

} // namespace rmf_traffic

#endif // RMF_TRAFFIC__TRAJECTORY_HPP