# Build configuration for NTS-1 mkII user unit
# Uses Make wildcards to auto-discover legacy source files in src/legacy/

PROJECT := Squall
PROJECT_TYPE := revfx

# Auto-compile all C/C++ files in src/legacy/ to avoid manual config updates
UCSRC = src/header.c $(wildcard src/legacy/*.c)
UCXXSRC = src/unit.cc $(wildcard src/legacy/*.cc) $(wildcard src/legacy/*.cpp)

UASMSRC =
UASMXSRC =

UINCDIR = \
  $(PROJECT_ROOT)/src \
  $(PROJECT_ROOT)/src/legacy

ifeq ($(PROJECT_TYPE),osc)
UINCDIR += $(abspath $(TOOLSDIR)/../platform/nutekt-digital/inc)
UINCDIR += $(abspath $(TOOLSDIR)/../platform/nutekt-digital/inc/dsp)
UINCDIR += $(abspath $(TOOLSDIR)/../platform/nutekt-digital/inc/utils)
endif

ULIBS = -lm
UDEFS =
UINCDIR += /Users/majid/repos/test_folder/CLOUDS_REVERB_REMADE/src/eurorack
UINCDIR += /Users/majid/repos/test_folder/CLOUDS_REVERB_REMADE/src/eurorack/clouds/dsp
UINCDIR += /Users/majid/repos/test_folder/CLOUDS_REVERB_REMADE/src/eurorack/clouds/dsp/fx
