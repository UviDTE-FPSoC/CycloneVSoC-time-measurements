Angstrom_OS/HPS-to-FPGA_PL330DMAC
=================================

Introduction
-------------
This application measures the data transfer rate between HPS and FPGA using the Direct Memory Access Controller (DMAC) PL330 available in HPS to do the transfers and Performance Monitoring Unit (PMU) to measure time.

The program performs transfers using the [DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) kernel module ans measures the time of each transfer. The following parameters affecting the transfer speed are studied:
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory).
* Transfer size: From 2B to 2MB in 2x steps.
* Coherency: DMAC can access HPS memories through ACP (coherent access) or to external SDRAM through the SDRAMC port from L3 (non-coherent access).
* DMA initialization time: tests are repeated with or without including the preparation of the DMAC microcode. DMAC microcode can be generated beforehand for lots of applications reducing transfer time.

Transfers are performed from processor memories (cache/SDRAM) and an On-CHip RAM (OCR) in the FPGA. The FPGA hardware project used is available in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K).
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

Results can be printed in screen or can be directly saved into a file.

Description of the code
------------------------

#### configuration.h:
configuration.h permits to control the default behaviour of the program:
* Selecting between  ON_CHIP_RAM_ON_LIGHTWEIGHT ON_CHIP_RAM_ON_HFBRIDGE the program is automatically adapted depending on the hardware project used. By default it is supossed that the FPGA OCR is connected to the HPS-to-FPGA (non Lightweight) bridge.
* Uncommenting EN_LOCKDOWN_STUDY the lockdown by master study can be activated. Regular measurements are repeated changing lockdown by master settings of the L2 8-way associative cache controller. Regular configuration; configurations locking 1, 4 and 7 ways for CPU; and ways locking 1, 4 and 7 ways for CPU and the rest for ACP are tested. When EN_LOCKDOWN_STUDY is activated, uncommenting LOCK_AFTER_CPU_GENERATES_TRANSFER_DATA the CPU and ACP is locked after the transfer data is generated and located in cache without locking. Otherwise the CPU/ACP are locked before data is generated. This study tries to measure the improvement that can be achieved on DMA transfers using ACP when applying lockdown by master.
* Uncommenting EN_SDRAMC_STUDY the SDRAM port priority and weight is activated. Regular measurements are repeated changing settings of the mmpriority, mpweight_0_4 and mpweight_1_4 (see Cyclone V SoC Handbook for more information on these registers). The following combinations are performed: default configuration when starting up the syste; give to CPUs port and L3-to-SDRAM controller port same priority (using mmpriority) giving to L3-to-SDRAM controller port 2, 4, 8 and 16 times more bandwidth (using mpweight_0_4 and mpweight_1_4); give to  L3-to-SDRAM controller port more priority than CPUs port so it is always accessed when both ports access simultaneously to data. This study tries to measure the improvement that can be achieved on DMA transfers using L3-to-SDRAM port when giving more bandwidth or priority to it.
* Uncommenting GENERATE_DUMMY_TRAFFIC_IN_CACHE traffic can be added to chache and main SDRAM memory to affect the DMA transfers and see more clearly the effects of lockdown by master and sdram controller port priority and bandwidth when EN_LOCKDOWN_STUDY or EN_SDRAMC_STUDY are enabled, respectively. The application creates a thread at the beginning of the application that continuously reads and writes the memory in a spam of 2MB to pollute both cache and main SDRAM memory.

#### time_measurements_DMA.c:
First of all the application decides if the results will be printed on screen or in a file. In case the results should go to the screen the user has to call the program just typing the name. In case the user wants results to be printed in file the name of the file should be added as extra argument when calling the program.

After that the PMU is initialized as a timer. PMU is the more precise way to measure time in Cyclone V SoC. Visit this [basic example](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) to learn more about it. It is initialized with a base frequency of 800 MHz (the frequency of the CPU configured in the FPGA hardware project) and freq divider = 1 so the measurement is most precise possible. After initialization empty measuremnts (measuring no code) are done to obtain the PMU measurement overhead. This overhead will be substracted to all measuremnts later on to obtain a more precise measurement.

After it the program generates a virtual address to access FPGA from application space, using mmap(). This is needed to check from the processor if the transfers done by the DMA driver [DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) are being done in proper way. By default the address mapped is the position of the OCR hanging from HPS-FPGA bridge (the default in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) too). If the hardware is changed and the memory is passed to Lightweight HPS-FPGA bridge the following macros must change:
* HPS_FPGA_BRIDGE_BASE (start of the brige) from 0xC0000000 (beginning of HPS-FPGA) to 0xFF200000 (beginning of Lightweight HPS-FPGA).
* ON_CHIP_MEMORY_BASE (start of the memory relative to the beginning of the bridge): from 0 to 0x40000 (or the address the user locates the FPGA_OCR with respect to the beginning of the bridge in Qsys). In the compiled projects in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) the FPGA OCR was located on position 0x40000 but the user can locate it in any address inside the Lightweight HPS-FPGA address space changing this macro acordingly.

Afterwards the FPGA OCR is checked to ensure that the processor has access to all of it. If access is possible the FPGA OCR content is cleared (0s are written).

If GENERATE_DUMMY_TRAFFIC_IN_CACHE is enabled the thread that continuously accesses the memory is launched.

After that the DMA driver [DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) is configured using a sysfs entry in /sys/dma_pl330/ to select between ACP or no ACP and between preparing DMAC microcode before or during the transfer. All 4 combinations are tested.

For each combination of DMA driver settings and each combination of data sizes, 100 measurements on writing the FPGA (WR) and reading the FPGA (RD) are done. In the case the FPGA OCR size (256KB) is smaller than transfer size (2B to 2MB) transfer are repeated until the total desired transfer size is completed (address of main memory keeps growing to see cache effects while address in FPGA is reset). When the 100 measuremnts are done, mean, max, min and variance is calculated and printed in scree or file.

If EN_LOCKDOWN_STUDY is defined measurements are repeated for all combinations of lockdown by master programmed.
If EN_SDRAMC_STUDY is defined measurements are repeated for all combination of port priority and bandwidth programmed.

Lastly the program frees the mapping and the file to save measurements (if used).


Contents in the folder
----------------------
* configuration.h
* time_measurements_DMA.c
* Makefile: describes compilation process.

Compilation
-----------
The regular way to compile is to compile using the SOC EDS compiler. To do that open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**.

If you wanna use the compiler used to compile the [DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) driver (Angstrom 2013.12 toolchain), open and modify the makefile to point to the correct toolchain (comment the SoC EDS toolchain and uncomment the toolchain used to compile the driver, in the first lines of the file). Then just open a Linux Terminal, navigate until the folder of the project and type **_make_**.

The compilation process generates the executable file *time_measurments_DMA*.

This compilation process was tested with both *Altera SoC EDS v14.1* and *Intel FPGA SoC EDS v16.1*.

How to test
------------
* Configure MSEL pins:
    *  MSEL[5:0]="000000" position when FPGA will be configured from SD card.
    *  MSEL[5:0]="110010" position when FPGA will be configured from EPCQ device or Quartus programmer.
* Switch on the board.
* Compile the FPGA hardware ([FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K)) and load it in the FPGA:
    *  If MSEL[5:0]="000000" FPGA is loaded by the U-boot during start-up. Check  the [tutorials to build a SD card with Operating System](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-operating-system) to learn how to configure the SD card so the FPGA is loaded from it.
    *  If MSEL[5:0]="110010" use Quartus to load the FPGA:
        *  Connect the USB cable (just next to the power connector).
        *  Open Quartus programmer.
        *  Click Autodetect -> Mode JTAG -> Select 5CSEMA5 (for DE1-SoC and DE0-nano-SoC) if asked -> Right click in the line representing the FPGA -> Change FIle -> Select the .sof file for the project you want to load -> tick Program/Configure -> Click Start.

* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Seral Terminal (like Putty) at 115200 bauds. Now you have access to the board OS console.
* Copy the [DMA_PL330_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) module in the SD card and insert it into the kernel using _insmod_ command:
```bash
  $ insmod DMA_PL330.ko
```

> Remember. The version of the kernel for which the driver is compiled for should be the same running in the board. This implies that you have to compile the OS you are running in the board and compile the driver with the output files from that compilation. In the [tutorials to build a SD card with Operating System](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-operating-system) and in the [DMA_PL330_LKM folder](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) you can find more information about compiling an Operating System and a Loadable Kernel Moduler (LKM) respectively.

* Copy the executable into the SD card and run the application:
 ```bash
  $ chmod 777 time_measurments_DMA
  $ ./time_measurments_DMA #to print in screen
  $ ./time_measurments_DMA file_name.txt #to print in file
```
Sometimes the program fails with illegal instruction. That´s because the driver activated the PMU in one CPU while this program is running in the other. You can try to remove (using rmmod DMA_PL330.ko) and insert again the driver so the PMU of the other CPU gets activated. Other option is to use taskset to force the application to run in a specific core.
