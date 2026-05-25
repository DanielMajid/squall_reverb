# Build configuration for NTS-1 mkII user unit

PROJECT := Squall
PROJECT_TYPE := revfx

UCSRC = src/header.c
UCXXSRC = src/unit.cc src/clouds_reverb.cc

UASMSRC =
UASMXSRC =

UINCDIR = \
  $(PROJECT_ROOT)/src

ULIBS = -lm
UDEFS =

# Clouds DSP headers vendored with this project.
UINCDIR += ./eurorack
UINCDIR += ./eurorack/clouds/dsp
UINCDIR += ./eurorack/clouds/dsp/fx
