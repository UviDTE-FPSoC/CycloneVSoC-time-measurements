CROSS_COMPILE := ~/angstrom-socfpga/build/tmp-angstrom_v2013_12-eglibc/sysroots/x86_64-linux/usr/bin/armv7ahf-vfp-neon-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
ARCH= arm
LDFLAGS =  -g -Wall

pmu.o: pmu.c pmu.h
	$(CC) -c pmu.c
