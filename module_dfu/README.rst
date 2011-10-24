XMOS AVB Device Firmware Upgrade via TFTP
=========================================

:Version: 1.0
:Date: 17/10/2011

This document provides preliminary documentation of a device firmware upgrade mechanism on XMOS AVB devices via the Trivial File Transfer Protocol. It is intended that a customer product can use the provided framework to implement a method of managing firmware upgrade of AVB devices in the field. Source code modules are provided in a manner that enables a customer to implement a transport mechanism other than TFTP, if they wish to do so.

Obtaining the source code
-------------------------

The source code modules mentioned in this document can be obtained from the latest master branch of the sw_avb and sc_xtcp Github repositories. For further information and download links, see the following web pages:

http://github.xcore.com/repo_index/sw_avb_readme.html

http://github.xcore.com/repo_index/sc_xtcp_readme.html

Flash Programming Library
-------------------------

XMOS provides a flash programming library for updating flash devices that are connected to an XCore device. This library can be used for firmware upgrades of a device at runtime. It provides support for reading and writing data to a flash device, making it simple to implement a firmware update mechanism. The transport layer for transferring the firmware to the device is dependent on the application. This functionality is provided as a standard component of the XMOS design tools and as such is available to all customers.

DFU Module
----------

A new module named ``module_dfu`` is provided as part of the AVB reference design in ``sw_avb``. This module provides a layer of abstraction between the AVB application and XMOS flash programming library. It can be extended if the customer wishes to write firmware images to a device other than flash memory.

Five functions are provided to the application in the header file ``avb_dfu.h``:

``int avb_dfu_device_reboot(void);``

``int avb_dfu_init(void);``

``int avb_dfu_data_block_write(unsigned char *data, int num_bytes);``

``int avb_dfu_image_complete(void);``

``int avb_dfu_deinit(void);``

Please refer to this header file for complete documentation of the functions and their intended use.

TFTP Module
-----------

A new example module named ``module_tftp`` is provided in the XMOS TCP/IP networking component ``sc_xtcp``. This module provides a basic TFTP (RFC 1350) implementation over UDP that is used to transfer an upgrade firmware image to the XMOS device from a host PC. 

This module implements a basic TFTP server that accepts a single Write Request connection only. It does not support any of the Option Extensions to TFTP, however, can be easily modified by a customer who requires this functionality.

This document does not cover the use of ``sc_xtcp`` and it is suggested that the user becomes familiar with its API before attempting to modify this module. Full documentation is available via the XMOS Github website at the following address:

http://github.xcore.com/sc_xtcp/index.html

TFTP API
--------

A header file named ``tftp_app.h`` within ``module_tftp`` defines a high level interface to the module from an application. The user must implement the four functions with their intended functionality as part of their application. They must be implemented as a blank function if the user intends not to use the hook, to enable the source code to compile.

``int tftp_app_transfer_begin(void);``

``int tftp_app_process_data_block(REFERENCE_PARAM(unsigned char, data), int num_bytes);``

``void tftp_app_transfer_complete(void);``

``void tftp_app_transfer_error(void);``

Two functions described in the header file ``tftp.h`` must be called in the high level application source code to initialise and provide the necessary TFTP processing within a ``xtcp_event``. For example:

::

 case xtcp_event(c_xtcp, conn):
 {
    ...
    tftp_handle_event(c_xtcp, conn);
    ...
    break;
 }

The necessary functions are:

``void tftp_init(chanend c_xtcp);``

``void tftp_handle_event(chanend c_xtcp, xtcp_connection_t conn);``


Please refer to the header files for complete documentation of the functions and their intended use. An example of a TFTP application providing a DFU service is available in ``app_xc2_avb_demo`` within ``sw_avb``. Further instructions on how to use this example are provided at the end of this document.

TFTP Configuration
------------------

TFTP configuration parameters are provided as #defines in the header file ``tftp_conf.h``. It is recommended that the default values are used, however, the user may wish to edit parameters to test or debug issues with TFTP.

``#define TFTP_DEBUG_PRINT			0``

  Disabled by default. Provides debug print output via the XTAG interface. Set to ``1`` to enable. Note: Debug UART print may slow down TFTP transfer.

``#define TFTP_DEFAULT_PORT			69``

  The default UDP port that the TFTP server listens on for a new connection.

``#define TFTP_BLOCK_SIZE			512``	

  The default data block size in bytes of TFTP DATA packets. Note: 512 bytes is the TFTP default and there is currently no block size negotiation capability.

``#define TFTP_MAX_FILE_SIZE		(128 * 1024) 	/* 128 KB */``

  The maximum size in bytes of a file the TFTP server can accept. In the case of DFU, must be equal to or larger than the biggest upgrade image that the device can expect.

``#define TFTP_TIMEOUT_SECONDS		3``

  The number of seconds after which the connection will close if no new data is received.

``#define TFTP_ACCEPT_ANY_FILENAME 	0``

  The TFTP server only accepts an upgrade image called ``upgrade.bin`` by default. Setting ``TFTP_ACCEPT_ANY_FILENAME`` to ``1`` allows the client to accept any filename.

``#define TFTP_IMAGE_FILENAME		"upgrade.bin"``

  The default upgrade image filename. It can be changed if ``TFTP_ACCEPT_ANY_FILENAME`` is set to ``0``.

TFTP DFU Example Application
----------------------------

XMOS provides an example application that integrates DFU capability via TFTP into the AVB software stack. The application named ``app_xc2_avb_demo`` within ``sw_avb`` is an AVB example endpoint that runs on the XC-2 development kit.

A number of #defines are provided in the application header file ``avb_conf.h`` for convenience. DFU functionality is enabled by default in the application by the following define:

``#define AVB_ENABLE_TFTP_DFU``

To disable upgrade functionality within the example, remove this #define.

The source file ``avb_tftp_dfu.c`` implements the required TFTP functionality as described in the TFTP API section. 
These functions are called from the TFTP server when an event occurs that the application may be interested in. The application interacts with the flash via calls to the ``module_dfu`` functions.

The following instructions may be followed to demonstrate the upgrade capabilities of the AVB firmware. It is assumed that the user has basic knowledge of the XMOS tools. Please refer to the Tools User Guide [#]_ if further detail on the compiling and flashing process is required.

.. [#] Tools User Guide 11.2. https://www.xmos.com/published/tools-user-guide-112

1. No changes need to be made to the source code to generate the factory firmware image that will be flashed onto the XC-2 development board. Compile the ``app_xc2_avb_demo`` application using the standard command line or XDE workflow and obtain the binary output file named ``app_xc2_avb_demo.xe``.

2. Use the following ``xflash`` command to write the combined flash loader and factory image to the flash on the XC-2. 

  ``xflash --id`` *[id]* ``app_x2_avb_demo.xe``

  Where *[id]* is the device ID of the XTAG adapter connected to the XC-2.

  Power cycle the XC-2 board and verify that the factory image boots.

3. To generate a test upgrade image, edit the header file ``avb_conf.h`` and change the line:

  ``#define AVB_DFU_UPGRADE_IMAGE_TEST 0``

  to

  ``#define AVB_DFU_UPGRADE_IMAGE_TEST 1``

  This change flashes an LED on the XC-2 development board to demonstrate when the upgrade image is running. Recompile the application and generate an upgrade image using the following command:

  ``xflash --id`` *[id]* ``--upgrade`` *[version]* ``app_x2_avb_demo.xe -o upgrade.bin``

  Where *[version]* is a version number for the upgrade image, which must be greater than 0.

4. You should now have an upgrade image named ``upgrade.bin`` that can be transferred to the XC-2 via a TFTP client. The firmware running on the XC-2 will attempt to obtain an IP address via mDNS. If this is the only development board connected to the network, the IP address assigned will likely default to 192.254.207.65.

  Note: There may be a delay between 10 and 20 seconds before the device obtains an IP address via mDNS.

  If you are unsure, check the debug UART output from the XTAG to confirm the address. It will be printed after the string ``ipv4ll:``. Alternatively, the XC-2 firmware can be compiled to use a static IP address, however this is outside the scope of this tutorial.

5. Ensure that the XC-2 development board is connected to the host PC directly or via an Ethernet switch. You should be able to ping the XC-2 via the IP address obtained in step 4. If you are unable to ping the device, check your connections and the IP address of the XC-2.

6. You are now ready to flash the upgrade image to the device. Using your favourite TFTP client, transfer the file ``upgrade.bin`` to the IP address of the XC-2 board. A typical TFTP client command to do this is:

  ``tftp 169.254.207.65 PUT upgrade.bin``

  Check the documentation of the particular TFTP client you are using. Ensure that the client is transferring the file in binary (octet) mode and that the blocksize is set to 512 bytes. 

7. The TFTP client should indicate that the file transfer has completed. If the upgrade image is valid, the XC-2 should immediately reboot and an LED should be flashing to indicate that the new image is running. DFU capability is also enabled in the upgrade image, so it is possible to overwrite an upgrade image with a newer upgrade image via TFTP.

The factory image will always remain untouched. The flash loader will recognise a corrupt upgrade image via its CRC check and only boot the valid factory image. For more information on this functionality, see the section titled *Targeting Flash Devices* in the Tools User Guide.

Note: The #define 

``#define AVB_DFU_TRAP_ON_FLASH_ERROR 1``

in ``avb_conf.h`` enables a run-time check that the XCore device connects to the flash correctly. It will trigger an exception and print the line number of the assertion in the source code if a problem has occurred. This is useful for debugging problems with the flash. It should not be used in a production environment, as it will cause the firmware to stop executing.
