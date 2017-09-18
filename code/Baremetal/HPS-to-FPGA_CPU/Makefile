SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

CACHE_PATH = ../../inc/Cache
STATISTICS_PATH = ../../inc/Statistics
PMU_PATH = ../../inc/PMU
IO_PATH = ../../inc/Baremetal_IO
FPGA_PATH = ../../inc/FPGA_system_headers

INC = -I$(CACHE_PATH) -I$(STATISTICS_PATH) -I$(PMU_PATH) -I$(IO_PATH) -I$(FPGA_PATH)

# look for SoC EDS 15.0 directory structure
ifneq (,$(wildcard $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal))
SOCAL_ROOT := $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av
endif

# look for SoC EDS 14.0 directory structure
ifneq (,$(wildcard $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include/socal))
SOCAL_ROOT := $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib/include
endif

ifndef SOCAL_ROOT
$(error SOCAL_ROOT is undefined)
endif

#Program to convert img to bin. this will change from system to system.
#If you are using only preloader to run baremetal app is not needed because
#preloader uses img. When using preloader+uboot to run the application, you
#need to generate the .bin.
#If you are using it after uboot it is not needed.
MKI := mkimage

C_SRC := main.c $(IO_PATH)/io.c $(PMU_PATH)/pmu.c  $(CACHE_PATH)/cache_high_level_API.c $(STATISTICS_PATH)/statistics.c
ASSEMBLER_SRC := $(CACHE_PATH)/arm_cache_modified.s
ALL_SRC  := $(C_SRC) $(ASSEMBLER_SRC)

LINKER_SCRIPT := cycloneV-dk-ram-modified.ld

MULTILIBFLAGS := -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon
CFLAGS  := -g -O0 -Wall -Werror -std=c99 $(MULTILIBFLAGS) -I$(HWLIBS_ROOT)/include -I. -Imsgdma -Iqsys_headers -I$(SOCAL_ROOT) $(INC)
LDFLAGS := -T$(LINKER_SCRIPT) $(MULTILIBFLAGS)

CROSS_COMPILE := arm-altera-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
NM := $(CROSS_COMPILE)nm
OC := $(CROSS_COMPILE)objcopy
OD := $(CROSS_COMPILE)objdump
RM := rm -rf
CP := cp -f

ELF ?= $(basename $(firstword $(C_SRC))).axf
OBJ_O := $(patsubst %.c,%.o,$(C_SRC))
OBJ_ALL := $(patsubst %.c,%.o,$(ALL_SRC))
#BIN = $(basename $(firstword $(C_SRC))).bin
BIN = baremetalapp.bin

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	$(RM) $(ELF) $(OBJ_O) $(BIN) $(BIN).img
	$(RM) *.map
	$(RM) *.objdump

%.c: $(HWLIBS_ROOT)/src/hwmgr/%.c
	$(CP) $< $@

$(OBJ_O): %.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ_O)
	$(LD) $(LDFLAGS) $(OBJ_ALL) -o $@
	$(NM) $@ > $@.map
	$(OD) -d $@ > $@.objdump

$(BIN): $(ELF)
	$(OC) -O binary $(ELF) $(BIN)
	$(MKI) -A arm -O u-boot -T standalone -C none -a 0x00100040 -e 0x00100040 -n "baremetalapp" -d $(BIN)  $(BIN).img
