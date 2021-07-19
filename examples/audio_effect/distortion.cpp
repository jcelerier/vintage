/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/audio_effect.hpp>

#include <cmath>

struct TanhDistortion
{
  // General metadata
  static constexpr auto name = "Tanh Distortion";
  static constexpr auto vendor = "jcelerier";
  static constexpr auto product = "1.0";
  static constexpr auto category = vintage::PlugCategory::Effect;
  static constexpr auto version = 1;
  static constexpr auto unique_id = 0xBA55E5;
  static constexpr auto channels = 2;

  // Will be set to the correct values.
  // If you want a notification upon change,
  // define instead a more intelligent class with an active operator=
  int32_t sample_rate = 0;
  int32_t buffer_size = 0;
  int32_t current_program = 0;

  // Definition of the controls
  struct
  {
    // A first control
    struct
    {
      constexpr auto name() const noexcept { return "Preamplification"; }
      constexpr auto label() const noexcept { return "Preamp"; }
      constexpr auto short_label() const noexcept { return "Preamp"; }
      auto display(char* data) const noexcept
      {
        snprintf(
            data, vintage::Constants::ParamStrLen, "%d dB", int(value * 100));
      }

      float value{0.5};
    } preamp;

    // A second control
    struct
    {
      constexpr auto name() const noexcept { return "Volume"; }

      float value{1.0};
    } volume;
  } parameters;

  // Definition of the presets
  struct
  {
    std::string_view name;
    decltype(TanhDistortion::parameters) parameters;
  } programs[2]{
      {.name{"Low gain"}, .parameters{.preamp = {0.3}, .volume = {0.6}}},
      {.name{"Hi gain"}, .parameters{.preamp = {1.0}, .volume = {1.0}}},
  };

  // Main processing function, will be generated for the float and double cases
  template <typename sample_t>
  void process(sample_t** inputs, sample_t** outputs, int32_t frames)
  {
    const sample_t preamp = 100. * parameters.preamp.value;
    const sample_t volume = parameters.volume.value;

    for (int32_t c = 0; c < channels; c++)
      for (int32_t i = 0; i < frames; i++)
        outputs[c][i] = volume * std::tanh(inputs[c][i] * preamp);
  }
};

VINTAGE_DEFINE_EFFECT(TanhDistortion)
