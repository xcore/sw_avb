Source code structure
---------------------

Directory Structure
+++++++++++++++++++

The source code is split into several top-level directories (which are
presented as separate projects in Eclipse). These are split into
modules and applications. The applications build into a single
executable using the source code from the modules. The modules used by
an application are specified using the ``USED_MODULES`` variable in
the application Makefile. For more details on this module structure
please see the documentation in the ``module_xmos_common`` directory.

The source package contains  demonstration applications for
different hardware:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Platform
 * - app_xr_avb_lc_demo
   - XR-AVB-LC-BRD reference design kit
 * - app_xdk_avb_demo
   - XDK development kit
 * - app_xdk_xai_avb_demo
   - XDK development kit with XAI audio
     interface board
 * - app_xc2_avb_demo
   - XC-2 development kit
 
Each of these applications uses different modules. 

The ethernet handling modules are:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
 * - module_ethernet
   - Ethernet MAC
 * - module_xtcp
   - XTCP TCP/IP stack
 * - module_zeroconf
   - Zeroconf/Multicast=DNS stack

The following modules contain the core AVB code and are needed by
every application:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
 * - module_avb
   - Main AVB code for control and configuration.
 * - module_avb_1722
   - IEEE P1722 transport (listener and talker functionality).
 * - module_avb_1722_maap
   - IEEE P1722 MAAP - Multicast address allocation code.
 * - module_avb_srp
   - 802.1Qat stream reservation code.
 * - module_osc
   - A module providing an Open Sound Control style tree hierarchy 
     for settings. **Note that no OSC protocol is currently
     implemented**.
 * - module_gptp
   - 802.1as code.
 * - module_avb_audio
   - Code for media FIFOs and audio hardware interfaces (I2S/TDM etc).
 * - module_avb_media_clock
   - Media clock server code.
 * - module_avb_util
   - General utility functions used by all modules.
 * - module_locks
   - A generic module for concurrency locking.
 * - module_avb_attero_cfg
   - Module containing UDP configuration code for communicating with
     the example Atterotech control application.

In addition to the core AVB code the following modules are used by the
demos for other functionality:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
 * - module_i2c
   - Two wire configuration protocol code.
 * - module_xdk_avb_common
   - A selection of code used in XDK AVB demos.
 * - module_xlog
   - A logging server for redirecting prints over UART or an XC
     channel. 
     
Finally, the following module is needed by every application to
provide a common Makefile build system:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Description
 * - module_xmos_common
   - Common build infrastructure.

Key Files
+++++++++

.. list-table::
 :header-rows: 1

 * - File
   - Description
 * - ``avb.h``
   - Header file containing declarations for all AVB component
     functions and the AVB control API.      
 * - ``xtcp_client.h``
   - Header file for clients of the TCP/IP stack.
 * - ``ethernet_rx_client.h`` 
   - Header file for clients that require direct access to the ethernet MAC
     (RX). 
 * - ``ethernet_tx_client.h``
   - Header file for clients that require direct access to the ethernet MAC
     (TX). 
 * - ``gptp.h``
   - Header file for access to the PTP server.
 * - ``mdns.h``
   - Header file for clients wishing to register Zeroconf names/services.
 * - ``audio_i2s.h``
   - Header file containing the I2S audio component.
 * - ``audio_tdm.h``
   - Header file containing the TDM audio component.
