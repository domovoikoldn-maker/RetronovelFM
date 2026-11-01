#!/usr/bin/make -f
NAME = RetroNovelFM
FILES_DSP = RetroNovelFM.cpp
FILES_UI  = RetroNovelFMUI.cpp
include ../../dpf/Makefile.plugins.mk
ifeq ($(BUILD_JACK),true)
TARGETS += jack
endif
TARGETS += vst2 vst3 clap
ifeq ($(MACOS),true)
TARGETS += au
endif
all: $(TARGETS)
