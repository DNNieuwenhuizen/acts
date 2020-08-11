// This file is part of the Acts project.
//
// Copyright (C) 2016-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingVolume.hpp"

#include <sstream>

namespace Acts {

/// Simple struct to select volumes
struct VolumeSelector {
  bool selectMaterial = true;
  bool selectLayer = false;
  bool selectPassive = false;

  /// VolumeSelector with options
  ///
  /// @param sMaterial is the directive to select material volumes
  /// @param sLayer is the directive to select volumes with layers
  /// @param sPassive is the directive to select passive volumes
  VolumeSelector(bool sMaterial = true, bool sLayer = false,
                 bool sPassive = false)
      : selectMaterial(sMaterial),
        selectLayer(sLayer),
        selectPassive(sPassive) {}

  /// Call operator to check if a volume should be selected
  ///
  /// @param volume is the test volume
  bool operator()(const Acts::TrackingVolume& volume) const {
    if (selectMaterial && volume.volumeMaterial() != nullptr) {
      return true;
    }
    if (selectLayer && volume.confinedLayers() != nullptr) {
      return true;
    }
    if (selectPassive) {
      return true;
    }
    return false;
  }
};

/// The information to be writtern out per hit volume
struct VolumeHit {
  const TrackingVolume* volume = nullptr;
  Vector3D position;
  Vector3D direction;
};

/// A Volume Collector struct
/// templated with a Selector type
///
/// Whenever a volume is passed in the propagation
/// that satisfies the selector, it is recorded
/// for further usage in the flow.
template <typename Selector = VolumeSelector>
struct VolumeCollector {
  /// The selector used for this volume
  Selector selector;

  /// Simple result struct to be returned
  /// It has all the VolumeHit objects that
  /// are collected (and thus have been selected)
  struct this_result {
    std::vector<VolumeHit> collected;
  };

  using result_type = this_result;

  /// Collector action for the ActionList of the Propagator
  /// It checks if the propagator state has a current volume,
  /// in which case the action is performed:
  /// - it records the volume given the configuration
  ///
  /// @tparam propagator_state_t is the type of Propagator state
  /// @tparam stepper_t Type of the stepper used for the propagation
  ///
  /// @param [in,out] state is the mutable stepper state object
  /// @param [in] stepper The stepper in use
  /// @param [in,out] result is the mutable result object
  template <typename propagator_state_t, typename stepper_t>
  void operator()(propagator_state_t& state, const stepper_t& stepper,
                  result_type& result) const {
    const auto& logger = state.options.logger;
    // The current volume has been assigned by the navigator
    if (state.navigation.currentVolume &&
        selector(*state.navigation.currentVolume)) {
      // Create for recording
      VolumeHit volume_hit;
      volume_hit.volume = state.navigation.currentVolume;
      volume_hit.position = stepper.position(state.stepping);
      volume_hit.direction = stepper.direction(state.stepping);
      bool save = true;
      // Check if the Volume ws already encountered
      for (auto const& res : result.collected) {
        if (res.volume == volume_hit.volume) {
          save = false;
          break;
        }
      }
      // Save if in the result if it does not already exist
      if (save) {
        result.collected.push_back(volume_hit);
        // Screen output
        ACTS_VERBOSE("Collect volume  "
                     << state.navigation.currentVolume->geoID());
      }
    }
  }

  /// Pure observer interface
  /// - this does not apply to the volume collector
  template <typename propagator_state_t, typename stepper_t>
  void operator()(propagator_state_t& /*state*/,
                  const stepper_t& /*unused*/) const {}
};

}  // namespace Acts
