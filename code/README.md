CycloneVSoC-time-measurements/code
==================================

Content in the code folder:
* Angstrom_OS: Programs used to perform processor-FPGA tranfer rate measuremnts with Angstrom OS. They could also be used for other Linux-based OSs.
* Baremetal: Programs used to perform processor-FPGA tranfer rate measuremnts in baremetal (no OS).
* inc: folder with common files used by more than one program.
	* Baremetal_IO: contains io.c. This file gives support to printf function in baremetal programs.
	* Cache: Cache-related functions for Baremetal:
		* arm_cache_modified.h and arm_cache_modified.c: low level functions to control cache. They are a modification of [Legup cache functions](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches) to allow ACP coherent accesses to processor memory.
		* cache_high_level_API.h and cache_high_level_API.c: define cache_configuration() fuction that permits easy configuration of cache forgetting low-level details. Internally it uses arm_cache_modified.h and arm_cache_modified.c. This file also defines L2_lockdown_by_master() function that permits control of the lockdown by master feature in the L2 cache controller.
		* acp_high_level_API.h and acp_high_level_API.c: Defines acp_configuration() function. This configures the ACP ID mapper to dinamically translate the AXI ID from any ACP incoming master into ID 3 in reads and ID 4 for writes for the L2Controller. Therefore the L2C sees: ID 0 when CPU0 accesses, ID 1 when CPU1 accesses, ID 3 when any master in ACP reads, ID 4 when any master writes in ACP.
	* FPGA_DMA: it contains an API to control the (non scatter-gather) DMA Controller for FPGA available in Platform Designer (Qsys).
	* FPGA_system_headers: It contains header files with the base addresses and the spam of the FPGA peripherals of a system developed with Platform Designer. They should be used to access them from software to these peripherals. They have been generated using the sopc-create-header-files applied in the different [hardware projects](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware):
		* hps_0_HPS-FPGA-X.h and hps_0_LW.h are generated from [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K). They are used in the HPS-to-FPGA programs (those using HPS as master).
		* hps_0_FPGA-HPS.h is generated from [FPGA_DMA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_DMA).
 These files are used the Baremetal and Angstrom OS programs.
	* PMU: contains the files pmu.c and pmu.h that permit control of Performance Monitoring Unit (PMU) as timer. Used to measure time in both the Baremetal and Angstrom OS programs.
	* SDRAMC: defines the sdramc.h and sdramc.c files that define functions to control port priority and weight in the SDRAM Controller. Used by the baremetal programs.
	* Statistics: functions to calculate some statistics, namely mean, min, max and variance. Used by both the Baremetal and Angstrom OS programs.

As an exception, the Angstrom OS programs make use of code not included in this repository. One exception is a driver to perform DMA transfers to FPGA from OS used in the [Angstrom HPS-to-FPGA with DMA program](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_PL330DMAC). It is located at our repository examples because it is generic code (and not transfer rate measurement code): [https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM).
Other exception is a driver used to allocate physically contiguous buffers, needed for the [Angstrom program that transfers data using DMAC in the FPGA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/FPGA-to-HPS_FPGADMAC). This driver is also located in the examples repository at: [https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM).
