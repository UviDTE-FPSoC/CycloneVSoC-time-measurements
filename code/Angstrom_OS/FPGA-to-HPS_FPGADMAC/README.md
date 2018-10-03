Angstrom_OS/FPGA-to-HPS_FPGADMAC
=================================

Introduction
-------------
This application measures the data transfer rate between FPGA and HPS using Direct Memory Access Controllers (DMACs) in the FPGA. The DMAC used is the (non scatter-gather) DMAC available in Platform Designer (formerly Qsys). One DMAC is connected per available port: one to the FPGA-to-HPS bridge and two (-128-bit) or four (32- and 64-bit) to the FPGA-to-SDRAM ports, depending on the bridge size. The hardware project used is the [FPGA_DMA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_DMA). To control the DMACs a custom API, located in code/inc/FPGA_DMA, has been developed.
To allocate physically contiguous buffer used by the DMACs the programs uses the [Alloc_DMAble_buff_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM) driver.

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

#### time_measurements_DMA.c:

The main() is here. When entering in the main the program prepares some variables to do the experiments. The most important is data_size_dmac, that contains the data transfer of the DMACs depending on the number of DMACs used. data_size_dmac[i][j] is the data transfer when using i+1 DMACs for the data size data_size[j].

Then the program generates the address of the DMACs and FPGA-OCR memories in the FPGA to have acess to it from the processor. The address used is the beginning of the bridge where the component is connected (HPS-to-FPGA bridge) plus the address of the memory with respect to the beginning of the bridge (address in Qsys). After that the FPGA-OCR memories are checked (to test that all Bytes are accesible) and reset.

After that the AXI signals in the Avalon to AXI component (needed for the DMAC connected to the FPGA-to-HPS bridge) are set to 0x00870087 (fastest) and the FPGA-to-SDRAM ports are removed from reset so they are usable from FPGA. The rest of hardware configuration needed (enablement of SDRAMC ports, ACP configueation and enable access of PMU from user space) are carried out by the Alloc_DMAble_buff_LKM driver.

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
* time_measurements_DMA.c
* Makefile: describes compilation process.

Compilation
-----------
The regular way to compile is to compile using the SOC EDS compiler. To do that open *SoC EDS Command Shell* (*Intel FPGA SoC EDS* needs to be installed in your system), navigate to the folder of the example and type **_make_**. This program makes usage of some hwlib files (in configuration.h) to retrieve some hardware constants. The provided makefile is prepared to automatically detect and adapt to both the old hwlib folder structure (before *Intel FPGA SoC EDS v15*) and the new (*Intel FPGA SoC EDS v15* and ahead) so the included files are found.

The compilation process generates the executable file *time_measurments_DMA*.

This compilation process was tested with *Intel FPGA SoC EDS v16.1*.

How to test
------------
* Configure MSEL pins:
    *  MSEL[5:0]="000000" position when FPGA will be configured from SD card.
    *  MSEL[5:0]="110010" position when FPGA will be configured from EPCQ device or Quartus programmer.
* Switch on the board.
* Compile the FPGA hardware ([FPGA_DMA](https://github.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/tree/master/fpga-hardware/DE1-SoC/FPGA_DMA)) and load it in the FPGA:
    *  If MSEL[5:0]="000000" FPGA is loaded by the U-boot during start-up. Check  the [tutorials to build a SD card with Operating System](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-operating-system) to learn how to configure the SD card so the FPGA is loaded from it.
    *  If MSEL[5:0]="110010" use Quartus to load the FPGA:
        *  Connect the USB cable (just next to the power connector).
        *  Open Quartus programmer.
        *  Click Autodetect -> Mode JTAG -> Select 5CSEMA5 (for DE1-SoC and DE0-nano-SoC) if asked -> Right click in the line representing the FPGA -> Change FIle -> Select the .sof file for the project you want to load -> tick Program/Configure -> Click Start.

* Connect the serial console port (the mini-USB port in DE1-SoC) to the computer and open a session with a Seral Terminal (like Putty) at 115200 bauds. Now you have access to the board OS console.
* Copy the [Alloc_DMAble_buff_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM) module in the SD card and insert it into the kernel using _insmod_ command:
```bash
  $ insmod alloc_dmable_buffer.ko
```

> Remember. The version of the kernel for which the driver is compiled for should be the same running in the board. This implies that you have to compile the OS you are running in the board and compile the driver with the output files from that compilation. In the [tutorials to build a SD card with Operating System](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/SD-operating-system) and in the [Alloc_DMAble_buff_LKM](https://github.com/UviDTE-FPSoC/CycloneVSoC-examples/tree/master/Linux-modules/Alloc_DMAble_buff_LKM) you can find more information about compiling an Operating System and a Loadable Kernel Moduler (LKM) respectively.

* Copy the executable into the SD card and run the application:
 ```bash
  $ chmod 777 time_measurments_DMA
  $ ./time_measurments_DMA #to print in screen
  $ ./time_measurments_DMA file_name.txt #to print in file
```
Sometimes the program fails with illegal instruction. That´s because the driver activated the PMU in one CPU while this program is running in the other. You can try to remove (using rmmod Enable_PMU_user_space.ko) and insert again the driver so the PMU of the other CPU gets activated. Other option is to use taskset to force the application to run in a specific core.
