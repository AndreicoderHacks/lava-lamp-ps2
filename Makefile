EE_BIN = lavalamp.elf
EE_OBJS = src/main.o src/physics.o src/metaball.o src/input.o src/menu.o

# gsKit (+ its bundled dmaKit) is NOT part of ps2sdk -- it's a separate
# project installed under $(GSKIT) (e.g. /usr/local/ps2dev/gsKit in the
# ps2dev/ps2dev image; the image already exports this env var).
GSKIT ?= /usr/local/ps2dev/gsKit

# lib names are lowercase (libgskit.a, libgskit_toolkit.a, libdmakit.a) --
# gsKit_toolkit before gsKit before dmakit, since gsKit's fontm/print
# helpers (gsKit_fontm_*) live in gskit_toolkit and depend on gskit symbols.
EE_LIBS = -lgskit_toolkit -lgskit -ldmakit -lpad -lpatches -lc

EE_CFLAGS += -O2 -Wall -G0 \
	-I$(GSKIT)/include \
	-I$(GSKIT)/ee/dma/include \
	-I$(GSKIT)/ee/gs/include \
	-I$(GSKIT)/ee/toolkit/include \
	-I$(PS2SDK)/ports/include \
	-I$(PS2SDK)/ee/include
EE_LDFLAGS += -L$(GSKIT)/lib -L$(PS2SDK)/ports/lib -L$(PS2SDK)/ee/lib

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
