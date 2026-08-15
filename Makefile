EE_BIN = lavalamp.elf
EE_OBJS = src/main.o src/physics.o src/metaball.o src/input.o src/menu.o
EE_LIBS = -lgskit -ldmakit -lpad -lpatches -lc

EE_CFLAGS += -O2 -Wall -G0 -I$(PS2SDK)/ports/include -I$(PS2SDK)/ee/include
EE_LDFLAGS += -L$(PS2SDK)/ports/lib -L$(PS2SDK)/ee/lib

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
