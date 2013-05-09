.. _avb_quickstart:

AVB Quick Start Guide
=====================

This guide is intended for customers who have purchased the Low-Cost AVB Audio Endpoint Kit.
It applies to version 5.2 of the reference design firmware.

Obtaining the latest firmware
-----------------------------

#. Log into xmos.com and access `My XMOS` |submenu| `Reference Designs`
#. Request access to the `XMOS AVB Reference Design` by clicking the `Request Access` link under `LOW-COST AVB AUDIO ENDPOINT KIT`. An email will be sent to your registered email address when access is granted.
#. A `Download` link will appear where the `Request Access` link previously appeared. Click and download the firmware zip.
#. Do not extract the firmware .zip file


Installing the xTIMEcomposer studio
-----------------------------------

The latest development tools can be obtained at the following URL https://www.xmos.com/resources/downloads


Importing and building the firmware
-----------------------------------

To import and build the firmware, open the xTIMEcomposer Studio and
follow these steps:

#. Choose `File` |submenu| `Import`.

#. Choose `General` |submenu| `Existing Projects into Workspace` and
   click **Next**.

#. Click **Browse** next to **`Select archive file`** and select
   the firmware .zip file downloaded in section 1.

#. Make sure that all projects are ticked in the
   `Projects` list.
 
#. Click **Finish**.

#. Select the ``app_simple_avb_demo`` project in the Project Explorer and click the **Build** icon.

Installing the application onto flash
-------------------------------------

#. Connect the 20-way IDC header on the XTAG-2 debug adapter to the XSYS connector on the 
   first development board, then plug the XTAG-2 into your development system via USB.
#. In the xTIMEcomposer Studio, Right click on the binary within the bin folder of the project.
#. Choose `Flash As` |submenu| `Flash Configurations`
#. Double click `xCORE Application` in the left panel
#. Choose `hardware` in `Device options` and select the relevant XTAG-2 adapter
#. Click on **Apply** if configuration has changed
#. Click on **Flash**. Note that the firmware will not run until the board is reset.
#. Repeat steps 1 through 8 for the second development board.

Setting up the hardware
-----------------------

.. only:: latex

  .. image:: images/board.pdf
     :align: center

Refer to the above figure for board controls and connectors.

#. Connect the two development boards together directly via an Ethernet cable between the RJ45 connectors.
#. Set the audio input connection jumpers for either RCA input or 3.5 mm jack input.
#. On the first development board, connect the output of a line-level audio source to the audio input connector.
#. On the second development board, connect an audio playback device to the audio output connector.
#. If not already powered, connect the power supplies to the input power jacks of the boards and power them on.

.. note:: 
    Note: The audio output from the board is line level. If using headphones, an external headphone amplifier may be required.

The endpoints will automatically connect to each other via 1722.1 and audio should immediately be heard. 
Note that the endpoints are configured as a simultaneous Talker and Listener and audio streaming will operate in both directions.

Connection via an Ethernet switch
---------------------------------

To evaluate more than two endpoints, or operate a third party 1722.1 Controller, 
an AVB-enabled switch is required. A number of options are available from switch provider Extreme Networks and other vendors. 
See the AVB System Requirements Guide for further information.

Management by an external 1722.1 Controller
+++++++++++++++++++++++++++++++++++++++++++

To disable the internal 'plug and play' 1722.1 Controller that auto-connects streams on the XMOS endpoints,
press the **Remote** button on all development boards. All active streams will be disconnected.

A third party 1722.1 Controller application running on a PC, for example, 
can then be used to connect and disconnect streams between the endpoints.

Next Steps
----------

Read the AVB Design Guide to learn more about the XMOS AVB reference design and begin customising the firmware: https://www.xmos.com/published/avb-reference-design-guide-0

Design files for the XR-AVB-LC-BRD board can be obtained under xKITS Resources on xmos.com: https://www.xmos.com/resources/xkits?category=Low-cost+AVB+Audio+Endpoint+Kit