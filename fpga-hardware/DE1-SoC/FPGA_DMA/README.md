FPGA_DMA
===========

Description
------------
This project implements a double port On-Chip RAM (FPGA-OCR) and a DMA Controller Core (available in Qsys) controller in the FPGA. Its purpose is to test the DMA Controller and serve as example to use it. With the connections of the DMA COntroller ports data can be copied from the FPGA-OCR to some memory in the HPS. This project is a modification of the DE1-SoC Golden Hardware Reference Design (GHRD) available in the DE1-SoC CD-ROM documentation.

The OCR configured in the FPGA has the following characteristics:

* Implemented using embedded 10kB memory blocks.
* Size = 1kB, the maximum power of two feasible in DE1-SoC board.
*  Data_size is 128bits. The bigger the data size the faster the data rates achieved so 128 bits is selected cause it is the maximum of the bridge where it is connected.
* Double port. One port is connected to the HPS-to-FPGA bridge so the processor
can access it. Its address relative to the HPS-to-FPGA bridge is 0x0 (0x00000000 in Qsys). The physical address to be used from processor is threfore the sum of the beginning of the HPS-to-FPGA bridge (0xC0000000) plus the Qsys address (0x0).
The second port is connected to the read port of the DMA Controller core at address 0x0 too.

The DMA Controller Core has the following characteristics:
* The 32-bit control bridge is connected to the HPS-to-FPGA bridge at address 0x10000 so the processor can control it. Therefore the physical addresss of the component as seen by the processor is again the sum of the bridge address (0xC0000000) plus the DMA Controller address (0x10000) equal to 0xC0010000.
* The Read master bus is configured 128-bit and connected to the second port of the FPGA-OCR. The Write master is connected to the FPGA-to-HPS bridge so it can write to all HPS addresses, including SDRAM-Controller, ACP and HPS On-Chip RAM (HPS-OCR).
* Bursts are not enabled.
* Max transfer size allowed 16MB.
* FIFO depth 128 Bytes.

The following drawing depicts the hardware just described:

<p align="center">
  <img src="https://raw.githubusercontent.com/robertofem/CycloneVSoC-examples/soft_dma_baremetal/FPGA-hardware/DE1-SoC/FPGA_DMA/FPGA_DMA.png" width="500" align="middle" alt="Cyclone V SoC with DMA in FPGA" />
</p>

 There is also a PLL sourcing all FPGA components including the FPGA-side of the HPS-to-FPGA and FPGA-to-HPS bridge (not depicted in the figure):

 * Input frequency: 50MHz from external oscillator in the DE1-SoC board.
 * Output frequency: 100MHz. This clock is used to source the FPGA OCR and the FPGA side of the HPS-FPGA bridge.

Compilation instructions
--------------------------

This hardware project was tested on Quartus II and Altera SoC EDS v16.0 Update 2. To compile this project:

* Open Quartus (v16.0 Update 2). **Open project > soc_system.qpf**
* Open Qsys and **load soc_system.qsys**
* On Qsys, Select **Generate > Generate HDL...** De-select “Create block symbol file” option and specify desired HDL language (VHDL our case). Press “Generate” button.
* After generation ends, go to Quartus and press the **Start Analysis & Synthesis** button
* When synthesis ends, go to **Tools > Tcl scripts...** and run the scripts hps_sdram_p0_parameters.tcl and hps_sdram_p0_pin_assignments.tcl. Wait for confirmation pop-up window.
* Perform again the **Analysis & Synthesis** of the project
* Run the **Fitter (Place & Route)** utility
* Run the **Assembler (Generate programming files)** utility

**NOTE:** The last 3 steps could be run altogether pressing the “Start Compilation” button


Generate hardware address map header
-----------------------------------------

To generate the system header file, first open the *SoC EDS Command Shell*. Then, the following instruction can be run from the project root directory, and it will generate a header file describing the HPS address map. It can be used by an HPS C/C++ program to get base addresses and other specifications of the FPGA
peripherals.
```bash
  $ sopc-create-header-files --single hps_0.h --module hps_0
```
After running it, a header named *hps_0.h* will be generated on the current directory.
