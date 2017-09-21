Baremetal/HPS-to-CPU
====================

Introduction
------------
This baremetal application measures the data transfer rate between HPS and FPGA using CPU with memcpy or for loop method to do the transfers. Time measurement is done by Performance Monitoring Unit (PMU) because it is the most precise timer in the system.

The following parameters affecting the transfer speed are studied:
* Method: memcpy() function or *for loop* method are used. *For loop* method consists on using a for loop copying one word per loop iteration.
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory).
* Transfer size: From 2B to 2MB in 2x steps.

Transfers are performed from processor memories (cache/SDRAM) and an On-CHip RAM (OCR) in the FPGA. The FPGA hardware project used is available in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K).
The memory in the FPGA has the following characteristics:
* Implemented using embedded 10kB memory blocks.
* Size = 256kB, the maximum power of two feasible in DE1-SoC board.
* Connected by default to position 0 of the HPS-FPGA bridge (starts at 0xC0000000). The memory can be changed easyly to hang from the Lightweight HPS-FPGA bridge using Qsys.
* Data_size is 128-bit by default. When hanging from the HPS-FPGA bridge, the size of the bridge and/or this memory can be changed to 64-bit or 32-bit. When hanging from the Lightweight HPS-FPGA bridge the size should be 32-bit.

Each transfer is repeated by default 100 times and the following statistics are calculated for each combination of the aforementioned parameters:
* mean value
* minimum value
* maximum value
* variance

Results are printed in console.

Description of the code
-----------------------

#### configuration.h:
configuration.h permits to control the default behaviour of the program:
* Selecting between   ON_CHIP_RAM_ON_LIGHTWEIGHT,  ON_CHIP_RAM_ON_HFBRIDGE32, ON_CHIP_RAM_ON_HFBRIDGE64 and ON_CHIP_RAM_ON_HFB the program is automatically adapted depending on the hardware project used. By default it is supossed that the FPGA OCR is connected to the HPS-to-FPGA (non Lightweight) bridge with 128-bit width. configuration.h obtains hardware information for each bridge and size from [code/inc/FPGA_system_headers](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/FPGA_system_headers).
* Cache features that are activated can be controlled by the CACHE_CONFIG macro that can be defined as a number between 0 and 13 with the following meanings. Each number adds a feature to the previous state so adding the effect of each feature can be easily studied:
	* 0 no cache no MMU
	Basic config and optimizations:
	* 1 enable MMU
	* 2 do 1 and initialize L2C
	* 3 do 2 and enable SCU
	* 4 do 3 and enable L1_I
	* 5 do 4 and enable branch prediction
	* 6 do 5 and enable L1_D
	* 7 do 6 and enable L1 D side prefetch
	* 8 do 7 and enable L2C
	Special L2C-310 controller + Cortex A9 optimizations:
	* 9 do 8 and enable L2 prefetch hint
	* 10 do 9 and enable write full line zeros
	* 11 do 10 and enable speculative linefills of L2 cache
	* 12 do 11 and enable early BRESP
	* 13 do 12 and store_buffer_limitation

* By default only memcpy() tests are done. Uncommenting the macro UP_FORLOOP the for loop experiments can be performed after the memcpy() ones. The for_loop is very much slow than the memcpy() one and it is rarely needed. Thats why by default it is not performed.

#### main.c:
First the program generates the address to access FPGA-OCR. The address used is the beginning of the bridge where the FPGA-OCR is connected plus the address of the memory with respect to the beginning of the bridge (address in Qsys). Using this address the FPGA OCR is checked to ensure that the processor has access to all of it. If access is possible the FPGA OCR content is cleared (0s are written). The HPS-OCR (On-Chip RAM in HPS) is also checked but it is not used in the rest of the program (this was used during debugging).

After that cache is switched on with the level of optimization specified by the macro CACHE_CONFIG of configuration.h. The function used to switch on the cache, cache_configuration() is in the files cache_high_level_API.h and cache_high_level_API.c in folder [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache). This function calls to the low level functions defined in files arm_cache_modified.h and arm_cache_modified.c in the same folder. This files are a modification to the [Legup files to configure Cyclone V SoC cache](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches). The modification, explained at the beginning of arm_cache_modified.c consists in changing main memory table attributes in MMU to shareable memory so coherent ACP access are permited. After switching on the cache the ACP is configured to allow access to cached memory through ACP by the DMA controller. The ACP ID mapper is configured using acp_configuration() defined in acp_high_level_API.h and acp_high_level_API.c, located in [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache) too. This function sets a AXI ID for accessing L2 cache controller equal to 3 to any master reading the ACP and equal to 4 to any master reading the cache.

Then the PMU is initialized as a timer. PMU is the more precise way to measure time in Cyclone V SoC. Visit this [basic example](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) to learn more about it. It is initialized with a base frequency of 800 MHz (the frequency of the CPU configured in the FPGA hardware project) and freq divider = 1 so the measurement is most precise possible (1.25ns resolution). After initialization empty measuremnts (measuring no code) are done to obtain the PMU measurement overhead (30ns). The measurement overhead will be substracted from the tranfer time measurements to improve precision.

For each combination of data sizes, 100 measurements on writing the FPGA (WR) and reading the FPGA (RD) are done. In the case the FPGA OCR size (256KB) is smaller than transfer size (2B to 2MB) transfer are repeated until the total desired transfer size is completed (address of main memory keeps growing to see cache effects while address in FPGA is reset). When the 100 measuremnts are done, mean, max, min and variance is calculated and printed in screen or file. First measuremnts with memcpy() function are done. memcpy() is a function from standard library to efficiently copy data between parts of the system using CPU. If UP_FORLOOP is uncommented in configuration.h then the measurements are repeated using the *for loop* method.For loop is created and data is copied word by word (one word per loop is copied). For loop resulted much slower than memcpy() so always use memcpy() in your applications.

The file [code/inc/Baremetal_io/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Baremetal_IO)io.c gives support to printf function.

Contents in the folder
----------------------

* configuration.h
* main.c
* cycloneV-dk-ram-modified.ld: describes the memory regions (stack, heap, etc.).
* Makefile: describes compilation process.

Compilation
-----------
Open SoC EDS Command Shell, navigate to the folder of the example and type **make**.
The provided makefile is prepared to automatically detect and adapt to both the old hwlib folder structure (before *Intel FPGA SoC EDS v15*) and the new (*Intel FPGA SoC EDS v15* and ahead) so the included files are found.

This compilation process was tested with both *Altera SoC EDS v14.1* and *Intel FPGA SoC EDS v16.1*.

The compilation process generates two files:
* baremetalapp.bin: to load the baremetal program from u-boot
* baremetalapp.bin.img: to load the baremetal program from preloader

How to test
-----------
In the following folder there is an example on how to run baremetal examples available in this repository:
[https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-baremetal](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-baremetal).
