CycloneVSoC-time-measurements
=============================
```diff
- Message to reviewers: This repository is under construction. It will be totally finished on Friday Sepember 22th.
```
Programmable Systems-on-Chip (FPSoCs) are heterogeneous reconfigurable platforms consisting of hard processors and FPGA fabric. Authors have been working with FPSoCs for a long time, and have thoroughly searched many times information regarding the best way to inteconnect processor and FPGA. As a matter of fact, there is little information available on the topic, and even that very little is divided in pieces posted on different, unrelated websites. This repository contains our experiments regarding the processor-FPGA transfer rates in Cyclone V SoC devices, a very important family of FPSoCs. It also serves as support to the paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"*. Since data and figures were too many to fit in a single article all data is provided in this repository. The code used for the experiments is also provided.

Feel free to use and share all contents in this repository. Please, remember to reference our paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"* if you make any publication that uses our work. 
The repository license is GPL v3.0.



Table of contents of this README file:

1. [Repository Contents](#1---repository-contents)
2. [Cyclone V SoC Overview](#2---cyclone-v-soc-overview)
3. [Setting up Cache in Baremetal](#3---setting-up-cache-in-baremetal)
4. [HPS-to-FPGA - Main Experiments](#4---hps-to-fpga---main-experiments)
5. [HPS-to-FPGA - L2 Lockdown by Master Experiments](#5---hps-to-fpga---l2-lockdown-by-master-experiments)
6. [HPS-to-FPGA - SDRAM Controller Experiments](#6---hps-to-fpga---sdram-controller-experiments)


1 - Repository contents
-----------------------
The repository is organized as follows:
* **This README** file contains explanation of the experiments. For each experiment a summary of the results and the more important plots are provided. The reader can also find links to the code used for the experiment and to the full set of numeric results.
* **code/**: contains all programs used to do the experiments. A README file explaining the code and how to compile and test each program is provided inside their corresponding folders.
* **fpga-hardware**: Quartus/Qsys hardware projects for the FPGA, used to perform the experiments.
* **results/**: contains all numeric results for the experiments.
* **figures/**: contains the figures used in the README files in this repository.


To make use of this repository download it as a zip or install git in your computer and download it using:
```bash
# navigate to the folder where you want to download the repository
git clone https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements.git
```


2 - Cyclone V SoC Overview
--------------------------
A simplified block diagram of Cyclone V SoC is depicted in the figure below. Cyclone V SoC architecture consists of a Hard Processor System (HPS) and FPGA fabric in a single chip, coupled by high throughput datapaths.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/CycloneVSoC.png" width="500" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

The HPS features a microprocessor unit (MPU), on-chip RAM (HPS-OCR), booting ROM, SDRAM controller (SDRAMC), DMA controller (DMAC), and HPS peripherals (such as Ethernet and USB controllers, UARTs, and timers). The MPU includes two ARM Cortex-A9 32-bit processors, with 32kB L1 data cache and 32kB L1 instruction cache each and 512kB L2 shared instruction and data cache. A Snoop Control Unit (SCU) ensures coherency among L1 caches, L2 cache, and SDRAM contents.

L3 is a partially connected crossbar switch interconnecting all elements in the HPS. The Accelerator Coherency Port (ACP) allows masters accessing L3 to perform coherent requests to the cache memories. All interconnection elements in HPS are AXI3-compliant.

Interconnection resources between the HPS and the FPGA fabric can be divided in two groups:
1)	Those through which HPS accesses FPGA as master:
* The HPS-to-FPGA (HF) bridge, a high-performance bus whose data width is configurable as 32-, 64- or 128-bit (referred to as HF32, HF64 and HF128, respectively, in the remainder of this README.
* The Lightweight (LW) HPS-to-FPGA bridge, a 32-bit bus connected to the L3 Slave Peripheral Switch (referred to as LW32 in the remainder of the paper). Its performance is lower than that of the previous bridge. It is aimed at accessing the configuration registers of peripherals implemented in the FPGA fabric.

2)	Those through which FPGA accesses HPS as master:
* The FPGA-to-HPS (FH) bridge, a high-performance bus enabling FPGA masters to access HPS peripherals, DMAC, and SDRAMC, as well as cache memories through ACP (coherent access).
* FPGA SDRAM configurable master ports, which access the external SDRAM memory in a non-coherent way directly through SDRAMC.


3 - Setting up Cache in Baremetal
---------------------------------
### Introduction
To perform the experiments we used Angstrom Operating System and Baremetal (no OS) implementations. When using OS it automatically setups the cache for us. However, when using Baremetal, regardless of booting with preloader or u-boot (see [SD-baremetal](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-baremetal) to learn more about baremetal booting) the caches start up disabled. Luckily, [Legup](http://legup.eecg.utoronto.ca/) provide [functions to switch on MMU, L1 and L2 caches, and apply most of the available optimizations](http://legup.eecg.utoronto.ca/wiki/doku.php?id=using_arm_caches) in arm_cache.h and arm_cache.s files. Moreover they tested the performance of each optimization in some benchmark programs. We used Legup-up files and applied the following change that was needed for our purpose of moving data between processor and FPGA:

* @L1_NORMAL_111_11 constant was changed from its original value 0x00007c0e to 0x00017c0e. This change sets S bit in table descriptors definyng normal memory region attributes, making normal memory shareable (coherent). This change allows coherent accesses through Aceleration Coherency Port (ACP).

This produced the [code/inc/Cache](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache)/arm_cache_modified.h and [code/inc/Cache](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache)/arm_cache_modified.s. We used the modified versions to setup cache for our baremetal project. To test the Legup functions and see the effect of each cache and optimization in the performance of the program we created a function called cache_configuration() in files [code/inc/Cache](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache)/cache_high_level_API.h and [code/inc/Cache](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/Cache)/cache_high_level_API.c. This function uses the low level functions in arm_cache_modified and permits to configure cache in a single call. It also avoids errors regarding the order for calling low-level functions. The function parameter to define the cache setup is a single integer between 0 and 13 with the following meanings:

	0: no cache no MMU
    
	Basic config and optimizations:
	1: enable MMU
	2: do 1 and initialize L2C
	3: do 2 and enable SCU
	4: do 3 and enable L1_I
	5: do 4 and enable branch prediction
	6: do 5 and enable L1_D
	7: do 6 and enable L1 D side prefetch
	8: do 7 and enable L2C
    
	Special L2C-310 controller + Cortex A9 optimizations:
	9: do 8 and enable L2 prefetch hint
	10: do 9 and enable write full line zeros
	11: do 10 and enable speculative linefills of L2 cache
	12: do 11 and enable early BRESP
	13: do 12 and store_buffer_limitation 

Since each level is equal to the previous just adding a single feature, the effect of adding each feature can be easily  identified.

### Explanation of the Experiments
Using the [code/Baremetal/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_CPU) program we measured the transfer time between the CPU and FPGA for the different parameters:
* Method: memcpy() function is used to perform the transfer.
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory). The program automatically does that.
* Transfer size: From 2B to 2MB in 2x steps. The program automatically does that.
* Cache level: levels 0 to 13 of the cache_configuration(). For each level the macro CACHE_CONFIG is set to one corresponding level and the program recompiled again. Therefore a total of 14 different programs are generated with different cache levels.

Tests are repeated 100 times (automatically done by the application) and mean value is given as result.

After running the 14 programs results are saved into a file and plotted.

### Analysis of the Results
All numeric results from this experiments are in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_baremetal_cache.xlsx. The figure below depicts the mean transfer speed (in MB/s) of data sizes between 32B and 2MB (small data sizes are discarded because their transfer speed is very different to the higher ones and distorted the plots).

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Cache-baremetal.png" width="600" align="middle" alt="Main-results" />
</p>

Since every steps adds a single feature the effect of each feature can be identified:
* Switching on MMU (cache configuration 1) has big effect on transfer speed.
* Switching on Instruction cache L1 (cache configuration 4) has small effect. Speed is gained because instructions are retrieved faster from memory.
* Switching on Data cache L1 (cache configuration 6) has a big effect on trasfer. Thats normal because cache allows transfer data to be retrieved or saved by the CPU faster.
* Switching on the L2 controller (Shared data and instruction L2 cache)(cache configuration 8) has also very big effect.

The rest of the features does not provide a clear advantage. Legup tests found out that Special L2C-310 controller + Cortex A9 optimizations (cache configurations from 9 to 13) provide no advantage for their processing benchmarks. We have found the same conclusions for our data transfer programs.

We used cache configuration 9 in our Baremetal programs.


4 - HPS-to-FPGA - Main Experiments
----------------------------------
### Introduction
These experiments represent the core of HPS-FPGA transfer rate measurements when using the HPS as master to move data. They provide a good overview of the device behaviour. Experiments are performed using Angstrom Operating System and Baremetal. Baremetal measurements provide the fastest data rates that can be achieved in the device. These data can be compared with transfer speed achieved when using OS to perceive its effect on transfers. All design parameters that have been identified to influence transfer rate are considered, with the exception of processor workload, which is kept to a minimum. The reason for this is that it is not practically feasible to consider the many different combination of tasks that the processors may be executing in real applications. Taking into account just a subset of those would result in partial and likely misleading conclusions, difficult (if at all possible) to be extrapolated to specific applications. Therefore, the results of the experiments presented in next section represent the optimal performance under each tested scenario, which is reduced as processor workload increases.

### Explanation of the Experiments
Transfer rates between HPS and FPGA when HPS is the master are measured for different combinations of values of the following parameters:

* OS or baremetal implementation:
	* Angstrom, the default Linux-based OS for Intel FPGA devices and well documented in their [support website](https://rocketboards.org/foswiki/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) has been used. It is a light OS, well suited for industrial applications.
	* Baremetal application running in one of the ARM cores.
* Master starting AXI bus transfers:
	* CPU: one of the ARM cores controls data transfer. The program [code/Baremetal/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_CPU) is used for baremetal and [code/Angstrom_OS/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_CPU) for Angstrom OS.
	* DMAC: DMAC executes data transfer microcode in HPSOCR, freeing the processor from this task. The program [code/Baremetal/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_PL330DMAC) is used for baremetal and [code/Angstrom_OS/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_PL330DMAC) for Angstrom OS.
* Bridge. All bridges having HPS as master have been tested: LW32, HF32, HP64 and HP128. The hardware project used is [fpga-hardware/DE1-SoC/FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K). Compiled files for the different bridges are availabe at [fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf).
* Cache enablement:
	* Caches off. All memory accesses are to external SDRAM. Caches were not switched off for Angstrom since it does not make much sense. Doing it in baremetal is enough to measure the effect of caches in the execution of transfer rate programs.  To switch ON or OFF the caches set to 9 or 0 the macro CACHE_CONFIG in the configuration.h file of each program.
	* Caches on. Accesses are to L1, L2, or external SDRAM, depending on where data are located and the port used.
* Coherency (only applies for DMAC): DMAC can access HPS memories through ACP (coherent access) or directly access external SDRAM through the SDRAMC port from L3 (non-coherent access). In the baremetal tests coherency is automatically changed depending on cache enablement. When cache is OFF non-coherent access is applied (coherent access will be slower because the access would be L3->L2->SDRAM) so it is better to directly do non coherent L3->SDRAM). When cache is ON the access is coherent through ACP. Tests with cache ON and non-coherent access where not performed because they are very similar to those non-coherent with cache OFF. In fact cache ON and non-coherent access will be little faster than non coherent with cache OFF because the few operations done by the CPU will execute faster. We say in plots that non-coherent access with cache off and on are the same. This is conservative and all conclusions derived from plots are therefore valid. In the case of OS the program repeats experiments in coherent and non-coherent way.
* Preparation of DMA microcode program (only applies for DMAC). In most applications the size of the transfer and the address of the destiny and source buffers is known before the transfer takes place so the DMAC microcode programs can be prepared beforehand (during board start-up and initializations) and its preparation time can be saved when the program is actually running the application-ralated tasks. For both, baremetal and OS the transfer time is measured with and without DMAC microcode preparation time.
* Data size from 2B to 2MB, in 2x steps. This automatically done by the program that repeats the measurements for every data size.
* FPGA frequency from 50 to 200MHz, in 50MHz steps. For tests failing at 200MHz, the frequency was reduced in 20MHz steps until correct operation was achieved. Compiled files for the different FPGA frequencies are availabe at [fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf).
* Two DE1-SoC boards have been tested.

The FPGA-OCR data bus size is always the same as the bridge where it is connected.

Tests are repeated 100 times (automatically done by the application) and mean value is given as result.


### General Analysis of the Results
The full set of numeric values for the main experiments is in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_main_time_measurements.xlsx.

The effect of some parameters, namely FPGA frequency, cache enablement and bridge type, is independent of the implementation (OS or baremetal) and the data size. The fastest data transfers are always obtained for the HF128 bridge with caches on. In contrast, results for different coherency (DMAC access through ACP or SDRAMC) or AXI masters (CPU and DMAC) depend on implementation and data size. The maximum frequency all experiments run successfully is 150MHz. As commented later in frequency analysis some of them run at higher frequencies. The following figure shows the most important results: 

<p style="text-align: center;"> **Transfer rate (in MB/s) of experiments through HF128 bridge with FPGA frequency 150MHz**</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/HF128-150MHz.png" width="800" align="middle" alt="Main-results" />
</p>

For all plots at small data size transfer rate grows with data size because the initialization time (calling the memcpy() function in CPU plus memcpy() initialization tasks or preparing program in DMA in case of using the DMAC) becomes less compared to the transfer time. Initialization time grows as data transfer grows but much slowly than the time where data is actually being transferred. For intermediate data sizes the transfer rate tends to stabilize because initialization time becomes negligible when compared to the actual transfer. When caches are used, performance decreases for sizes above 32kB (that of L1 caches) and again above 512kB (the size of L2 cache) because of cache misses. After cache effects happen (for bigger data size than 512kB) the transfer rate stabilizes and for data size bigger than 2MB the transfer rate at 2MB is expected. Exceptions are plots 9 and 13 that will decrease a little bit after 2MB before they stabilize.

As explained before, cache enablement has a negligible effect in DMA transfers through SDRAMC in baremetal implementations. This can be noticed in the figure, where plots 10 and 14 represent DMA WRand RD operations, respectively, through SDRAMC with both caches off and on.

### Bridge Type Analysis
The following figures permit to graphically visualize the difference in performance between all bridges connecting HPS and FPGA where the HPS acts as master: High performance configured as 128-, 64- or 32-bit and the Lightweight 32-bit.

<p align="center"> <b>Comparison of HF128, HF64, HF32 and LW32 in programs moving data with CPU</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Bridges-analysis-CPU.png" width="900" align="middle" alt="Bridges-analysis-CPU" />
</p>

<p align="center"> <b> Comparison of HF128, HF64, HF32 and LW32 in Angstrom OS programs moving data with DMA </b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Bridges-analysis-DMA-angstrom.png" width="800" align="middle" alt="Bridges-analysis-DMA-angstrom" />
</p>

<p align="center"> <b> Comparison of HF128, HF64, HF32 and LW32 in baremetal programs moving data with DMA </b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Bridges-analysis-DMA-baremetal.png" width="800" align="middle" alt="Bridges-analysis-DMA-baremetal" />
</p>

HF128 is always faster than the rest of bridges. HF64 and HF32 are close to HF128 performance for most of experiments. However LW32 is very much slower than the others. As a summary of bridge perfomance the following table was created. Each value on the table represents the mean value for all data size, for a combination of the rest parameters.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Bridges-analysis-table.png" width="500" align="middle" alt="Bridges-analysis-table" />
</p>

As explained in the Explanation of the experiments the FPGA-OCR bus width is set to be the same as that of the bridge being used in each experiment. As an exception, some extra experiments have been carried out connecting 64- and 32-bit FPGA-OCRs to the HF128 bridge. Surprisingly, performance was 2%-12% faster than when connecting to bridges with the same width, HF64 and HF32. The conclusion is that HF128 configuration should be always used, even for 64- or 32-bit data width peripherals.

### FPGA Frequency Analysis



<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-CPU.png" width="900" align="middle" alt="Frequency-analysis-CPU" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-DMA-angstrom.png" width="900" align="middle" alt="Frequency-analysis-DMA-angstrom" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-DMA-baremetal.png" width="900" align="middle" alt="Frequency-analysis-DMA-baremetal" />
</p>

### OS vs Baremetal
### Cache Effects
### CPU vs DMA
### DMA Microcode Preparation Time
### Board Comparison


5 - HPS-to-FPGA - L2 Lockdown by Master Experiments
---------------------------------------------------
### Introduction
### Explanation of the Experiments
### Analysis of the Results


6 - HPS-to-FPGA - SDRAM Controller Experiments
----------------------------------------------
### Introduction
### Explanation of the Experiments
### Analysis of the Results
