Creating custom applications
----------------------------

To create your own AVB application, the general steps are:

#. Make a copy of the Eclipse project or
   application directory (the directory starting with ``app_``)
   you wish to base your
   code on, to a separate directory with a different name.

#. Make a copy of any modules you wish to alter, and update the Makefile of your
   new application to use these new custom modules.

#. Make appropriate changes to the code, rebuild and reflash the
   device for testing.

If you are using a custom board and have made a copy, you need to:

#. Provide a ``.xn`` file for your board (updating the `TARGET`
   variable in the Makefile appropriately).
#. Update ``avb_conf.h`` with the specific defines you wish
   to set.
#. Update ``avb_device_defines.h`` and ``avb_device_defines.c`` to
   provide device specific information to the AVB control API.
#. Update the top level main.
#. Add any custom code in other files you need.

Board Design
------------

The key PCB components for an XMOS AVB solution are:

#. XS1-G4 or XS1-L2 chip

#. 100 Mbit/s MII ethernet phy (100 Mbit) 

#. SPI flash for boot image loading and persistent storage

#. PLL/frequency synthesizer chip

#. An audio CODEC or processor

To aid board design please refer to the schematics that can be
found at:

  http://www.xmos.com/support/documentation.

This page includes reference designs for XMOS chips (including BOM
costs). The following schematics show examples connecting to
additional hardware:

.. list-table::
 :header-rows: 1
 
 * - Schematic 
   - Component
 * - XR-AVB-LC-BRD 
   - XS1-L2, Ethernet, Flash, PLL, Audio CODECs
 * - XC-2
   - XS1-G4, Ethernet, Flash
 * - XAI Audio Interface
   - PLL, Audio CODEC, S/PDIF

It is *highly* recommend to add an XSYS connector for an XTAG2 device
onto a board for debugging.
