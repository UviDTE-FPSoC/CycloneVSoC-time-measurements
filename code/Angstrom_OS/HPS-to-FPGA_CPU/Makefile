#Path of the compiler:
#Compile with the toolchain from Angstrom 2013.12 compilation
#CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-
#Compile from SOC EDS toolchain (from Altera Embedded Command Shell )
CROSS_COMPILE := arm-linux-gnueabihf-

SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
HWLIBS_INCLUDE = $(HWLIBS_ROOT)/include
# look for SoC EDS 15.0 and higher directory structure
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

#Own library paths
STATISTICS_PATH = ../../inc/Statistics
PMU_PATH = ../../inc/PMU
FPGA_PATH = ../../inc/FPGA_system_headers
#Generate paths to files in the include folder

CC = $(CROSS_COMPILE)gcc
ARCH= arm
LDFLAGS =  -g -pthread -Wall -I$(STATISTICS_PATH) -I$(PMU_PATH) -I$(FPGA_PATH) -I$(SOCAL_ROOT) -I$(HWLIBS_INCLUDE)

time_measurements_CPU: time_measurements_CPU.o pmu.o statistics.o
	$(CC) $(LDFLAGS) -o time_measurements_CPU time_measurements_CPU.o pmu.o statistics.o

time_measurements_CPU.o: time_measurements_CPU.c $(PMU_PATH)/pmu.h $(STATISTICS_PATH)/statistics.h
	$(CC) $(LDFLAGS) -c time_measurements_CPU.c

statistics.o: $(STATISTICS_PATH)/statistics.c $(STATISTICS_PATH)/statistics.h
	$(CC) $(LDFLAGS) -c $(STATISTICS_PATH)/statistics.c

pmu.o: $(PMU_PATH)/pmu.c $(PMU_PATH)/pmu.h
	$(CC) $(LDFLAGS) -c $(PMU_PATH)/pmu.c

.PHONY: clean
clean:
	@rm -f time_measurements_CPU time_measurements_CPU.o pmu.o statistics.o
