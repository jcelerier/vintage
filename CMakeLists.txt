cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(vintage)

# Example audio effect
add_library(TanhDistortion SHARED examples/audio_effect/distortion.cpp)

target_compile_features(TanhDistortion PRIVATE cxx_std_20)
target_compile_definitions(TanhDistortion PRIVATE FMT_HEADER_ONLY=1)
target_include_directories(TanhDistortion PRIVATE include)

set_target_properties(
  TanhDistortion
  PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE 1
    VISIBILITY_INLINES_HIDDEN 1
    CXX_VISIBILITY_PRESET hidden
)

add_library(Utility SHARED examples/audio_effect/utility.cpp)

target_compile_features(Utility PRIVATE cxx_std_20)
target_compile_definitions(Utility PRIVATE FMT_HEADER_ONLY=1)
target_include_directories(Utility PRIVATE include)

set_target_properties(
  Utility
  PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE 1
    VISIBILITY_INLINES_HIDDEN 1
    CXX_VISIBILITY_PRESET hidden
)


# Example synthesize
add_library(Osci SHARED examples/synth/osci.cpp)

target_compile_features(Osci PRIVATE cxx_std_20)
target_compile_definitions(Osci PRIVATE FMT_HEADER_ONLY=1)
target_include_directories(Osci PRIVATE include)

set_target_properties(
  Osci
  PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE 1
    VISIBILITY_INLINES_HIDDEN 1
    CXX_VISIBILITY_PRESET hidden
)
