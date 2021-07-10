#pragma once

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <vintage/vintage.hpp>

#include <boost/pfr.hpp>

#include <algorithm>
#include <atomic>
#include <string_view>

namespace vintage
{
template <std::size_t N>
struct limited_string_view : std::string_view
{
  using std::string_view::string_view;
  template <std::size_t M>
  constexpr limited_string_view(const char (&str)[M])
      : std::string_view{str, M}
  {
    static_assert(M < N, "A name is too long");
  }
  constexpr limited_string_view(std::string_view str)
      : std::string_view{str.data(), std::min(str.size(), N)}
  {
  }

  void copy_to(void* dest) const noexcept
  {
    std::copy_n(data(), size(), reinterpret_cast<char*>(dest));
  }
};

template <std::size_t N>
struct limited_string : std::string
{
  using std::string::string;
  constexpr limited_string(std::string&& str) noexcept
      : std::string{std::move(str)}
  {
    if (this->size() > N)
      this->resize(N);
  }

  void copy_to(void* dest) const noexcept
  {
    std::copy_n(data(), size() + 1, reinterpret_cast<char*>(dest));
  }
};

using program_name = limited_string_view<vintage::Constants::ProgNameLen>;
using param_display = limited_string<vintage::Constants::ParamStrLen>;
using vendor_name = limited_string_view<vintage::Constants::VendorStrLen>;
using product_name = limited_string_view<vintage::Constants::ProductStrLen>;
using effect_name = limited_string_view<vintage::Constants::EffectNameLen>;
using name = limited_string_view<vintage::Constants::NameLen>;
using label = limited_string_view<vintage::Constants::LabelLen>;
using short_label = limited_string_view<vintage::Constants::ShortLabelLen>;
using category_label = limited_string_view<vintage::Constants::CategLabelLen>;
using file_name = limited_string_view<vintage::Constants::FileNameLen>;

template <typename Parameters, typename F>
void for_nth_parameter(Parameters& params, int n, F&& func) noexcept
{
  [=]<std::size_t... Index>(std::integer_sequence<std::size_t, Index...>)
  {
    ((void)(Index == n && (func(boost::pfr::get<Index>(params)), true)), ...);
  }
  (std::make_index_sequence<boost::pfr::tuple_size_v<Parameters>>());
}

template <typename Effect>
intptr_t default_dispatch(
    Effect& eff,
    EffectOpcodes opcode,
    int32_t index,
    intptr_t value,
    void* ptr,
    float opt)
{
  auto& self = eff.implementation;
  // std::cerr << int(opcode) << ": " << index << " ; " << value << " ;" << ptr
  //           << " ;" << opt << std::endl;
  switch (opcode)
  {
    case EffectOpcodes::Identify: // 22
      return 0;
    case EffectOpcodes::SetProcessPrecision: // 77
    {
      if constexpr (requires
                    { self.precision = vintage::ProcessPrecision::Single; })
        self.precision
            = value ? ProcessPrecision::Double : ProcessPrecision::Single;
      return 1;
    }
    case EffectOpcodes::SetBlockSizeAndSampleRate: // 43
    {
      if constexpr (requires { self.buffer_size = 512; })
        self.buffer_size = value;
      return 1;
    }
    case EffectOpcodes::SetSampleRate: // 10
    {
      if constexpr (requires { self.sample_rate = 44100; })
        self.sample_rate = opt;
      return 1;
    }
    case EffectOpcodes::SetBlockSize: // 11
    {
      if constexpr (requires { self.sample_rate = 44100; })
        self.sample_rate = opt;
      if constexpr (requires { self.buffer_size = 512; })
        self.buffer_size = value;
      return 1;
    }
    case EffectOpcodes::Open: // 0
      return 1;
    case EffectOpcodes::GetPlugCategory: // 35
    {
      if constexpr (requires { self.category; })
        return static_cast<int32_t>(self.category);
      return 0;
    }

    case EffectOpcodes::ConnectInput: // 31
      return 1;
    case EffectOpcodes::ConnectOutput: // 32
      return 1;

    case EffectOpcodes::GetMidiKeyName: // 66
      return 1;

    case EffectOpcodes::GetProgram: // 3
    {
      if constexpr (requires { self.current_program; })
      {
        return self.current_program;
      }
      return 0;
    }

    case EffectOpcodes::SetProgram: // 2
    {
      if constexpr (requires { self.current_program; })
      {
        if (value < std::ssize(self.programs))
        {
          self.current_program = value;
          self.parameters = self.programs[value].parameters;
          eff.copy_all_parameters_to_atomics();
          eff.request(HostOpcodes::UpdateDisplay, 0, 0, nullptr, 0.f);
        }
        else
        {
          self.current_program = 0;
        }
      }
      else
      {
        self.current_program = 0;
      }
      return 0;
    }

    case EffectOpcodes::BeginSetProgram: // 67
    {
      break;
    }
    case EffectOpcodes::EndSetProgram: // 68
    {
      break;
    }

    case EffectOpcodes::SetBypass: // 44
    {
      if constexpr (requires { self.bypass = true; })
      {
        self.bypass = bool(value);
      }
      break;
    }
    case EffectOpcodes::MainsChanged: // 12
      return 0;

    case EffectOpcodes::StartProcess: // 71
      return 0;
    case EffectOpcodes::StopProcess: // 72
      return 0;

    case EffectOpcodes::GetInputProperties: // 33
      return 0;
    case EffectOpcodes::GetOutputProperties: // 34
      return 0;
    case EffectOpcodes::GetParameterProperties: // 56
    {
      auto& props = *(vintage::ParameterProperties*)ptr;

      for_nth_parameter(
          self.parameters,
          index,
          [&props, index](const auto& param)
          {
            props.stepFloat = 0.01;
            props.smallStepFloat = 0.01;
            props.largeStepFloat = 0.01;

            if constexpr (requires { param.label(); })
            {
              vintage::label{param.label()}.copy_to(props.label);
            }
            props.flags = {};
            props.minInteger = 0;
            props.maxInteger = 1;
            props.stepInteger = 1;
            props.largeStepInteger = 1;

            if constexpr (requires { param.shortLabel(); })
            {
              vintage::short_label{param.shortLabel()}.copy_to(
                  props.shortLabel);
            }
            props.displayIndex = index;
            props.category = 0;
            props.numParametersInCategory = 2;
            if constexpr (requires { param.categoryLabel(); })
            {
              vintage::category_label{param.categoryLabel()}.copy_to(
                  props.categoryLabel);
            }
          });
      return 1;
    }
    case EffectOpcodes::CanBeAutomated: // 26
      return 1;
    case EffectOpcodes::GetEffectName: // 45
    {
      vintage::name{self.name}.copy_to(ptr);
      return 1;
    }

    case EffectOpcodes::GetVendorString: // 47
    {
      vintage::vendor_name{self.vendor}.copy_to(ptr);
      return 1;
    }

    case EffectOpcodes::GetProductString: // 48
    {
      vintage::product_name{self.product}.copy_to(ptr);
      return 1;
    }

    case EffectOpcodes::GetProgramName: // 5
    {
      if constexpr (
          requires { self.current_program; }
          && requires { std::ssize(self.programs); })
      {
        if (self.current_program < std::ssize(self.programs))
        {
          vintage::program_name{self.programs[self.current_program].name}
              .copy_to(ptr);
          return 1;
        }
      }
      break;
    }

    case EffectOpcodes::GetProgramNameIndexed: // 29
    {
      if constexpr (requires { std::ssize(self.programs); })
      {
        if (index < std::ssize(self.programs))
        {
          vintage::program_name{self.programs[index].name}.copy_to(ptr);
          return 1;
        }
      }
      break;
    }

    case EffectOpcodes::GetParamLabel: // 6
    {
      for_nth_parameter(
          self.parameters,
          index,
          [ptr](const auto& param)
          {
            if constexpr (requires { param.label(); })
            {
              vintage::label{param.label()}.copy_to(ptr);
            }
          });

      return 1;
    }

    case EffectOpcodes::GetParamName: // 8
    {
      for_nth_parameter(
          self.parameters,
          index,
          [ptr](const auto& param)
          {
            if constexpr (requires { param.name(); })
            {
              vintage::name{param.name()}.copy_to(ptr);
            }
          });
      return 1;
    }

    case EffectOpcodes::GetParamDisplay: // 7
    {
      for_nth_parameter(
          self.parameters,
          index,
          [ptr](const auto& param)
          {
            if constexpr (requires { param.display((char*) nullptr); })
            {
              param.display(reinterpret_cast<char*>(ptr));
            }
            else if constexpr (requires { param.display(); })
            {
              vintage::param_display{param.display()}.copy_to(ptr);
            }
            else
            {
              snprintf(reinterpret_cast<char*>(ptr), vintage::Constants::ParamStrLen, "%.2f", param.value);
            }
          });
      return 1;
    }

    case EffectOpcodes::GetVendorVersion: // 49
      return self.version;
    case EffectOpcodes::GetApiVersion: // 58
      return Constants::ApiVersion;
    case EffectOpcodes::CanDo: // 51
    {
      /*
      "hasCockosExtensions";
      "sendVstEvents";
      "sendVstMidiEvent";
      "receiveVstEvents";
      "receiveVstMidiEvent";
      "receiveVstSysexEvent";
      "MPE";

      fmt::print(" --> Can do ? {} \n", (const char*)ptr);
      */
      return 0;
    }

    default:
      break;
  }
  return 0;
}

template <typename T>
struct SimpleAudioEffect : vintage::Effect
{
  T implementation;
  vintage::HostCallback master{};

  static const constexpr int32_t parameter_count
      = boost::pfr::tuple_size_v<decltype(T::parameters)>;
  std::atomic<float> parameters[parameter_count];

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

    if constexpr (requires {
                    implementation.process(
                        (float**){}, (float**){}, int32_t{});
                  })
    {
      Effect::process = [](Effect* effect,
                           float** inputs,
                           float** outputs,
                           int32_t sampleFrames)
      {
        auto& self = *static_cast<SimpleAudioEffect*>(effect);
        return self.process(inputs, outputs, sampleFrames);
      };

      Effect::processReplacing = [](Effect* effect,
                                    float** inputs,
                                    float** outputs,
                                    int32_t sampleFrames)
      {
        auto& self = *static_cast<SimpleAudioEffect*>(effect);
        return self.process(inputs, outputs, sampleFrames);
      };
    }

    if constexpr (requires {
                    implementation.process(
                        (double**){}, (double**){}, int32_t{});
                  })
    {
      Effect::processDoubleReplacing = [](Effect* effect,
                                          double** inputs,
                                          double** outputs,
                                          int32_t sampleFrames)
      {
        auto& self = *static_cast<SimpleAudioEffect*>(effect);
        return self.process(inputs, outputs, sampleFrames);
      };
    }

    Effect::setParameter
        = [](Effect* effect, int32_t index, float parameter) noexcept
    {
      auto& self = *static_cast<SimpleAudioEffect*>(effect);

      if (index < parameter_count)
        self.parameters[index].store(parameter, std::memory_order_release);
    };

    Effect::getParameter = [](Effect* effect, int32_t index) noexcept
    {
      auto& self = *static_cast<SimpleAudioEffect*>(effect);

      if (index < parameter_count)
        return self.parameters[index].load(std::memory_order_acquire);
      else
        return 0.f;
    };

    if constexpr (requires { std::size(implementation.programs); })
    {
      Effect::numPrograms = std::size(implementation.programs);
    }

    Effect::numInputs = T::channels;
    Effect::numOutputs = T::channels;
    Effect::numParams = boost::pfr::tuple_size_v<decltype(T::parameters)>;

    Effect::flags
        = EffectFlags::CanReplacing | EffectFlags::CanDoubleReplacing;
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

    copy_all_parameters_to_atomics();
  }

  intptr_t request(HostOpcodes opcode, int a, int b, void* c, float d)
  {
    return this->master(this, static_cast<int32_t>(opcode), a, b, c, d);
  }

  template <typename sample_t>
  void process(sample_t** inputs, sample_t** outputs, int32_t sampleFrames)
  {
    // Check if processing is to be bypassed
    if constexpr (requires { implementation.bypass; })
    {
      if (implementation.bypass)
        return;
    }

    // Before processing starts, we copy all our atomics back into the struct
    load_all_parameters();

    // Actual processing
    return implementation.process(inputs, outputs, sampleFrames);
  }

  void copy_all_parameters_to_atomics()
  {
    [this]<std::size_t... Index>(std::integer_sequence<std::size_t, Index...>)
    {
      auto& sink = this->parameters;
      auto& source = implementation.parameters;
      (sink[Index].store(
           boost::pfr::get<Index>(source).value, std::memory_order_relaxed),
       ...);
    }
    (std::make_index_sequence<parameter_count>());

    std::atomic_thread_fence(std::memory_order_release);
  }

  void load_all_parameters()
  {
    std::atomic_thread_fence(std::memory_order_acquire);

    [this]<std::size_t... Index>(std::integer_sequence<std::size_t, Index...>)
    {
      auto& source = this->parameters;
      auto& sink = implementation.parameters;
      ((boost::pfr::get<Index>(sink).value
        = source[Index].load(std::memory_order_relaxed)),
       ...);
    }
    (std::make_index_sequence<parameter_count>());
  }
};
}

#if defined(_WIN32)
#define VINTAGE_EXPORTED_SYMBOL __declspec(dllexport)
#else
#define VINTAGE_EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif

#define VINTAGE_DEFINE_EFFECT(EffectMainClass)                       \
  extern "C" VINTAGE_EXPORTED_SYMBOL vintage::Effect* VSTPluginMain( \
      vintage::HostCallback cb)                                      \
  {                                                                  \
    return new vintage::SimpleAudioEffect<EffectMainClass>{cb};      \
  }
