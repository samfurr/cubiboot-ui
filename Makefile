.PHONY: preview-python-deps preview-build preview-run preview clean

DEVKITPRO ?= /opt/devkitpro
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC
PREVIEW_VENV := $(CURDIR)/.venv
PREVIEW_CACHE := $(CURDIR)/.cache

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
