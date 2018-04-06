Baremetal/HPS-to-FPGA_PL330DMAC
=================================

Introduction
-------------
This baremetal application measures the data transfer rate between HPS and FPGA using the Direct Memory Access Controller (DMAC) PL330 available in HPS to do the transfers and Performance Monitoring Unit (PMU) to measure time.

The program controls the DMA using a modified version of DMA functions available in hwlib.

The following parameters affecting the transfer speed are studied:
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory).
* Transfer size: From 2B to 2MB in 2x steps.
* Coherency: DMAC accesses HPS memories through ACP (coherent access) when the cache memory is ON and through the L3-to-SDRAMC port when cache is off.
* DMA initialization time: tests are repeated with or without including the preparation of the DMAC microcode. DMAC microcode can be generated beforehand for lots of applications reducing transfer time.

Transfers are performed from processor memories (cache/SDRAM) and an On-Chip RAM (OCR) in the FPGA. The FPGA hardware project used is available in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K).
The memory in the FPGA has the following characteristics:
* Implemented using embedded 10kB memory blocks.
* Size = 256kB, the maximum power of two feasible in DE1-SoC board.
* Connected by default to position 0 of the HPS-FPGA bridge (starts at 0xC0000000). The memory can be changed easly to hang from the Lightweight HPS-FPGA bridge using Qsys.
* Data_size is 128-bit by default. When hanging from the HPS-FPGA bridge, the size of the bridge and/or this memory can be changed to 64-bit or 32-bit. When hanging from the Lightweight HPS-FPGA bridge the size should be 32-bit.

Each transfer is repeated by default 100 times and the following statistics are calculated for each combination of the aforementioned parameters:
* mean value
* minimum value
* maximum value
* variance

Results are printed in console.

Description of the code
------------------------

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
* Uncommenting EN_LOCKDOWN_STUDY the lockdown by master study can be activated. Regular measurements are repeated changing lockdown by master settings of the L2 8-way associative cache controller. Regular configuration; configurations locking 1, 4 and 7 ways for CPU; and ways locking 1, 4 and 7 ways for CPU and the rest for ACP are tested. When EN_LOCKDOWN_STUDY is activated, uncommenting LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA the CPU and ACP is locked after the transfer data is generated and located in cache without locking. Otherwise the CPU/ACP are locked before data is generated. This study tries to measure the improvement that can be achieved on DMA transfers using ACP when applying lockdown by master.
* Uncommenting EN_SDRAMC_STUDY the SDRAM port priority and weight is activated. Regular measurements are repeated changing settings of the mmpriority, mpweight_0_4 and mpweight_1_4 (see Cyclone V SoC Handbook for more information on these registers). The following combinations are performed: default configuration when starting up the syste; give to CPUs port and L3-to-SDRAM controller port same priority (using mmpriority) giving to L3-to-SDRAM controller port 2, 4, 8 and 16 times more bandwidth (using mpweight_0_4 and mpweight_1_4); give to  L3-to-SDRAM controller port more priority than CPUs port so it is always accessed when both ports access simultaneously to data. This study tries to measure the improvement that can be achieved on DMA transfers using L3-to-SDRAM port when giving more bandwidth or priority to it.
* Uncommenting GENERATE_DUMMY_TRAFFIC_IN_CACHE traffic can be added to chache and main SDRAM memory to affect the DMA transfers and see more clearly the effects of lockdown by master and sdram controller port priority and bandwidth when EN_LOCKDOWN_STUDY or EN_SDRAMC_STUDY are enabled, respectively. To generate the traffic, in the while loop where the application waits for the DMA controller to finish the transfer, reads and writes to/from memory take place in a span of 2MB.

#### main.c:

First the program generates the address to access FPGA-OCR. The address used is the beginning of the bridge where the FPGA-OCR is connected plus the address of the memory with respect to the beginning of the bridge (address in Qsys). Using this address the FPGA OCR is checked to ensure that the processor has access to all of it. If access is possible the FPGA OCR content is cleared (0s are written).

After that cache is switched on with the level of optimization specified by the macro CACHE_CONFIG of configuration.h. The function used to switch on the cache, cache_configuration() is in the files cache_high_level_API.h and cache_high_level_API.c in folder [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache). This function calls to the low level functions defined in files arm_cache_modified.h and arm_cache_modified.c in the same folder. This files are a modification to the [Legup files to configure Cyclone V SoC cache](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches). The modification, explained at the beginning of arm_cache_modified.c consists in changing main memory table attributes in MMU to shareable memory so coherent ACP access are permited. After switching on the cache the ACP is configured to allow access to cached memory through ACP by the DMA controller. The ACP ID mapper is configured using acp_configuration() defined in acp_high_level_API.h and acp_high_level_API.c, located in [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache) too. This function sets a AXI ID for accessing L2 cache controller equal to 3 to any master reading the ACP and equal to 4 to any master reading the cache.

Then the PMU is initialized as a timer using functions defined in [code/inc/PMU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/PMU). PMU is the more precise way to measure time in Cyclone V SoC. Visit this [PMU basic baremetal example](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) to learn more about it. It is initialized with a base frequency of 800 MHz (the frequency of the CPU configured in the FPGA hardware project) and freq divider = 1 so the measurement is most precise possible (1ns resolution). After initialization empty measuremnts (measuring no code) are done to obtain the PMU measurement overhead. This overhead will be substracted to all measuremnts later on to obtain a more precise measurement.

To do the measurment the functions in alt_dma_modified.c and alt_dma_modified.h are used. These files are a modification of alt_dma.c and alt_dma.h available in hwlib. The modification consitsts in splitting alt_dma_memory_to_memory(), a function used to move data using DMA. From alt_dma_memory_to_memory() code, alt_dma_memory_to_memory_only_prepare_program() and alt_dma_channel_exec() are created. alt_dma_memory_to_memory_only_prepare_program() only prepares the DMA microcode that DMA cointroller will execute to do the transfer. alt_dma_channel_exec() just executes the microcode in DMA controller and performs the actual transfer. This is very useful because preparing the microcode is between 20% and 80% of the execution time of alt_dma_memory_to_memory(). In most applications the source and destiny buffers, along with the size of the transfer is know, so the microcode can be prepared at the beginning of the program only once, instead of being regenrated every time a transfer is done.

Then the measurements start. For each combination of data sizes, 100 measurements on writing the FPGA (WR) and reading the FPGA (RD) are done. In the case the FPGA OCR size (256KB) is smaller than transfer size (2B to 2MB) transfer are repeated until the total desired transfer size is completed (address of main memory keeps growing to see cache effects while address in FPGA is reset). When the 100 measuremnts are done, mean, max, min and variance is calculated using functions in [code/inc/Statistics](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Statistics)/statistics.c and printed in screen. If GENERATE_DUMMY_TRAFFIC_IN_CACHE is defined the processor goes accessing memory doing reads and writes in a span of 2MB to pollute cache and SDRAM controller with traffic while the program whites for the transfer to end.
To do the transfer the program first uses the original hwlib function alt_dma_memory_to_memory() so microcode preparation time + actual transfer are measured. Later all measurements are repated using alt_dma_memory_to_memory_only_prepare_program() at the beginning to prpare the microcode for DMA controller and alt_dma_channel_exec() to do the transfer. The second method gives fater transfer rates because the preparation time is not measured.

If EN_LOCKDOWN_STUDY is defined measurements are repeated for all combinations of lockdown by master programmed. Lockdown is controlled by function L2_lockdown_by_master() defined in [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache)cache_high_level_API.c.
If EN_SDRAMC_STUDY is defined measurements are repeated for all combination of port priority and bandwidth programmed. Controlled by function print_sdramc_priorities_weights()defined in [code/inc/SDRAMC/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/SDRAMC)sdram.c.

The file [code/inc/Baremetal_io/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Baremetal_IO)io.c gives support to printf function.



Contents in the folder
----------------------

* configuration.h
* main.c
* alt_dma_modified.c  and alt_dma_modified.h describe the DMAC control functions. The original alt_dma.c from hwlib was modified. The changes are:
    * ALT_DMA_CCR_OPT_SC_DEFAULT was changed by ALT_DMA_RC_ON = 0x00003800 in alt_dma.c. ALT_DMA_CCR_OPT_DC_DEFAULT was changed by ALT_DMA_WC_ON =  0x0E000000. These changes make the channel 0 of the DMAC to do cacheable access with its AXI master port.
    * Other change is the split of alt_dma_memory_to_memory() into 2 functions: alt_dma_memory_to_memory_only_prepare_program() and alt_dma_channel_exec(), as explained above.
* alt_dma_program.c, alt_dma_program.h, alt_address_space.c and alt_address_space.h and alt_types.h. Copied exactly from the hwlib. They were copied to be able to compile in old (lover than v.15) and newer (v.15 and higher) SoC EDS hwlib folder structure, because the .h in the new style ask for the #define soc_cv_av.

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
