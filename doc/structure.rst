Source code structure
---------------------

Directory Structure
+++++++++++++++++++

The source code is split into several top-level directories which are
presented as separate projects in xTIMEcomposer Studio. These are split into
modules and applications.

Applications build into a single
executable using the source code from the modules. The modules used by
an application are specified using the ``USED_MODULES`` variable in
the application Makefile. For more details on this module structure
please see the XMOS build system documentation.

The source package contains a simple demonstration application `app_simple_avb_demo` that can be run on different hardware targets.

Some support modules originate in other repositories:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
   - Repository
 * - module_ethernet
   - Ethernet MAC
   - sc_ethernet
 * - module_ethernet_board_support
   - Hardware specific board configuration for Ethernet MAC
   - sc_ethernet
 * - module_i2c_simple
   - Two wire configuration protocol code.
   - sc_i2c
 * - module_random
   - Random number generator
   - sc_util

The following modules contain the core AVB code and are needed by
every application:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
 * - module_avb
   - Main AVB code for control and configuration.
 * - module_avb_1722
   - IEEE 1722 transport (listener and talker functionality).
 * - module_avb_1722_1
   - IEEE P1722.1 AVB control protocol.
 * - module_avb_1722_maap
   - IEEE 1722 MAAP - Multicast address allocation code.
 * - module_avb_audio
   - Code for media FIFOs and audio hardware interfaces (I2S/TDM etc).
 * - module_avb_media_clock
   - Media clock server code for clock recovery.
 * - module_avb_srp
   - 802.1Qat stream reservation (SRP/MRP/MVRP) code.
 * - module_avb_util
   - General utility functions used by all modules.
 * - module_gptp
   - 802.1AS Precision Time Protocol code.
     

Key Files
+++++++++

.. list-table::
 :header-rows: 1

 * - File
   - Description
 * - ``avb_api.h``
   - Header file containing declarations for the core AVB control API.
 * - ``avb_1722_1_app_hooks.h``
   - Header file containing declarations for hooks into 1722.1  
 * - ``ethernet_rx_client.h`` 
   - Header file for clients that require direct access to the ethernet MAC
     (RX). 
 * - ``ethernet_tx_client.h``
   - Header file for clients that require direct access to the ethernet MAC
     (TX). 
 * - ``gptp.h``
   - Header file for access to the PTP server.
 * - ``audio_i2s.h``
   - Header file containing the I2S audio component.