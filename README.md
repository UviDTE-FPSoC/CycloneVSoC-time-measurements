CycloneVSoC-time-measurements
=============================
```diff
- Message to reviewers: This repository is under construction. It will be totally finished on Friday Sepember 22th.
```
Programmable Systems-on-Chip (FPSoCs) are heterogeneous reconfigurable platforms consisting of hard processors and FPGA fabric. Authors have been working with FPSoCs for a long time, and have thoroughly searched many times information regarding the best way to inteconnect processor and FPGA. As a matter of fact, there is little information available on the topic, and even that very little is divided in pieces posted on different, unrelated websites. This repository contains all our experiments regarding the processor-FPGA transfer rates in Cyclone V SoC devices, a very important family of FPSoCs. It also serves as support to the paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"*. Since data and figures were too many to fit in a single article all data is provided in this repository. The code used for the experiments is also provided.

Feel free to use and share all contents in this repository. Please, remember to reference our paper *"Design Guidelines for Efficient Processor-FPGA Communication in Cyclone V FPSoCs"* if you make any publication that uses our work. 
The repository license is GPL v3.0.

Authors belong to the [Electronic Technology Department](http://dteweb.webs.uvigo.es/) or the [University of Vigo](https://uvigo.gal/uvigo_en/index.html). If you have any question regarding the experiments or you find any error in the repository contents pleas contact us at:
* José Fariña Rodríguez: jfarina@uvigo.es
* Juan José Rodríguez Andina: jjrdguez@uvigo.es [[Personal Web](http://jjrdguez.webs2.uvigo.es/homepage.html)]
* Roberto Fernández Molanes: robertofem@uvigo.es [[GitHub](https://github.com/robertofem)]

Reader may be interested in our other repositories regarding FPSoCs, hosted the [UviDTE-FPSoC](https://github.com/UviDTE-FPSoC/) GitHub organization.

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

After running the 14 programs results are saved into a file and plotted.

### Analysis of the Results
All numeric results from this experiments are in [results](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/results)/CycloneVSoC_baremetal_cache.xlsx. The figure below depicts the mean transfer speed (in MB/s) of data sizes between 32B and 2MB (small data sizes are discarded because their transfer speed is very different to the higher ones and distorted the plots).

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/Cache-baremetal.png" width="500" align="middle" alt="Cache-baremetal" />

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
	* CPU: one of the ARM cores controls data transfer.
	* DMAC: DMAC executes data transfer microcode in HPSOCR, freeing the processor from this task.
• Bridge. All bridges having HPS as master have been tested: LW32, HF32, HP64 and HP128. The hardware project used is [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K). Compiled files for the different bridges are availabe. 
• Cache enablement:
− Caches off. All memory accesses are to external SDRAM.
− Caches on. Accesses are to L1, L2, or external SDRAM,
depending on where data are located and the port used.
• Coherency: DMAC can access HPS memories through ACP
(coherent access) or directly access external SDRAM
through the SDRAMC port from L3 (non-coherent access).
• Data size from 2B to 2MB, in 2x steps.
• FPGA frequency from 50 to 200MHz, in 50MHz steps. For
tests failing at 200MHz, the frequency was reduced in
20MHz steps until correct operation was achieved.
• Two DE1-SoC boards have been tested.
Not all possible combinations have been implemented
because some of them do not make sense or give no
information. For instance, caches are not switched off when
using Angstrom and DMAC access through ACP is not tested
when caches are off.
C. Software
Data transfers are controlled by a software program running
in one of the processors. All programs used to carry out the
experiments are available in [40].
To measure transfer speed a clock cycle counter in the
Performance Monitoring Unit (PMU) is used. PMU is a
coprocessor close to the ARM cores that can gather data about
processor and memory operations. It has been chosen because
it is the most accurate resource available to measure time in the
system and it can be used in both Angstrom and baremetal
implementations.


### General Analysis of the Results
<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/HF128-150MHz.png" width="800" align="middle" alt="Main-results" />
</p>
### Bridge Type Analysis
### FPGA Frequency Analysis
### OS vs Baremetal
### Cache Effects
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
