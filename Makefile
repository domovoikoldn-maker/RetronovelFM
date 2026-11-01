#!/usr/bin/make -f
all:
	$(MAKE) -C plugins/RetroNovelFM
clean:
	$(MAKE) -C plugins/RetroNovelFM clean
	rm -rf bin build
