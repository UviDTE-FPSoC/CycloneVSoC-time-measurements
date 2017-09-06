#Path of the compiler:
#Compile with the toolchain from Angstrom 2013.12 compilation
CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-
#Compile from SOC EDS toolchain (from Altera Embedded Command Shell )
#CROSS_COMPILE := arm-linux-gnueabihf-

#Generate paths to files in the include folder
STATISTICS_PATH = ../../inc/Statistics
PMU_PATH = ../../inc/PMU
CC = $(CROSS_COMPILE)gcc
ARCH= arm
LDFLAGS =  -g -Wall -I$(STATISTICS_PATH) -I$(PMU_PATH)

time_measurements_DMA: time_measurements_DMA.o pmu.o statistics.o
	$(CC) $(LDFLAGS) -o time_measurements_DMA time_measurements_DMA.o pmu.o statistics.o

time_measurements_DMA.o: time_measurements_DMA.c $(PMU_PATH)/pmu.h $(STATISTICS_PATH)/statistics.h
	$(CC) $(LDFLAGS) -c time_measurements_DMA.c

statistics.o: $(STATISTICS_PATH)/statistics.c $(STATISTICS_PATH)/statistics.h
	$(CC) $(LDFLAGS) -c $(STATISTICS_PATH)/statistics.c

pmu.o: $(PMU_PATH)/pmu.c $(PMU_PATH)/pmu.h
	$(CC) $(LDFLAGS) -c $(PMU_PATH)/pmu.c

.PHONY: clean
clean:
	@rm -f time_measurements_DMA time_measurements_DMA.o pmu.o statistics.o
