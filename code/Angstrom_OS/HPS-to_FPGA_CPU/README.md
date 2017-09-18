HPS-to-FPGA_CPU
================

Introduction
-------------
This application measures the data transfer rate between HPS and FPGA using CPU with memcpy or for loops methods to do the transfers. Time measurement is done by Performance Monitoring Unit (PMU) because it is the most precise timer in the system, and repeated again using time.h library functions to compare with PMU measurements and asses that PMU is measuring correctly.

The following parameters affecting the transfer speed are studied:
* Method: memcpy() function or "dir" method copy using for loop copying word by word data. 
* Transfer direction: WR (read from processor and WRITE to FPGA) and RD (READ from FPGA and write in processor memory).
* Transfer size: From 2B to 2MB in 2x steps.

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

configuration.h permits to control the default behaviour of the program: 
* Selecting between   ON_CHIP_RAM_ON_LIGHTWEIGHT,  ON_CHIP_RAM_ON_HFBRIDGE32, ON_CHIP_RAM_ON_HFBRIDGE64 and ON_CHIP_RAM_ON_HFB the program is automatically adapted depending on the hardware project used. By default it is supossed that the FPGA OCR is connected to the HPS-to-FPGA (non Lightweight) bridge with 128-bit width.

Description of the code
------------------------
First of all the application decides if the results will be printed on screen or in a file. In case the results should go to the screen the user has to call the program just typing its name. In case the user wants results to be printed in file the name of the file should be added as extra argument when calling the program.

Afterwards the program generates a virtual address to access FPGA from application space, using mmap(). This is needed to access hardware addresses from a user space application (that uses virtual addresses). mmap makes a mapping of the hardware addresses to some equivalent virtual addresses we can use from within the application. By default the address mapped is the position of the OCR hanging from HPS-FPGA bridge with 128-bit(the default in [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K) too). If the bridge size is changed to 64-bit or 32-bit or if the memory is mapped to Lightweight bridge is changed ON_CHIP_RAM_ON_HFBRIDGE128 should be commented in configuration.h and its corresponding macro should be uncommented. If the address of the memory in Qsys is changed the system headers of the bridge modified should be updated in the [code/inc/FPGA_system_headers](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/FPGA_system_headers). See [FPGA_OCR_256K](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K)to know how to generate the system headers after compiling a FPGA project. To ease the test of the program all FPGA projects, varying bridge type and size and FPGA frequency are made available in [FPGA_OCR_256K/compiled_sof_rbf](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/FPGA-hardware/DE1-SoC/FPGA_OCR_256K/compiled_sof_rbf). If these hardware projects are used no modification to [code/inc/FPGA_system_headers](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/code/inc/FPGA_system_headers) will be needed.

Afterwards the FPGA OCR is checked to ensure that the processor has access to all of it. If access is possible the FPGA OCR content is cleared (0s are written).

First set of measurements are performed using time.h library to measure time. CLOCK_REALTIME is used since provides the smallest resolution (1ns) and error (around 240 ns) among all OS timers. After initialization empty measuremnts (measuring no code) are done to obtain the CLOCK_REALTIME measurement overhead (720ns). This overhead will be substracted to all measuremnts later on to obtain a more precise measurement. 

For each combination of data sizes, 100 measurements on writing the FPGA (WR) and reading the FPGA (RD) are done. In the case the FPGA OCR size (256KB) is smaller than transfer size (2B to 2MB) transfer are repeated until the total desired transfer size is completed (address of main memory keeps growing to see cache effects while address in FPGA is reset). When the 100 measuremnts are done, mean, max, min and variance is calculated and printed in screen or file. First measuremnts with memcpy() function are done. memcpy() is a function from standard library to efficiently copy data between parts of the system using CPU. Secondly transfers are performed with the for loop method. In this case a for loop is created and data is copied word by word (one word per loop is copied). For loop resulted much slower than memcpy() so always use memcpy().

After that the PMU is initialized as a timer. PMU is the more precise way to measure time in Cyclone V SoC. Visit this [basic example](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Baremetal-applications/Second_counter_PMU) to learn more about it. It is initialized with a base frequency of 800 MHz (the frequency of the CPU configured in the FPGA hardware project) and freq divider = 1 so the measurement is most precise possible (1ns resolution). After initialization empty measuremnts (measuring no code) are done to obtain the PMU measurement overhead (30ns). In the same way as for tests with time.h, this overhead is substracted to all measuremnts later on to obtain a more precise measurement.

Same measurements done for time.h are repeated for PMU.

Lastly the program frees the mapping and the file to save measurements (if used).


Contents in the folder
----------------------
* time_measurements_CPU: all code of the program previously explained is here.
* Makefile: describes compilation process.
* configuration.h

Compilation
-----------
The regular way to compile is to compile using the SOC EDS compiler. To do that open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**. This program makes usage of some hwlib files (in configuration.h) to retrieve some hardware constants. The provided makefile is prepared to automatically detect and adapt to both the old hwlib folder structure (before *Intel FPGA SoC EDS v15*) and the new (*Intel FPGA SoC EDS v15* and ahead) so the included files are found.

The compilation process generates the executable file *time_measurments_CPU*.

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
* Copy the [Enable_PMU_user_space](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Enable_PMU_user_space) module in the SD card and insert it into the kernel using _insmod_ command:
```bash
  $ insmod Enable_PMU_user_space.ko
```
By default the access to PMU from user space is forbidden. Thats the reason to insert this driver which only function is to access from kernel space to the PMU and set a bit that allows user space access to PMU (needed to perform time measurements from this application). When inserting the driver the output in the kernel console should reflect the change in the bit of the PMU permitting access from user space.
> Remember. The version of the kernel for which the driver is compiled for should be the same running in the board. This implies that you have to compile the OS you are running in the board and compile the driver with the output files from that compilation. In the [tutorials to build a SD card with Operating System](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-operating-system) and in the [DMA_PL330_LKM folder](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/DMA_PL330_LKM) you can find more information about compiling an Operating System and a Loadable Kernel Moduler (LKM) respectively.

* Copy the executable into the SD card and run the application:
 ```bash
  $ chmod 777 time_measurments_CPU
  $ ./time_measurments_CPU #to print in screen
  $ ./time_measurments_CPU file_name.txt #to print in file
```
Sometimes the program fails with illegal instruction. That´s because the driver activated the PMU in one CPU while this program is running in the other. You can try to remove (using rmmod Enable_PMU_user_space.ko) and insert again the driver so the PMU of the other CPU gets activated. Other option is to use taskset to force the application to run in a specific core.