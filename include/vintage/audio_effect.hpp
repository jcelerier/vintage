#pragma once

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/vintage.hpp>
#include <vintage/helpers.hpp>

namespace vintage
{

template <typename T>
struct SimpleAudioEffect : vintage::Effect
{
  T implementation;
  vintage::HostCallback master{};

  Controls<T> controls;
  Processor<T> processor;
  Programs<T> programs;

  explicit SimpleAudioEffect(vintage::HostCallback master)
      : master{master}
  {
    Effect::dispatcher = [](Effect* effect,
                            int32_t opcode,
                            int32_t index,
                            intptr_t value,
                            void* ptr,
                            float opt) -> intptr_t
    {
      auto& self = *static_cast<SimpleAudioEffect*>(effect);
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
    Effect::numParams = Controls<T>::parameter_count;

    Effect::flags = EffectFlags::CanReplacing | EffectFlags::CanDoubleReplacing;
    Effect::ioRatio = 1.;
    Effect::object = nullptr;
    Effect::user = nullptr;
    Effect::uniqueID = T::unique_id;
    Effect::version = 1;

    if constexpr (requires { implementation.sample_rate = 44100; })
      implementation.sample_rate
          = request(HostOpcodes::GetSampleRate, 0, 0, nullptr, 0.f);

    if constexpr (requires { implementation.buffer_size = 512; })
      implementation.buffer_size
          = request(HostOpcodes::GetBlockSize, 0, 0, nullptr, 0.f);

    controls.read(implementation);
  }

  intptr_t request(HostOpcodes opcode, int a, int b, void* c, float d)
  {
    return this->master(this, static_cast<int32_t>(opcode), a, b, c, d);
  }


  void process(
          std::floating_point auto** inputs,
          std::floating_point auto** outputs,
          int32_t sampleFrames)
  {
    // Check if processing is to be bypassed
    if constexpr (requires { implementation.bypass; })
    {
      if (implementation.bypass)
        return;
    }

    // Before processing starts, we copy all our atomics back into the struct
    controls.write(implementation);

    // Actual processing
    if constexpr(requires { implementation.process(inputs, outputs, sampleFrames); })
    {
      return implementation.process(inputs, outputs, sampleFrames);
    }
    else if constexpr(requires { outputs[0][0] = implementation.process(inputs[0][0]); })
    {
      for(int32_t c = 0; c < implementation.channels; ++c)
      {
        for(int32_t i = 0; i < sampleFrames; i++)
        {
            outputs[c][i] = implementation.process(inputs[c][i]);
        }
      }
    }
  }
};
}

#define VINTAGE_DEFINE_EFFECT(EffectMainClass)                       \
  extern "C" VINTAGE_EXPORTED_SYMBOL vintage::Effect* VSTPluginMain( \
      vintage::HostCallback cb)                                      \
  {                                                                  \
    return new vintage::SimpleAudioEffect<EffectMainClass>{cb};      \
  }
