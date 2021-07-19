/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/polyphonic_synth.hpp>

#include <cmath>

struct Osci
{
  // General metadata
  static constexpr auto name = "Oscillate";
  static constexpr auto vendor = "jcelerier";
  static constexpr auto product = "1.0";
  static constexpr auto category = vintage::PlugCategory::Synth;
  static constexpr auto version = 1;
  static constexpr auto unique_id = 0xFACADE;
  static constexpr auto channels = 2;

  int32_t sample_rate = 0;
  int32_t buffer_size = 0;

  // Definition of the controls
  struct
  {
    // Global volume
    struct
    {
      constexpr auto name() const noexcept { return "Attack"; }
      float value{0.2};
    } attack;
    struct
    {
      constexpr auto name() const noexcept { return "Release"; }
      float value{1.};
    } release;
    struct
    {
      constexpr auto name() const noexcept { return "Volume"; }
      float value{1.0};
    } volume;
  } parameters;

  struct voice
  {
    float frequency{};
    float volume{};
    float pan[channels]{};
    int32_t elapsed{};
    int32_t release_frame{-1};
    bool recycle{};

    float phase{};

    // Arguments are the end sample of each
    template <typename sample_t>
    auto envelope(int32_t attack, int32_t sustain, int32_t release)
    {
      if (elapsed < attack)
      {
        return sample_t(elapsed) / sample_t(attack);
      }
      else
      {
        if (sustain < 0 || elapsed < sustain)
        {
          return sample_t{1.0};
        }
        else
        {
          const int32_t res = release - sustain;
          if (res > 0 && elapsed < release)
          {
            return sample_t(1.) - sample_t(elapsed - sustain) / sample_t(res);
          }
          else
          {
            recycle = true;
            return sample_t{0.0};
          }
        }
      }
    }

    // Main processing function, will be generated for the float and double cases
    template <typename sample_t>
    void process(Osci& synth, sample_t** outputs, int32_t frames)
    {
      const sample_t vol = this->volume * synth.parameters.volume.value;
      const sample_t phi = 2. * vintage::pi * frequency / synth.sample_rate;
      const int32_t a
          = synth.parameters.attack.value * 0.1 * synth.sample_rate;
      const int32_t s = release_frame;
      const int32_t r
          = release_frame
            + (0.001 + synth.parameters.release.value) * synth.sample_rate;

      for (int32_t i = 0; i < frames; i++)
      {
        sample_t sample = vol * envelope<sample_t>(a, s, r) * std::sin(phase);
        for (int32_t c = 0; c < channels; c++)
          outputs[c][i] += sample * pan[c];
        phase += phi;
        elapsed++;
      }
    }
  };
};

VINTAGE_DEFINE_SYNTH(Osci)
