SOURCE_DIRS  := src
BUILD_DIR    := build

CPP_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.cpp))
C_FILES   := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
EE_OBJS    := $(C_FILES:%.c=%.o) $(CPP_FILES:%.cpp=%.o)
EE_BIN 	   = 3dpinball-ps2.elf

EE_INCS 	:= -Isrc -I$(PS2SDK)/ports/include
EE_LDFLAGS = -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib
EE_LIBS = -lSDL2 -lpad -lpacket -lgskit -ldmakit -lmtap -lps2_drivers -ldma -lgraph -ldraw -lmc -lc -lstdc++
EE_LINKFILE := $(PS2SDK)/ee/startup/linkfile

EE_IRX_FILES=\
	usbd.irx \
	bdm.irx \
	bdmfs_fatfs.irx \
	usbmass_bd.irx \
	usbhdfsd.irx

EE_IRX_SRCS = $(addsuffix _irx.c, $(basename $(EE_IRX_FILES)))
EE_IRX_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix _irx.o, $(basename $(EE_IRX_FILES))))
EE_OBJS += $(EE_IRX_OBJS)

# Where to find the IRX files
vpath %.irx $(PS2SDK)/iop/irx/

# Rule to generate them
$(BUILD_DIR)/%_irx.o: %.irx
	bin2c $< $(BUILD_DIR)/$*_irx.c $*_irx
	$(EE_CC) -c $(BUILD_DIR)/$*_irx.c -o $(BUILD_DIR)/$*_irx.o

# This is for the sbv patch
SBVLITE = $(PS2SDK)/sbv
EE_INCS += -I$(SBVLITE)/include
EE_LDFLAGS += -L$(SBVLITE)/lib
EE_LIBS += -lpatches

all: $(BUILD_DIR) $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN) $(EE_IRX_SRCS)

$(BUILD_DIR):
	mkdir -p $@

run:
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
