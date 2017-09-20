CycloneVSoC-time-measurements
=============================
```diff
- Message to reviewers: This repository is under construction. It will be totally finished on Wednesday Sepember 20th.
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
* **code/**: contains all programs used to do the experiments. A README file explaining the code and how to compile and test each program is provided inside the folder of each program.
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
A simplified block diagram of Cyclone V SoC is depicted in the figure below. Cyclone V SoC architecture consists of a Hard Processor System (HPS) and FPGA fabric, coupled by high throughput datapaths in the same chip.

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
### Explanation of the Experiments
### Analysis of the Results


4 - HPS-to-FPGA - Main Experiments
----------------------------------
### Introduction
### Explanation of the Experiments
### General Analysis of the Results
<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/HF128-150MHz.png" width="800" align="middle" alt="Cyclone V SoC simplified block diagram" />
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
