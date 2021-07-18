#pragma once

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/vintage.hpp>
#include <vintage/helpers.hpp>

namespace vintage
{

template <typename T>
struct PolyphonicSynthesizer: vintage::Effect
{
  T implementation;
  vintage::HostCallback master{};

  SynthControls<T> controls;
  Processor<T> processor;
  Programs<T> programs;

  explicit PolyphonicSynthesizer(vintage::HostCallback master)
      : master{master}
  {
    Effect::dispatcher = [](Effect* effect,
                            int32_t opcode,
                            int32_t index,
                            intptr_t value,
                            void* ptr,
                            float opt) -> intptr_t
    {
      auto& self = *static_cast<PolyphonicSynthesizer*>(effect);
      auto code = static_cast<EffectOpcodes>(opcode);

      if (code == EffectOpcodes::Close)
      {
        delete &self;
        return 1;
      }

      if constexpr (requires {
                      self.implementation.dispatch(
                          nullptr, 0, 0, 0, nullptr, 0.f);
                    })
      {
        return self.implementation.dispatch(
            self, code, index, value, ptr, opt);
      }
      else
      {
        return default_dispatch(self, code, index, value, ptr, opt);
      }
    };

    processor.init(*this);
    controls.init(*this);
    programs.init(*this);

    Effect::numInputs = T::channels;
    Effect::numOutputs = T::channels;
    Effect::numParams = Controls<T>::parameter_count + 3;

    Effect::flags = EffectFlags::CanReplacing | EffectFlags::CanDoubleReplacing | EffectFlags::IsSynth;
    Effect::ioRatio = 1.;
    Effect::object = nullptr;
    Effect::user = nullptr;
    Effect::uniqueID = T::unique_id;
    Effect::version = 1;

    controls.read(implementation);

    voices.reserve(127);
    release_voices.reserve(127);
  }

  intptr_t request(HostOpcodes opcode, int a, int b, void* c, float d)
  {
    return this->master(this, static_cast<int32_t>(opcode), a, b, c, d);
  }

  void midi_input(const vintage::MidiEvent& e)
  {
      int32_t status = e.midiData[0] & 0xf0;

      switch (status) {
      case 0x80: // Note off
      case 0x90: // Note on
      {
          int32_t note = e.midiData[1] & 0x7F;
          int32_t velocity = e.midiData[2] & 0x7F;
          if (status == 0x80 || velocity == 0) {
              for(auto it = voices.begin(); it != voices.end(); )
              {
                  if(it->note == note)
                  {
                      it->implementation.release_frame = it->implementation.elapsed;
                      release_voices.push_back(*it);
                      it = voices.erase(it);
                  }
                  else
                  {
                      ++it;
                  }
              }
          } else {
              voices.push_back({.note = float(note), .velocity = float(velocity), .detune = 0.0f});
              float unison = this->controls.unison_voices * 20.0;
              float detune = this->controls.unison_detune;
              float vol = this->controls.unison_volume;
              for(float i = -unison; i <= unison; i += 2.f)
              {
                  voices.push_back({.note = float(note), .velocity = velocity * vol, .detune = i * (1.f + detune)});
              }
          }
      } break;

      case 0xE0: // Pitch bend
          break;
      case 0xB0: // Controller
          break;
      }
  }


  struct voice {
      float note{};
      float velocity{};
      float detune{};

      typename T::voice implementation;

      template<typename sample_t>
      void process(PolyphonicSynthesizer& self, sample_t** outputs, int32_t frames)
      {
          implementation.frequency = 440. * std::pow(2.0, (note - 69) / 12.0) + detune;
          implementation.volume = velocity / 127.;

          implementation.process(self.implementation, outputs, frames);
      }
  };

  void process(
          std::floating_point auto** inputs,
          std::floating_point auto** outputs,
          int32_t frames)
  {
    // Check if processing is to be bypassed
    if constexpr (requires { implementation.bypass; })
    {
      if (implementation.bypass)
        return;
    }

    // Before processing starts, we copy all our atomics back into the struct
    controls.write(implementation);

    // Clear buffer
    for (int32_t c = 0; c < implementation.channels; c++)
        for (int32_t i = 0; i < frames; i++)
            outputs[c][i] = 0.0;

    // Process voices
    for(auto& voice : voices)
    {
      voice.process(*this, outputs, frames);
    }

    // Process voices that were note'off'd in order to cleanly fade out
    for(auto it = release_voices.begin(); it != release_voices.end(); )
    {
        auto& voice = *it;
        voice.process(*this, outputs, frames);
        if(voice.implementation.recycle)
            it = release_voices.erase(it);
        else
            ++it;
    }

    // Post-processing
    if constexpr(effect_processor<float, T> || effect_processor<double, T>)
    {
      implementation.process(inputs, outputs, frames);
    }
  }

  std::vector<voice> voices;
  std::vector<voice> release_voices;
};
}

#define VINTAGE_DEFINE_SYNTH(EffectMainClass)                       \
  extern "C" VINTAGE_EXPORTED_SYMBOL vintage::Effect* VSTPluginMain( \
      vintage::HostCallback cb)                                      \
  {                                                                  \
    return new vintage::PolyphonicSynthesizer<EffectMainClass>{cb};      \
  }