#!/usr/bin/make -f
NAME = RetroNovelFM
FILES_DSP = RetroNovelFM.cpp
include ../../dpf/Makefile.plugins.mk
TARGETS += vst2 vst3 clap
ifeq ($(MACOS),true)
TARGETS += au
endif
all: $(TARGETS)
