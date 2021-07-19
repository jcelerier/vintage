/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/audio_effect.hpp>

#include <cmath>

struct Utility
{
  // General metadata
  static constexpr auto name = "Utility";
  static constexpr auto vendor = "jcelerier";
  static constexpr auto product = "1.0";
  static constexpr auto category = vintage::PlugCategory::Effect;
  static constexpr auto version = 1;
  static constexpr auto unique_id = 0xACCEDED;
  static constexpr auto channels = 2;

  // Definition of the controls
  struct
  {
    // A second control
    struct
    {
      constexpr auto name() const noexcept { return "Volume"; }
      float value{1.0};
    } volume;
    struct
    {
      constexpr auto name() const noexcept { return "Phase invert"; }
      auto display() const noexcept { return value ? "Inverted" : "Normal"; }
      float value{0.};
    } phase;
  } parameters;

  auto process(std::floating_point auto input)
  {
    return (parameters.volume.value)
           * (parameters.phase.value > 0.5f ? 1 - input : input);
  }
};

VINTAGE_DEFINE_EFFECT(Utility)
