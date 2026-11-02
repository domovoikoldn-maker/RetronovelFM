#!/usr/bin/make -f
NAME = RetroNovelFM
FILES_DSP = RetroNovelFM.cpp RetroFileLoad.cpp $(wildcard samples/*.cpp)
FILES_UI  = RetroNovelFMUI.cpp RetroWinKeys.cpp RetroRender.cpp RetroEncode.cpp pixelart/PixelArtData.cpp
# audio encoders for "export song": MP3 (shine, LGPL) and OGG Vorbis (libogg / libvorbis, BSD)
FILES_UI += $(wildcard thirdparty/shine/*.c) thirdparty/ogg/src/bitwise.c thirdparty/ogg/src/framing.c
FILES_UI += $(filter-out thirdparty/vorbis/lib/psytune.c thirdparty/vorbis/lib/tone.c thirdparty/vorbis/lib/barkmel.c,$(wildcard thirdparty/vorbis/lib/*.c))
include ../../dpf/Makefile.plugins.mk
ENC_INC = -Ithirdparty/ogg/include -Ithirdparty/vorbis/include -Ithirdparty/vorbis/lib
BUILD_C_FLAGS   += $(ENC_INC) -w
BUILD_CXX_FLAGS += $(ENC_INC)
ifeq ($(BUILD_JACK),true)
TARGETS += jack
endif
TARGETS += vst2 vst3 clap
ifeq ($(MACOS),true)
TARGETS += au
endif
all: $(TARGETS)
