FPGA_DMA
===========

Description
------------
This project implements a set of DMA Controllers (DMACs) in the FPGA to test the FPGA-to-HPS bridge and the FPGA-to-SDRAMC ports.

The DMAC used is the (non scatter-gather) DMA Controller available in Platform Designer (formerly Qsys). One DMAC is connected to each available bridges (different number depending on the bridge size). This component uses 2 Avalon
masters to read and write data and contains and extra Avalon slave for its
configuration and control. The master ports of each DMA controller
are connected to one On-Chip RAM (FPGA-OCR) in the FPGA and a bridge in the
HPS so data can be simultaneously moved between FPGA-OCRs and HPS through
all the possible bridges. The following figure shows the block diagram
of the described hardware when the bridges configured as 128-bit.

<p align="center">
  <img src="https://raw.githubusercontent.com/UviDTE-FPSoC/CycloneVSoC-time-measurements/master/figures/fpga-hps-hw.png" width="500" align="middle" alt="Cyclone V SoC with DMACs in the FPGA" />
</p>

Apart from the
DMA buses, depicted as solid lines, the figure shows a 32-bit Avalon bus in
the FPGA, depicted in dotted lines. This bus is used to configure the DMACs
and access to the FPGA-OCRs from the processor. A PLL is
used to provide a variable clock source to all components in the FPGA,
including the FPGA side of the bridges.

The FPGA-to-HPS bridge is a single AXI slave that can be configured with
32-, 64- and 128-bit data width. The SDRAM controller ports offer more
configuration possibilities. The SDRAM controller in Cyclone V SoC contains
four 64-bit ports configurable as AXI or Avalon and mergeable to create
ports of bigger data size. The following configurations are permitted when
all ports are the same size, type and direction (RD or WR):

* One 256-bit AXI port.
* Two 128-bit AXI ports.
* Three 64-bit AXI ports.
* Three 32-bit AXI ports.
* One 256-bit Avalon port.
* Two 128-bit Avalon ports.
* Four 64-bit Avalon ports.
* Four 32-bit Avalon ports.

Since Avalon and AXI contain the same fundamental lines and features,
the same speed may be achieved in both standards when directly
connected to the FPGA-to-SDRAMC ports. For this reason
and because the DMA Controller used is Avalon, only this bus is studied
in FPGA-to-SDRAMC accesses. Moreover, FPGA-to-HPS and the DMA Controller
cannot be configured to perform 256-bit transfers and this width is discarded.
The remaining combinations produce the following FPGA hardware configurations
with bridges of the same size:

* One DMAC connected to 128-bit FPGA-HPS and two DMACs
connected to two 128-bit Avalon FPGA-SDRAM ports. As depicted
in the previous figure.
* One DMAC connected to 64-bit FPGA-HPS and four DMACs
connected to four 64-bit Avalon FPGA-SDRAM ports. Similar to the previous figure
with 64-bit interfaces and two DMA controllers more connected to the FPGA-SDRAM ports.
* One DMAC connected to 32-bit FPGA-HPS and four DMA controllers
connected to four 32-bit Avalon FPGA-to-SDRAM ports. Similar to the previous figure
with 32-bit interfaces and two DMACs more connected to the FPGA-SDRAM ports.

These 3 configurations are the hardware projects used in the experiments.
In some experiments not all DMACs are used. To obtain higher maximum frequency
in the experiments the components not used
are disabled so they do not ocuppy space in the FPGA. For instance,
if only FPGA-to-HPS bridge is to be tested, the DMACs and corresponding
FPGA-OCRs connected to FPGA-to-SDRAM ports are disabled. The folder qsys_projects/ contains qsys projects with the different configuration of direction (RD and WR), bridge size (32-, 64- and 128-bit) and with different number of DMACs enabled (all DMACs enabled, only the DMAC connected to the FPGA-to-HPS enabled, only the DMACs connected to the FPGA-to-SDRAM ports enabled and only one DMAC connected to the FPGA-to-SDRAM ports enabled). To use one of them overwrite the main soc_system.qsys and recompile. The bigger the bridge and the more DMACs instantiated the lower operating frequency achieved. All of them work at least at 100MHz and most of them at 150MHz.

FPGA-to-SDRAM ports and DMACs are both Avalon so they can be directly connected.
However the FPGA-to-HPS bridge is AXI so an Avalon to AXI component is needed
in between the DMAC and this bridge, as depicted in the previous figure.
Its purpose is to configure the extra AXI lines that are not included in the
Avalon bus and that affect transfer capabilities. After testing all
combinations of these AXI lines, the following configuration was selected
to perform all tests because it permits coherent access (needed when accessing
through ACP) and provides the fastest data transfers when using both ACP
and the Main Switch to SDRAMC port (see the L2 controller manual for more information
on these signals):

  * AWCACHE[3-0] = 0111: cache access policy is cacheable write-back, allocate reads only.
  * AWPROT[2-0] = 000: protection normal (non-private), Trustzone non-secure, data payload (to diferentiate from instruction).
  * AWUSER[4-0] = 00001: coherent access when accessing cache through ACP.

Each FPGA-OCR is used to write data from HPS in RD operations and viceversa
in WR ones, and check that the transfer has been successful. Each of the
FPGA-OCR is double port and 1kB, implemented with embedded memory blocks.
The width of FPGA-OCRs and DMACs data bus is always the same of the bus
tested, that is -32, -64 or -128bit.

There is also a PLL sourcing all FPGA components including the FPGA-side of the HPS-to-FPGA and FPGA-to-HPS bridge (not depicted in the figure).


Compilation instructions
--------------------------

This hardware project was tested on Quartus II and Altera SoC EDS v16.0 Update 2. To compile this project:

* Open Quartus (v16.0 Update 2). **Open project > soc_system.qpf**
* Overwrite the **soc_system.qsys** by any of the projects in qsys_projects/ folder, depending on what you wanna test. Change the FPGA operating frequency in the PLL if needed.
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
After running it, a header named *hps_0.h* will be generated on the current directory. This file is the same for all qsys projects in qsys_projects/ folder and has already been generated and saved in the folder code/ip/FPGA_system_headers with the name hps_0_FPGA-HPS.h.
