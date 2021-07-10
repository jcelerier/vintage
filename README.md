# vintage - a declarative API for C++ audio plug-ins

This is an experiment in seeing how far modern C++ features allow to 
write purely declarative code and introspect this code through 
various reflection-like features.

## Building plug-ins

```
$ g++ \
  examples/audio_effect/distortion.cpp \
  -std=c++20 \
  -I include/ \
  -fPIC \
  -shared \
  -o Distortion.so
``` 

## Building very very smol plug-ins

The example plug-in can be as small as 6.4kb on Linux: 

```
$ clang++ \
  examples/audio_effect/distortion.cpp \
  -I include/ \
  -O3 \
  -march=native \
  -std=c++20 \
  -shared \
  -s \
  -Bsymbolic \
  -fvisibility=internal \
  -fvisibility-inlines-hidden \
  -fvisibility-inlines-hidden-static-local-var \
  -fstruct-path-tbaa \
  -fstrict-aliasing \
  -fstrict-vtable-pointers \
  -fno-semantic-interposition \
  -fno-stack-protector \
  -fno-ident \
  -fno-unwind-tables \
  -fno-asynchronous-unwind-tables \
  -fno-plt \
  -fvirtual-function-elimination \
  -flto \
  -fno-exceptions \
  -fno-rtti \
  -ffunction-sections \
  -fdata-sections \
  -fuse-ld=lld \
  -Wl,-O2 \
  -Wl,--lto-O3 \
  -Wl,-z,defs \
  -Wl,--as-needed \
  -Wl,--no-undefined \
  -Wl,--gc-sections \
  -Wl,--icf=all \
  -o Distortion.so

$ du -b Distortion.so
6368	Distortion.so

$ nm -C -D Distortion.so
w __cxa_finalize@GLIBC_2.2.5
w __gmon_start__
w _ITM_deregisterTMCloneTable
w _ITM_registerTMCloneTable
U memmove@GLIBC_2.2.5
U snprintf@GLIBC_2.2.5
U tanh@GLIBC_2.2.5
U tanhf@GLIBC_2.2.5
0000000000001940 T VSTPluginMain
U operator delete(void*)@GLIBCXX_3.4
U operator new(unsigned long)@GLIBCXX_3.4
```

