Baremetal/FPGA-to-HPS_FPGADMAC
=================================

Introduction
-------------
This baremetal application measures the data transfer rate between FPGA and HPS using Direct Memory Access Controllers (DMACs) in the FPGA. The DMAC used is the (non scatter-gather) DMAC available in Platform Designer (formely Qsys). One DMAC is connected per available port: one to the FPGA-to-HPS bridge and two (-128-bit) or four (32- and 64-bit) to the FPGA-to-SDRAM ports, depending on the bridge size. The hardware project used is the [FPGA_DMA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_DMA). To control the DMACs a custom API, located in code/inc/FPGA_DMA, has been developed.

The program carries out transfers between On-Chip RAM memories in the FPGA (FPGA-OCRs) and HPS memories (cache through ACP, SDRAM trhough L3->SDRAMC port or SDRAM directly through FPGA-to-SDRAM ports) and measures the transfer time using the Performance Monitoring Unit.

When executing the program different combinations of FPGA DMA Controllers transferring data are tested:
* Only one DMAC connected to the FPGA-to-SDRAM port transfers data.
* All DMACs connected to the FPGA-to-SDRAM ports (two or four DMACs depending on bridge size) are used to transfer data.
* Only the DMAC attached to the FPGA-to-HPS port using the ACP port.
* Only the DMAC attached to the FPGA-to-HPS port using the L3->SDRAM port.
* All DMACs, the one connected to the FPGA-to-HPS port using the L3-SDRAM port and all DMACs connected to the FPGA-to-SDRAM ports.

The program automatically tests different data sizes, from data size 2B to 2MB, in 2x steps. When more than one DMAC is
used the work is equally distributed among the DMACs. To do this the data payload moved by each DMAC is calculated as the target data size (to be moved from FPGA-to-HPS) divided by the number of DMACs. This permits to study if distributing the payload of certain transfer can be speed up using more than one DMAC.

Each transfer is repeated by default 100 times and the following statistics are calculated for ea:
* mean value
* minimum value
* maximum value
* variance

Results are printed in console.

Description of the code
------------------------
#### configuration.h
This file permits to configure the experiments:

The following macros must be used to configure the software depending on the size of the data bus of the FPGA-HPS bridges used:
* BRIDGES_32BIT: uncomment when 32-bit bridges are used.
* BRIDGES_64BIT: uncomment when 64-bit bridges are used.
* BRIDGES_128BIT: uncomment when 128-bit bridges are used.

Since the DMAC ports are unidirectional only one direction is tested with one execution of the program. To choose the data direction:
* WR: read data from FPGA-OCRs and write to processor memories. Uncomment WR_HPS in configuration.h to select WR.
* RD: read data from processor memories and write to FPGA-OCRs. Uncomment RD_HPS in configuration.h to select RD.

Notice that in this case data in WR and RD operations travels in the opposite direction than the WR and RD operations defined for HPS-to-FPGA experiments.

Not all combinations must always be done. Using the following macros certain combinations can be disabled:
* TEST_ALL_COMBINATIONS: uncomment to test all combinations.
* TEST_ONLY_F2S_BRIDGES: uncomment to test only the combinations that do not use the FPGA-to-HPS bridge.
* TEST_F2H_AND_ALL_BRIDGES: uncomment to test only the combinations that use the FPGA-to-HPS bridge.

The macro CACHE_CONFIG configures the cache level. Its options have been already explained in HPS-to-FPGA baremetal programs.

#### main.c:

First the program prepares some variables to do the experiments. The most important is data_size_dmac, that contains the data transfer of the DMACs depending on the number of DMACs used. data_size_dmac[i][j] is the data transfer when using i+1 DMACs for the data size data_size[j].

Then the program generates the address of the DMACs and FPGA-OCR memories in the FPGA to have acess to it from the processor. The address used is the beginning of the bridge where the component is connected (HPS-to-FPGA bridge) plus the address of the memory with respect to the beginning of the bridge (address in Qsys). After that the FPGA-OCR memories are checked (to test that all Bytes are accesible) and reset.

After that cache is switched on with the level of optimization specified by the macro CACHE_CONFIG of configuration.h. The function used to switch on the cache, cache_configuration() is in the files cache_high_level_API.h and cache_high_level_API.c in folder [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache). This function calls to the low level functions defined in files arm_cache_modified.h and arm_cache_modified.c in the same folder. This files are a modification to the [Legup files to configure Cyclone V SoC cache](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches). The modification, explained at the beginning of arm_cache_modified.c consists in changing main memory table attributes in MMU to shareable memory so coherent ACP access are permited. After switching on the cache the ACP is configured to allow access to cached memory through ACP by the DMAC. The ACP ID mapper is configured using acp_configuration() defined in acp_high_level_API.h and acp_high_level_API.c, located in [code/inc/Cache/](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache) too. This function sets a AXI ID for accessing L2 cache controller equal to 3 to any master reading the ACP and equal to 4 to any master reading the cache. Then the AXI signals in the Avalon to AXI component (needed for the DMAC connected to the FPGA-to-HPS bridge) are set to 0x00870087 (fastest) and the FPGA-to-SDRAM ports are removed from reset so they are usable from FPGA.

Then the PMU is initialized as a timer using functions defined in [code/inc/PMU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/PMU). PMU is the more precise way to measure time in Cyclone V SoC. Visit this [PMU basic baremetal example](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) to learn more about it. It is initialized with a base frequency of 800 MHz (the frequency of the CPU configured in the FPGA hardware project) and freq divider = 1 so the measurement is most precise possible (1.25ns resolution). After initialization empty measuremnts (start the counter and inmediately read it) are done to obtain the PMU measurement overhead. This overhead will be substracted to all measuremnts later on to obtain a more precise measurement.

Later the DMACs are initialized. The transfer word (size of one transaction) is
set equal to the bridge size to maximize speed. The transfer is considered to be finished when the length of the internal counter of the DMAC reaches zero (all data is sent). Lastly the DMAC is configured to use a constant read address during WR operations or constant write address during RD operations. That means that the DMAC always uses the same address to access the FPGA-OCR from the 1kB available. This allows the program to test big data sizes above 512kB, the maximum size that can be configured in the DE1-SoC. Moreover, having a smaller FPGA-OCR permits to reduce the interconnect and increase the maximum FPGA operating frequency during the experiments. Tests have been done to verify that using constant adresseses for the FPGA-OCRs does not affect the transfer data rate, so the experiments carried out in this way are equivalent to ones carried out with a non constant address (normal way to proceed).

Now measurements start. The different combinations of DMACs are prepared and defined using a for loop and switch case. They define the DMACs that will be used and the data size of the experiments for every combination loop. Then the destiny address is cleared and the source filled with a constant. After that the transfer is configured from source to destiny. The PMU counter is reset and the transfer/s is/are started for the DMAC/s active in the current combination. When all DMACs finish the value of the PMU counter is read and this is taken as transfer time. Each transfer is repeated by default 100 times and the following statistics are calculated for each data size and combination of DMACs:
* mean value
* minimum value
* maximum value
* variance

Results are printed in console.

Contents in the folder
----------------------

* configuration.h
* main.c
* alt_address_space.c and alt_address_space.h. These functions are copied without modifications from hwlib. They contain the functions to configure the ACP.
* cycloneV-dk-ram-modified.ld: describes the memory regions (stack, heap, etc.).
* Makefile: describes compilation process.

Compilation
-----------
Open SoC EDS Command Shell, navigate to the folder of the example and type **make**.
The provided makefile is prepared to automatically detect and adapt to both the old hwlib folder structure (before *Intel FPGA SoC EDS v15*) and the new (*Intel FPGA SoC EDS v15* and ahead) so the included files are found.

This compilation process was tested in *Intel FPGA SoC EDS v16.1*.

The compilation process generates two files:
* baremetalapp.bin: to load the baremetal program from u-boot
* baremetalapp.bin.img: to load the baremetal program from preloader

How to test
-----------
In the following folder there is an example on how to run baremetal examples available in this repository:
[https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-baremetal](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-baremetal).
Remember to load in the SD card the .rbf file of the FPGA project equivalent with the selections in configuration.h.
