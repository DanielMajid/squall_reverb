PROJECT ?= Squall
MODULE ?= revfx
VERSION ?= 1.1.0

LOGUE_SDK_DIR ?= ./logue-sdk

SDK_PLATFORM_DIR := $(LOGUE_SDK_DIR)/platform/nts-1_mkii
SDK_TEMPLATE_MAKEFILE := $(SDK_PLATFORM_DIR)/$(if $(filter osc,$(MODULE)),dummy-osc,$(if $(filter modfx,$(MODULE)),dummy-modfx,$(if $(filter delfx,$(MODULE)),dummy-delfx,dummy-revfx)))/Makefile

UNIT_FILE := $(PROJECT).nts1mkiiunit
PACKAGE := $(PROJECT)-$(VERSION).zip

SDK_MAKE_ARGS := \
  PROJECT_ROOT=$(CURDIR) \
  COMMON_INC_PATH=$(SDK_PLATFORM_DIR)/common \
  COMMON_SRC_PATH=$(SDK_PLATFORM_DIR)/common \
  TOOLSDIR=$(LOGUE_SDK_DIR)/tools \
  EXTDIR=$(LOGUE_SDK_DIR)/platform/ext \
  CMSISDIR=$(LOGUE_SDK_DIR)/platform/ext/CMSIS/CMSIS \
  LDDIR=$(SDK_PLATFORM_DIR)/ld \
  INSTALLDIR=$(CURDIR)

.PHONY: all install clean package check-sdk
check-sdk:
	@if [ ! -f "$(SDK_TEMPLATE_MAKEFILE)" ]; then \
	  echo "Error: SDK template not found: $(SDK_TEMPLATE_MAKEFILE)"; \
	  echo "Set LOGUE_SDK_DIR to your local logue-sdk path."; \
	  exit 1; \
	fi

all: check-sdk
	@$(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) all

install: check-sdk
	@$(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) install

clean:
	@if [ -f "$(SDK_TEMPLATE_MAKEFILE)" ]; then \
	  $(MAKE) -f "$(SDK_TEMPLATE_MAKEFILE)" $(SDK_MAKE_ARGS) clean; \
	else \
	  echo "Skipping SDK clean (template Makefile not found)."; \
	fi
	@rm -f "$(PACKAGE)"

package: install
	@echo Packaging $(UNIT_FILE) -\> $(PACKAGE)
	@rm -f "$(PACKAGE)"
	@src_files="$$(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) | sort)"; \
	meta_files=""; \
	for f in Makefile config.mk README.md LICENSE .gitmodules; do \
	  if [ -f "$$f" ]; then \
	    meta_files="$$meta_files $$f"; \
	  fi; \
	done; \
	opt_files=""; \
	if [ -f src/credits.txt ]; then \
	  opt_files="$$opt_files src/credits.txt"; \
	fi; \
	zip -q9 "$(PACKAGE)" "$(UNIT_FILE)" $$src_files $$meta_files $$opt_files
	@echo Done
