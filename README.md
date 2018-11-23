CycloneVSoC-time-measurements
=============================

Programmable Systems-on-Chip (FPSoCs) are heterogeneous reconfigurable platforms consisting of hard processors and FPGA fabric. Authors have been working with FPSoCs for a long time, and have thoroughly searched many times information regarding the best way to inteconnect processor and FPGA. As a matter of fact, there is little information available on the topic, and even that very little is divided in pieces posted on different, unrelated websites. This repository contains our experiments regarding the processor-FPGA transfer rates in Cyclone V SoC devices, a very important family of FPSoCs. It also serves as support to the paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"*. Since data and figures were too many to fit in a single article all data is provided in this repository. The code used for the experiments is also provided.

Authors belong to the [Electronic Technology Department](http://dteweb.webs.uvigo.es/) of the [University of Vigo](https://uvigo.gal/uvigo_en/index.html). If you have any question regarding the experiments or you find any error in the repository contents please contact us at:
* José Fariña Rodríguez: jfarina@uvigo.es
* Juan José Rodríguez Andina: jjrdguez@uvigo.es [[Personal Web](http://jjrdguez.webs2.uvigo.es/homepage.html)]
* Roberto Fernández Molanes: robertofem@uvigo.es [[GitHub](https://github.com/robertofem)]

Reader may be interested in our other repositories regarding FPSoCs, hosted the [UviDTE-FPSoC](https://github.com/UviDTE-FPSoC/) GitHub organization.

Feel free to use and share all contents in this repository. Please, remember to reference our paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"* if you make any publication that uses our work.
The repository license is GPL v3.0.

Table of contents of this README file:

1. [Repository Contents](#1---repository-contents)
2. [Cyclone V SoC Overview](#2---cyclone-v-soc-overview)
3. [Setting up Cache in Baremetal](#3---setting-up-cache-in-baremetal)
4. [HPS-to-FPGA - Main Experiments](#4---hps-to-fpga---main-experiments)
5. [HPS-to-FPGA - L2 Lockdown by Master Experiments](#5---hps-to-fpga---l2-lockdown-by-master-experiments)
6. [HPS-to-FPGA - SDRAM Controller Experiments](#6---hps-to-fpga---sdram-controller-experiments)
7. [FPGA-to-HPS - Main Experiments](#7---fpga-to-hps---main-experiments)



1 - Repository contents
-----------------------
The repository is organized as follows:
* **This README** file contains explanation of the experiments. For each experiment a summary of the results and the more important plots are provided. The reader can also find a links to the code used for each experiment and a link to the full set of numeric results.
* **code/**: contains all programs used to do the experiments. A README file explaining the code and how to compile and run each program is provided inside their corresponding folders.
* **fpga-hardware**: Quartus/Qsys hardware projects for the FPGA, used to perform the experiments.
* **results/**: contains all numeric results for the experiments.
* **figures/**: contains the figures used in the README files in this repository.

To make use of this repository download it as a zip or install git in your computer and download it using:
```bash
# navigate to the folder where you want to download the repository
git clone https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements.git
```
To compile the programs you need to install Intel FPGA SoC EDS software so the hardware libraries (hwlib) and compilers are available.

2 - Cyclone V SoC Overview
--------------------------
A simplified block diagram of Cyclone V SoC is depicted in the figure below. Cyclone V SoC architecture consists of a Hard Processor System (HPS) and FPGA fabric in a single chip, coupled by high throughput datapaths.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/CycloneVSoC.png" width="500" align="middle" alt="Cyclone V SoC simplified block diagram" />
</p>

The HPS features a microprocessor unit (MPU), 64kB on-chip RAM (HPS-OCR), booting ROM, SDRAM controller (SDRAMC), DMA controller (DMAC), and HPS peripherals (such as Ethernet and USB controllers, UARTs, and timers). The MPU includes two ARM Cortex-A9 32-bit processors, with 32kB L1 data cache and 32kB L1 instruction cache each and 512kB L2 shared instruction and data cache. A Snoop Control Unit (SCU) ensures coherency among L1 caches, L2 cache, and SDRAM contents.

L3 is a partially connected crossbar switch interconnecting all elements in the HPS. The Accelerator Coherency Port (ACP) allows masters accessing L3 to perform coherent requests to the cache memories. All interconnection elements in HPS are AXI3-compliant.

Interconnection resources between the HPS and the FPGA fabric can be divided in two groups:
1)	Those through which HPS accesses FPGA as master:
* The HPS-to-FPGA (HF) bridge, a high-performance bus whose data width is configurable as 32-, 64- or 128-bit (referred to as HF32, HF64 and HF128, respectively, in the remainder of this README).
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

Since each level is equal to the previous just adding a single feature, the effect each feature can be easily  identified.

### Explanation of the Experiments
Using the [code/Baremetal/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_CPU) program we measured the transfer time between the CPU and FPGA for the different parameters:
* Method: memcpy() function is used to perform the transfer.
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory). The program automatically does that.
* Transfer size: From 2B to 2MB in 2x steps. The program automatically does that.
* Cache level: levels 0 to 13 of the cache_configuration(). For each level the macro CACHE_CONFIG in [code/Baremetal/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_CPU)/configuration.h is set to one corresponding level and the program recompiled again. Therefore a total of 14 different programs are generated with different cache levels.

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
* Switching on Data cache L1 (cache configuration 6) has a big effect on trasfer. That´s normal because cache allows transfer data to be retrieved or saved by the CPU faster.
* Switching on the L2 controller (Shared data and instruction L2 cache)(cache configuration 8) has also very big effect.

The rest of the features does not provide a clear advantage. Legup tests found out that Special L2C-310 controller + Cortex A9 optimizations (cache configurations from 9 to 13) provide no advantage for their processing benchmarks. We have found the same conclusions for our data transfer programs.

We used cache configuration 9 in our Baremetal programs.


4 - HPS-to-FPGA - Main Experiments
----------------------------------
### Introduction
These experiments represent the core of HPS-FPGA transfer rate measurements when using the HPS as master to move data. They provide a good overview of the device behaviour. Experiments are performed using Angstrom Operating System and Baremetal. Baremetal measurements provide the fastest data rates that can be achieved in the device. These data can be compared with transfer speed achieved when using OS to observe OS effect on transfers. All design parameters that have been identified to influence transfer rate are considered, with the exception of processor workload, which is kept to a minimum. The reason for this is that it is not practically feasible to consider the many different combination of tasks that the processors may be executing in real applications. Taking into account just a subset of those would result in partial and likely misleading conclusions, difficult (if at all possible) to be extrapolated to specific applications. Therefore, the results of the experiments presented in next section represent the optimal performance under each tested scenario, which is reduced as processor workload increases.

To perform transfers to FPGA an On-Chip RAM (FPGA-OCR) was located in the FPGA simulating any possible peripheral to transfer data with. The FPGA-OCR is connected to any of the HPS-to-FPGA bridges (where HPS acts as master), namely HF128, HF64, HF32 and LW32. The characteristics of the hardware project and compiled FPGA configuration files for different FPGA frequencies and bridge connections can be found in [fpga-hardware/DE1-SoC/FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K). The board used was a Terasic´s DE1-SoC.


### Explanation of the Experiments
Transfer rates between HPS and FPGA when HPS is the master are measured for different combinations of values of the following parameters:

* OS or baremetal implementation:
	* Angstrom, the default Linux-based OS for Intel FPGA devices and well documented in their [support website](https://rocketboards.org/foswiki/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) has been used. It is a light OS, well suited for industrial applications.
	* Baremetal application running in one of the ARM cores.
* Master starting AXI bus transfers:
	* CPU: one of the ARM cores controls data transfer. The program [code/Baremetal/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_CPU) is used for baremetal and [code/Angstrom_OS/HPS-to-FPGA_CPU](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_CPU) for Angstrom OS.
	* DMAC: HPS DMA Controller (PL330) executes data transfer microcode in HPS-OCR, freeing the processor from this task. The program [code/Baremetal/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_PL330DMAC) is used for baremetal and [code/Angstrom_OS/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_PL330DMAC) for Angstrom OS.
* Bridge. All bridges having HPS as master have been tested: LW32, HF32, HP64 and HP128.
* Cache enablement:
	* Caches off. All memory accesses are to external SDRAM. Caches were not switched off for Angstrom since it does not make much sense. Doing it in baremetal is enough to measure the effect of caches in the execution of transfer rate programs.  To switch ON or OFF the caches set to 9 or 0 the macro CACHE_CONFIG in the configuration.h file of each program.
	* Caches on. Accesses are to L1, L2, or external SDRAM, depending on where data are located and the port used.
* Coherency (only applies for DMAC): DMAC can access HPS memories through ACP (coherent access) or directly access external SDRAM through the SDRAMC port from L3 (non-coherent access). In the baremetal tests coherency is automatically changed depending on cache enablement. When cache is OFF non-coherent access is applied (coherent access will be slower because the access would be L3->L2->SDRAM) so it is better to directly do non coherent L3->SDRAM). When cache is ON the access is coherent through ACP. Tests with cache ON and non-coherent access where not performed because they are very similar to those non-coherent with cache OFF. In fact cache ON and non-coherent access will be little faster than non coherent with cache OFF because the few operations done by the CPU will execute faster. We say in plots that non-coherent access with cache off and on are the same. This is conservative and all conclusions derived from plots are therefore valid. In the case of OS the program repeats experiments in coherent and non-coherent way.
* Preparation of DMA microcode program (only applies for DMAC). In most applications the size of the transfer and the address of the destiny and source buffers is known before the transfer takes place so the DMAC microcode programs can be prepared beforehand (during board start-up and initializations) and its preparation time can be saved when the program is actually running the application-ralated tasks. For both, baremetal and OS the transfer time is measured with and without DMAC microcode preparation time.
* Data size from 2B to 2MB, in 2x steps. This automatically done by the program that repeats the measurements for every data size.
* FPGA frequency from 50 to 200MHz, in 50MHz steps. For tests failing at 200MHz, the frequency was reduced in 20MHz steps until correct operation was achieved. Compiled files for the different FPGA frequencies are availabe at [fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf).
* Two DE1-SoC boards have been tested.

Tests are repeated 100 times (automatically done by the application) and mean value is given as result.

These are the data paths depending on AXI master (CPU or DMA) and coherency (through ACP or through SDRAMC):
<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Data_Paths.png" width="900" align="middle" alt="Data_Paths" />
</p>


### General Analysis of the Results
The full set of numeric values for the main experiments is in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_HPS-FPGA_main_time_measurements.xlsx.

The effect of some parameters, namely FPGA frequency, cache enablement and bridge type, is independent of the implementation (OS or baremetal) and the data size. The fastest data transfers are always obtained for the HF128 bridge with caches on. In contrast, results for different coherency (DMAC access through ACP or SDRAMC) or AXI masters (CPU and DMAC) depend on implementation and data size. The maximum frequency all experiments run successfully is 150MHz. As commented later in frequency analysis some of them run at higher frequencies. The following figure shows the most important results: transfers with the maximum frequency all tests run (150MHz) over the fastest bridge (150MHz).

<p align="center"> <b>Transfer rate (in MB/s) of experiments through HF128 bridge with FPGA frequency 150MHz</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/HF128-150MHz.png" width="800" align="middle" alt="Main-results" />
</p>

For all plots, at small data size transfer rate grows with data size because the initialization time (calling the memcpy() function in CPU plus memcpy() initialization tasks or preparing program in DMA in case of using the DMAC) becomes less compared to the transfer time. Initialization time grows as data transfer grows but much slowly than the time where data is actually being transferred. For intermediate data sizes the transfer rate tends to stabilize because initialization time becomes negligible when compared to the actual transfer. When caches are used, performance decreases for sizes above 32kB (that of L1 caches) and again above 512kB (the size of L2 cache) because of cache misses. After cache effects happen (for bigger data size than 512kB) the transfer rate stabilizes and for data size bigger than 2MB the transfer rate at 2MB is expected. Exceptions are plots 9 and 13 that will decrease a little bit after 2MB before they stabilize.

As explained before, cache enablement has a negligible effect in DMA transfers through SDRAMC in baremetal implementations. This can be noticed in the figure, where plots 10 and 14 represent DMA WR and RD operations, respectively, through SDRAMC with both caches off and on.

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

HF128 is always faster than the rest of bridges. HF64 and HF32 are close to HF128 performance for most of experiments. However LW32 is always much slower than the others. As a summary of bridge perfomance the following table was created using data from plots above. Each value on the table represents the mean value for all data sizes, for a combination of the rest parameters.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Bridges-analysis-table.png" width="500" align="middle" alt="Bridges-analysis-table" />
</p>

As explained in Introduction of this section the FPGA-OCR bus width is set to be the same as that of the bridge being used in each experiment. As an exception, some extra experiments have been carried out connecting 64- and 32-bit FPGA-OCRs to the HF128 bridge. Surprisingly, performance was 2%-12% faster than when connecting to bridges with the same width, HF64 and HF32. The conclusion is that HF128 configuration should be always used, even for 64- or 32-bit data width peripherals.

### FPGA Frequency Analysis
The following table shows the maximum frequency the tests correctly finished. The maximum frequency that was tested was 200MHz so some tests could run even at higher frequencies.
<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-table-max.png" width="500" align="middle" alt="Frequency-analysis-table-max" />
</p>

The following figures show graphically the effect of FPGA frequency in the transfer rate. Each point is the mean of transfer rate of all data sizes for that experiment. The higher frequency in the graphs change from 150MHz to 200MHz depending on the bridge because the maximum frequency was different depending on the method used and the bridge.

<p align="center"> <b>Effect of FPGA frequency in transfer rate for CPU</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-CPU.png" width="900" align="middle" alt="Frequency-analysis-CPU" />
</p>

<p align="center"> <b>Effect of FPGA frequency in transfer rate for DMA in Angstrom</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-DMA-ansgtrom.png" width="800" align="middle" alt="Frequency-analysis-DMA-angstrom" />
</p>

<p align="center"> <b>Effect of FPGA frequency in transfer rate for DMA in Baremetal</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-DMA-baremetal.png" width="800" align="middle" alt="Frequency-analysis-DMA-baremetal" />
</p>

A summary of the plotted data is done in the following table for the HF128 bridge. In this table transfer rate when reducing frequency is compared with 150MHz frequency for different combinations of other factors.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Frequency-analysis-table-reduction.png" width="500" align="middle" alt="Frequency-analysis-table-reduction" />
</p>

From the figures and table it can be stated that the fastest the method the more the FPGA frequency becomes the bottleneck and affects the transfer rate. For example DMA is usually faster than CPU and frequency affects more to DMA. Another example is the bridge comparison. LW32 is slower than HF bridges that are much more affected by FPGA frequency than LW32. LW32 transfer rate is almost not affected by FPGA frequency.

In a real application, hardware designs in FPGA should be constrained by the target frequency required for the specific application. If the operating frequency achieved is 150MHz, transfer rates should be in the order of magnitude of those presented in the figure in [General Analysis of the Results](#general-analysis-of-the-results). For other frequencies, data in that figure can be corrected with the previous table (transfer rate reduction) or plots presented in this chapter (or user can access to the full data set of numeric results in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_main_time_measurements.xlsx) to estimate transfer rates.

### OS vs Baremetal
The effect of using an OS with respect to a baremetal implementation for CPU (memcpy) method can be analyzed by comparing plots #1 and #4 with plots #8 and #12 in the figure in [General Analysis of the Results](#general-analysis-of-the-results). Baremetal results with caches on are 5 times faster for WR operations and 2.64 times faster for RD operations than OS ones. The reason for this are the CPU time used by OS for its background tasks and the cache usage by those tasks, that move transfer data out of cache, creating more cache misses for the transfer.

In the case of DMA the effect of the OS can be analyzed comparing plots #2 and #5 with plots #10 and #14 for ACP access and comparing plots #3 and #6 with plots #10 and #14 for SDRAMC access. In the case of DMAC there is also a big reduction in performance too. The reason is an intermediate copy of data between user space (where the application is running) and kernel space (where the driver performing the DMA transfer is running). Driver usage is needed because in user space is not possible to reserve a physically contiguous buffer to be used by DMA controller (except with advanced techniques).

### Cache Effects
The following figure depicts plots #1, #4, #7 and #11 from figure of [General Analysis of the Results](#general-analysis-of-the-results) to more easily appreciate the influence of cache. Transfer rates for CPU with caches on are 10.9 times faster for WR operations and 4.4 times faster for RD operations than when caches are off. The use of caches has more effect in WR operations than in RD ones because most cache optimizations affect the process of reading from processor memory (WR operations read data from processor memory and write them to FPGA-OCR).

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Cache-effects.png" width="400" align="middle" alt="Cache-effects" />
</p>

### AXI Master (CPU or DMAC)
The following figure depicts plots comparing the CPU method (memcpy function) and DMA method with and without preparation time for HF128 and FPGA frequency equal to 150MHz. As explained above in most applications the DMAC microcode preparation time can be saved because the microcode can be prepared during the initialization phase instead preparing it for every transfer. In the plots legend *DMAC-p+t* means transfer using DMA Controller and measuring both preparation time and actual transfer time; legend *DMAC-t* means transfer using DMA Controller only actual transfer time. As it can be observed the preparation time of the DMAC program is big and it should be performed during initialization of the program when possible.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/CPU-vs-DMA-baremetal.png" width="800" align="middle" alt="CPU-vs-DMA-baremetal" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/CPU-vs-DMA-angstrom.png" width="600" align="middle" alt="CPU-vs-DMA-angstrom" />
</p>

The figure shows that at small data sizes the processor performs better than DMA, even when DMAC microcode is prepared before the transfer. Thats because the DMAC does not start inmediately to transfer like the processor. It needs some time to retrieve the microcode from memory and initialize itself. For intermediate and big data sizes DMAC performs better than the processor. The difference is huge when cache is OFF and not so big when cache is ON. Comparicg CPU plots with DMA (without microcode preparation time) the following table was genrated. It contains the best method given a data size, implementation (OS or baremetal) and data direction (RD or WR).


<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Best-AXI-Master-table.png" width="500" align="middle" alt="Best-AXI-Master-table" />
</p>


### DMA Microcode Preparation Time
As commented in the previous subsection the DMAC microcode preparation time represents a big part of a DMA transfer (if it is prepared during the transfer). Substracting the Transfer rates with and without DMA microcode preparation time the preparation time can be obtained. This is represented in the following figures for both cache ON and cache OFF, in nanoseconds (ns) and in percentage of the transfer.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/DMAC-ucode-preparation-time.png" width="800" align="middle" alt="DMAC-ucode-preparation-time" />
</p>

Results show that the DMAC microcode preparation time grows as the data size grows. However in percentage it decreases. For small data sizes (below 2K) represents an 80% of the transfer! After 2kB it tends to decrease stabilizing after 256K around 20%-30% of the transfer time.

### Board Comparison

Small difference in nanometer fabrications could make some chips faster than the others. A comparison between transfer rates in two different DE1-SoC boards is plotted in the following figures.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Difference-boards-CPU.png" width="800" align="middle" alt="Difference-boards-CPU" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Difference-boards-DMA-angstrom.png" width="800" align="middle" alt="Difference-boards-DMA-angstrom" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Difference-boards-DMA-baremetal.png" width="800" align="middle" alt="Difference-boards-DMA-baremetal" />
</p>

Results don´t show that a board is faster than the other. Differences are below 2% in 99% of the cases, and the
maximum measured difference is 23%.

5 - HPS-to-FPGA - L2 Lockdown by Master Experiments
---------------------------------------------------
### Introduction
The 8-way associative L2 cache controller in Cyclone V SoC chip has a very interesting feature called Lockdown by master. It permits to lock (forbid) writing to specific ways of the cache for masters accessing the controller, namely CPU0, CPU1 and ACP. That means that can be used to lock some data in cache a ensure that some masters don´t pollute data that is intended for other masters. It is very useful to lock in cache inportant code or data and ensure fast access by CPUs or ACP. But it could also be used to improve processor-FPGA transfer rate for methods using DMAC and ACP. To explore this possibility we have conducted these experiments.

### Explanation of the Experiments
Using the programs in [code/Angstrom_OS/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_PL330DMAC) and [code/Baremetal/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_PL330DMAC) the  processor-FPGA transfer rate has been measured for a combination of the following parameters:

* OS or Baremetal.
* Locking 1, 4, and 7 ways for CPU; locking 1, 4, and 7 ways for CPU and the others for ACP. This is automatically done by the program when uncommenting the definition of the macro EN_LOCKDOWN_STUDY in the configuration.h file included in each program.
* Locking before and after the CPU generates transfer data.When LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA is defined in the configuration.h files the transfer data is locked after CPU generates it. Otherwise ways are locked and transfer data is prepared afterwards. Locking after data is generated is better because CPU uses all cache to generate data and later some ways are locked for it and it pollutes only a part of it. However locking before data is generated was also tested to see the difference between them.
* Locking with and without dummy CPU memory accesses. Dummy traffic can be added to the transfer uncommenting GENERATE_DUMMY_TRAFFIC_IN_CACHE in the configuration.h file of each program. In regular programs the CPU makes small usage of memory. To better emulate the conditions of some heavy memory access and appreciate better the effects of lockdown some CPU tasks are running to pollute caches. In the case of baremetal bytes are read and write in a spam os 2MB in the while loop that waits for DMA transfer to end. In the case of using OS a thread is launched before any transfer is done. The thread is contionuously accessing memory also in a spam of 2MB.

Tests are repeated 100 times (automatically done by the application) and mean value is given as result.

### Analysis of the Results
All the numeric data set for this experiments is in [/results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_HPS-FPGA_lockdown.xlsx.

Results show that dummy traffic provokes a reduction of 33% and 28% in transfer rate in baremetal for WR and RD, respectively. In the case of Angstrom the reduction is a 2% for WR and a 30% for RD. However, lockdown has not proven effective in mitigate the dummy traffict effect. The following plots show the difference of using some lockdown and not using lockdown at all. When the plot is positive lockdown is faster than not lockdown and viceversa.

<p align="center"> <b>**Lockdown effects in baremetal - HF128 FPGAfreq=150MHz**</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Lockdown-baremetal.png" width="800" align="middle" alt="Lockdown-baremetal" />
</p>

<p align="center"> <b>**Lockdown effects in Angstrom OS - HF128 FPGAfreq=150MHz**</b></p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Lockdown-angstrom.png" width="800" align="middle" alt="Lockdown-angstrom" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Lockdown legend.png" width="350" align="middle" alt="Difference-boards-DMA-angstrom" />
</p>

Results show that lockdown has variable effect on data transfer rates between processor and FPGA. In both Angstrom and baremetal implementations, lockdown may either increase or decrease data transfer rate depending on data size and the number of ways being locked, so no clear conclusions can be reached. It is clear that lockdown is useful to lock a portion of cache for ACP or CPU, e.g., to ensure fast access by ACP or CPU to important portions of code or data, but it does not provide any advantage for the specific task of moving data between CPU and FPGA

6 - HPS-to-FPGA - SDRAM Controller Experiments
----------------------------------------------

### Introduction
SDRAM controller has an scheduler that decides which port accessing to it is granted access in case of simultaneous access. Ports accessing SDRAMC are from L2 cache controller (traffic from CPUs or ACP), L3 port (used in our experiments for DMA with non-coherent accesses) and FPGA-SDRAMC ports. The scheduler provides 2 features to control the scheduling process: priority and weights. When 2 ports different priority the port with highest priority is accessed first in case of simultaneous access. In case they have same priority the bandwidth given to each port is proportional to its associated weight. In example a port with weight 4 will have double bandwidth than a port  with weight 2. That means that access will be granted double number of times.

This feature can be used for the programs using DMA accessing memories from SDRAMC. When CPU is accessing too much to memories more priority or weight can be asigned to the L3-to-SDRAMC port to elevate the DMA transfer rate (at expense of lowering CPU performance). This is the reason to program these experiments.

### Explanation of the Experiments

Using the programs in [code/Angstrom_OS/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/HPS-to-FPGA_PL330DMAC) and [code/Baremetal/HPS-to-FPGA_PL330DMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/HPS-to-FPGA_PL330DMAC) the  processor-FPGA transfer rate has been measured for a combination of the following parameters:

* L3-SDRAMC port with higher priority than CPU port. This is automatically done by the program when uncommenting the definition of the macro EN_SDRAMC_STUDY in the configuration.h file included in each program.
* L3-SDRAMC port and CPU port with the same priority, allocating to L3 2, 4, 8, and 16 times more bandwidth than to CPU port. This is automatically done by the program when EN_SDRAMC_STUDY is defined.
* With and without dummy CPU memory accesses. In the same way that for the Lockdown tests,  dummy traffic can be added to the transfer uncommenting GENERATE_DUMMY_TRAFFIC_IN_CACHE in the configuration.h file of each program.


### Analysis of the Results
All the numeric data set for this experiments is in [/results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_HPS-FPGA_SDRAMC.xlsx. The following plots show the difference of using some SDRAM special configuration (different weights or priorities) and not using lockdown at all. When the plot is positive SDRAM features are advantageous, otherwise using those features is counterproductive.


<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/SDRAM-experiments.png" width="800" align="middle" alt="SDRAM-experiments" />
</p>


<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/SDRAM-legend.png" width="260" align="middle" alt="SDRAM-legend" />
</p>

As can be seen in the plots, in Angstrom implementations these features do not result in improved transfer rate in any of the cases, likely because a large portion of the transfer is performed by the CPU, which copies data from a buffer in user space to another in kernel space. In baremetal implementations, results show that these features don´t provide a clear advantagebelow data size 8kB. Above this size, transfer rates improve only for WR transactions, up to 8% when L3 is allocated 16 times more bandwidth than CPU port and 10% when the priority of L3 is higher than that of CPU port.

7 - FPGA-to-HPS - Main Experiments
----------------------------------
### Introduction
This group of experiments transfers data between FPGA and HPS using the FPGA as master. Along with the previous experiments that use HPS as master, these measurements complete all the possible ways to move information between HPS and FPGA. In this case, DMA controllers implemented in the FPGA are used to read data from HPS, referred as RD operation, or write data stored in the FPGA into the HPS memories, referred as WR operation. Note that in experiments  with the FPGA as master WR and RD operations move data in the oposite direction that in the experiments where HPS is the master. The hardware project used for the experiments is located [fpga-hardware/DE1-SoC/FPGA_DMA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_DMA)

### Explanation of the Experiments
Transfer rates between HPS and FPGA when HPS is the master are measured for different combinations of values of the following parameters:

* OS or baremetal implementation:
	* Angstrom, the default Linux-based OS for Intel FPGA devices and well documented in their [support website](https://rocketboards.org/foswiki/Documentation/AngstromOnSoCFPGA_1#Angstrom_v2013.12) has been used. It is a light OS, well suited for industrial applications. The program used for OS experiments can be found in [code/Baremetal/FPGA-to-HPS_FPGADMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Angstrom_OS/FPGA-to-HPS_FPGADMAC).
	* Baremetal application running in one of the ARM cores. The program used for OS experiments can be found in [code/Baremetal/FPGA-to-HPS_FPGADMAC](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/Baremetal/FPGA-to-HPS_FPGADMAC).

* Bridge. All bridges having FPGA as master have been tested: FPGA-to-HPS bridge and FPGA-to-SDRAM ports, configured as 32-, 64- and 128-bit. To shorten, they will be referred as FH32, FH64, FH128, FS32, FS64 and FS128. As explained in Section \label{FPGA-HPS-hardware} three hardware projects are generated configuring the bridges with the same size (32-, 64-, or 128-bit): 1 DMAC in FH32 and 4 DMACs in FS32, 1 DMAC in FH64 and 4 DMACs in FS64 and 1 DMAC in FH128 and 2 DMACs in FS128. They permit to perform tests using only one DMAC or several DMACs at once.

* FPGA DMA Controllers transferring data:
  * Only the DMAC connected to the FPGA-HPS.
  * Only one of the DMACs connected FPGA-to-SDRAMC ports.
  * All the DMACs connected to the FPGA-to-SDRAMC ports. That means two DMACs when using 128-bit ports and four DMACs when using 32- and 64-bit ports.
  * All DMACs, that is, the DMAC connected to the FPGA-to-HPS bridge and
  all the DMACs connected to the FPGA-to-SDRAMC ports.

* Data direction:
  * WR: read data from FPGA-OCRs and write to processor memories.
  * RD: read data from processor memories and write to FPGA-OCRs.

* Cache enablement. Cache is always ON.

* Coherency (only applies for FPGA-to-HPS bridge): the DMAC connected to FPGA-to-HPS bridge can access processor memories through ACP (coherent access) or through the Main-Switch to SDRAMC port (non-coherent access). When it works alone, both scenarios, coherent and non-coherent, are tested. When it works together with all the DMACs connected to FPGA-SDRAMC ports only non-coherent access is performed.

* Data size from 2B to 2MB, in 2x steps. When more than one DMAC is used the work is equally distributed among the DMACs. To do this the data payload moved by each DMAC is calculated as the target data size (to be moved from FPGA-to-HPS) divided by the number of DMACs.

* FPGA frequency. In baremetal FPGA frequencies tested spam from 50 to 200MHz, in 50MHz steps. In Angstrom OS, only 150MHz and 200MHz were tested.

Tests are repeated 100 times (automatically done by the application) and mean value is given as result.

### General Analysis of the Results
The full set of numeric values for the main experiments is in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_FPGA-to-HPS_main_time_measurements.xlsx.

The results obtained show that fastest data transfers are always achieved using the 128-bit bridges at higher frequencies, as expected.  However big differences are shown depending on the bridge type and number of them used. The following figure shows transfer data rates for different
combinations of bridges for the fastest experiments with 128-bit bridges the FPGA running at 150MHz. As explained in Section later no experiment ran at 200MHz and most did at 150MHz so it is selected as maximum frequency.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-main.png" width="800" align="middle" alt="Main-results" />
</p>

The shapes of all plots are similar to those of the HPS to FPGA experiments. The transfer rates grows fast at small data sizes because initialization effects and because at small data sizes, smaller or equal to the bridge size, a single transaction is carried out. In this cases, since the total time of the transaction is the same for them, bigger data sizes obtain less time per byte and therefore higher transfer rates. At intermediate data sizes these effects become  negligible with respect the actual transfer and the transfer rate stabilizes. At 132kB transfer rate drops for WR operations while it maintains in the case of reads. This effect is appreciable in all methods, including those which directly access SDRAMC and those accessing through ACP. This
effect is due to saturation of the SDRAMC internal buffers and prefetching mechanisms that permit higher rates when less transfer petitions reach to its ports. In the case of ACP the access is done to cache. However the cache is accessing SDRAMC to write data producing the same effect as
the F2S ports.

When comparing the bridge's type and size it can be observed that a FS port is much faster than the FH bridge in all cases and should be always be the preferred option. Another interesting result is that at 150MHz, using 2 FS bridges do not show any advantage with respect to using one. This is not the case at slower FPGA frequencies where using several FS briges can produce higher transfer rates than a sigle FS bridge and compensate the frequency drop, as explained later.

The maximum theoretical speed for a 128-bit bus at 150MHz is 2400MB/s. In this case, all FPGA to HPS transfers are under 1500MB/s. This value is higher than the 1200MB/s obtained for the HPS to FPGA experiments.


### Bridge Size Analysis

When comparing just the size of the bridges there is huge difference when 128-bit is used with repect to 32-and 64-bit bridges. 128-bit bridges of any type are always faster than 32- and 64-bit bridges and should be used to maximize data transfer rate. The following table shows a comparison of bridge size that contains the division of the mean transfer rates for all data sizes in 32-and 64-bit bridges divided by that of 128-bit ones.

Performance of 64-bit bridges (FH64 and FS64) and 32-bit bridges (FH32 and FS32) vs that of the 128-bit bridges (FH128 and FS128) in Cyclone V SoC.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-bridges-table.png" width="550" align="middle" alt="Main-results" />
</p>

A constant reduction is observed for each bridge size. 64-bit bridges have around a 50-60% of the transfer rate of the same bridge configured as 128-bit. 32-bit bridges normally have a 30-40%
of the 128-bit bridge speed. An exception occurs when all FS bridges plus FH bridge are used. In this case data paylod is equally distributed among 4 FS bridges and one FH bridge when 32- and 64-bit bridges are used and among only 2 FS bridges plus one FH in 128-bit bridges. 32-bit and 64-bit have more bridges with less transfer size each. Moreover the FH bridge slows down all the experiments because although FS bridges finish soon the transfer does not end until FH finishes. For this reason smaller reduction is obtained when all bridges are used and for 64-bit WR
the transfer rate is even higher that for 128-bit bridges.

To better visualize the effect of bridge size the following figure shows transfer rate for 32-, 64-, and 128-bit bridges when using a single FS bridge (the fastest case). For the rest of cases the effect is similar, as expresed by the previous table.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-bridges.png" width="600" align="middle" alt="Main-results" />
</p>

When implementing the FPGA to HPS interconnect in some application it is rare that the data stream to be saved in HPS memories is exactly 128-bit width. In this case data can be always packed in order to write words of 128-bit. Even when the speed of 32- or 64-bit is enough to accomodate the transfers of that specific application it is encouraged to use 128-bit bridge so the Main Switch and the SDRAMC (depending on bridge used) have to manage less number of individual transfers, freeing bandwidth for other components using these shared resources, like processor or HPS peripherals. Low sized bridges are only justifiable when the design does not fit in the FPGA or when the maximum FPGA frequency is below the desired one for the application.

On the other hand, when having more than one stream it is usually better to use a bridge for each one. Otherwise, packing words of different streams into 128-bit word to use a single bridge will write the information of each stream criss-crossed in memory and the processor will have to do a hard (and slow) work to grab each word of each stream from among the other stream's bytes and join them in other part of memory to make it usable.

### FPGA Frequency Analysis

The following table shows the maximum frequency achieved during the experiments. No experiment runs at 200MHz. Most of experiments run at maximum of 150MHz but some of them could only do it up to some data size. The maximum data size in kB is marked in parenthesis in the table, next to the maximum frequency. Finally, few experiments could only run at 100MHz.

Maximum frequency achieved in the FPGA to HPS experiments in Cyclone V SoC. The number in parenthesis is the maximum data size in kB that experiment correctly runs.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-freq-table.png" width="700" align="middle" alt="Main-results" />
</p>

The effect of FPGA frequency in transfer rate is summarized for 128-bit bridges in the following Table.

Transfer rate reduction (%) in FPGA-to-HPS 128-bit bridges when lowering the FPGA operating frequency in Cyclone V SoC.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-freq-reduction.png" width="550" align="middle" alt="Main-results" />
</p>

### OS vs Baremetal Implementations
The effect of the OS in the data transfer can be analyzed comparing experiments for Angstrom OS and Baremetal in the figure of General Analysis section. As observed the double copy produced in the driver used in the experiments with OS degrades the speed of the transfers about a 50. The effect is much more noticiable in WR operations.

### Number of Bridges Analysis
Some of the experiments were carried out using more than one bridge at the same time in order to find out if it is possible to speed up the transfer of a single stream in this way. To do that data payload is distributed equally among the number of bridges used and each DMAC moves a smaller
fraction of data.

When using all available FS bridges at 150MHz, the results show no improvement (nor downgrade) with respect using one. This can be observed in figure of the General Analysis of Results for the 128-bit case. However, at smaller frequencies, faster transfer rates are achieved when using two FS bridges, reaching speeds similar to those of 150MHz. This can be observed in the following figure.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/FPGA-HPS-128-1xF2S.png" width="800" align="middle" alt="Main-results" />
</p>

Therefore this technique can be used to compensate a reduction in the maximum transfer rate in case the FPGA frequency must be set lower than 150MHz. For 64- and 32-bit briges, conclusions are the same when comparing one FS with all available FS bridges, four in this case.

Tests were also conducted transferring data with all bridges available from FPGA to HPS, that is, one FH bridge and two FS (128-bit) or one FH bridge and four FS (32- and 64-bit). In this case, splitting a stream is not beneficial because the FH bridge is much slower that FS bridge. Therefore, although DMACs connected to FS bridges finish fast, the CPU has to wait for the FH bridge to finish, giving speeds better than using only the FH bridge but several times slower than using a single FS or all FS. In example, for 128-bit bridges at 100MHz the mean transfer rate achieved is 68MB/s for HF, 502MB/s when using two FS and 135MB/s when using FH and two FS at the same time. Therefore it is recommended not to use HF and FS bridges combined to speed up a single strem. They can be used for different streams though, sending the stream with lower data payloads through HF bridge and the rest through FS ones.
