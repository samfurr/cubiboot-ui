.PHONY: preview-python-deps preview-build preview-run preview clean test

DEVKITPRO ?= /opt/devkitpro
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC
PREVIEW_VENV := $(CURDIR)/.venv
PREVIEW_CACHE := $(CURDIR)/.cache
HOST_CC ?= cc

export DEVKITPRO
export DEVKITPPC

preview-python-deps: $(PREVIEW_VENV)/.ready

$(PREVIEW_VENV)/.ready: patches/scripts/requirements.txt
	python3 -m venv $(PREVIEW_VENV)
	$(PREVIEW_VENV)/bin/python -m pip install -r $<
	@touch $@

preview-build: preview-python-deps
	@mkdir -p $(PREVIEW_CACHE)/go-build
	$(MAKE) -C entry clean
	PATH="$(PREVIEW_VENV)/bin:$$PATH" GOCACHE="$(PREVIEW_CACHE)/go-build" $(MAKE) -C entry DOLPHIN_PREVIEW=1

preview-run: preview-build
	./scripts/dolphin-preview.sh

preview: preview-run

clean:
	$(MAKE) -C entry clean

test:
	@mkdir -p $(PREVIEW_CACHE)
	$(HOST_CC) -Wall -Wextra -Werror -I patches/source tests/title_layout_test.c patches/source/title_layout.c -o $(PREVIEW_CACHE)/title-layout-test
	$(PREVIEW_CACHE)/title-layout-test
	$(HOST_CC) -Wall -Wextra -Werror -I patches/source tests/menu_input_test.c patches/source/menu_input.c -o $(PREVIEW_CACHE)/menu-input-test
	$(PREVIEW_CACHE)/menu-input-test
	$(HOST_CC) -Wall -Wextra -Werror -I patches/source tests/ipl_pad_hook_test.c patches/source/ipl_pad_hook.c -o $(PREVIEW_CACHE)/ipl-pad-hook-test
	$(PREVIEW_CACHE)/ipl-pad-hook-test
	$(HOST_CC) -Wall -Wextra -Werror -I patches/source tests/menu_motion_test.c patches/source/menu_motion.c -o $(PREVIEW_CACHE)/menu-motion-test
	$(PREVIEW_CACHE)/menu-motion-test
