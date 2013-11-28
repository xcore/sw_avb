.. _avb_dc_quickstart:

AVB-DC Quick Start Guide
========================

This guide is intended for customers who have purchased the AVB-DC kit based on sliceKIT (XK-SK-AVB-DC).
It applies to version 1.0 of the AVB-DC firmware.

Obtaining the latest firmware
-----------------------------

#. Log into xmos.com and access `My XMOS` |submenu| `Reference Designs`
#. Request access to the `XMOS AVB-DC Software Release` by clicking the `Request Access` link under `AVB DAISY-CHAIN KIT`. An email will be sent to your registered email address when access is granted.
#. A `Download` link will appear where the `Request Access` link previously appeared. Click and download the firmware zip.


Installing xTIMEcomposer Tools Suite
------------------------------------

The AVB-DC software requires xTIMEcomposer version 13.0.0 or greater. It can be downloaded at the following URL
https://www.xmos.com/en/support/downloads/xtimecomposer


Importing and building the firmware
-----------------------------------

To import and build the firmware, open xTIMEcomposer Studio and
follow these steps:

#. Choose `File` |submenu| `Import`.

#. Choose `General` |submenu| `Existing Projects into Workspace` and
   click **Next**.

#. Click **Browse** next to **`Select archive file`** and select
   the firmware .zip file downloaded in section 1.

#. Make sure that all projects are ticked in the
   `Projects` list.
 
#. Click **Finish**.

#. Select the ``app_daisy_chain`` project in the Project Explorer and click the **Build** icon in the main toolbar.

Installing the application onto flash memory
--------------------------------------------

#. Connect the 20-way IDC header on the xTAG-2 debug adapter to the XSYS connector on the 
   first sliceKIT core board adapter. 
#. Plug the xTAG-2 into your development system via USB.
#. In xTIMEcomposer, right-click on the binary within the *bin* folder of the project.
#. Choose `Flash As` |submenu| `Flash Configurations`.
#. Double click `xCORE Application` in the left panel.
#. Choose `hardware` in `Device options` and select the relevant xTAG-2 adapter.
#. Click on **Apply** if configuration has changed.
#. Click on **Flash**. Note that the firmware will not run until the board is reset.
#. Repeat steps 1 through 8 for the second sliceKIT.

Setting up the hardware
-----------------------

.. only:: latex

  .. image:: images/board.jpg
     :align: center

Refer to the above figure for the correct setup of the I/O sliceCARDs to the sliceKIT core boards.

#. The Ethernet sliceCARDs (XA-SK-E100) must be inserted into the slots denoted by the Circle and Square symbols.
#. The audio sliceCARD (XA-SK-AUDIO-PLL) must be inserted into the slot denoted by the Triangle symbol.
#. Connect the two sliceKITs together via an Ethernet cable between either of the Ethernet ports.
#. Connect one of the remaining Ethernet ports to an AVB-capable Apple Mac running OS X 10.9 Mavericks.
#. Connect the provided 12V power supplies to the input power jacks of the boards and power them on.

Apple Mac OS X Setup
--------------------

All Apple Macs with a Thunderbolt port are AVB capable. 

To enumerate and stream audio between a Mac and XMOS AVB-DC endpoints:

#. Install/upgrade to OS X Mavericks Version >=10.9
#. Connect the XMOS AVB daisy chain to the Mac via the Ethernet port or Thunderbolt Ethernet adapter.
#. Open the *Audio MIDI Setup* utility.
#. In the menu bar, select `Window` |submenu| `Show Network Device Browser`.

    .. image:: images/show_browser.png
       :align: center

#. XMOS AVB-DC endpoints will enumerate in this list as *AVB 4in/4out*. Select the checkbox to the left of the entries to connect
   the devices. Pressing the *Identify* button will identify the particular device by flashing LED1 and LED2 on the audio sliceCARD.

    .. image:: images/network_device_browser.png
       :align: center   

#. On successful connection, the devices will appear as Audio Devices in the *Audio MIDI Setup* window.

#. The devices can be streamed to individually, aggregated as an 8in/8out device, or treated as a Multi-Output Device.
   Aggregate or Multi-Output devices can be created in the Audio MIDI Setup window by clicking on the arrow in the bottom left corner.

    .. image:: images/create_aggregate.png
       :align: center

#. Once created, an Aggregate or Multi-Output device can be configured to use the XMOS AVB-DC endpoints by selecting the 'Use' checkboxes
   beside the Audio Devices.

    .. image:: images/ams.png
       :align: center

#. To enable audio streaming to/from a device, right click on the device in the left pane and select *Use this device for sound input* and
   *Use this device for sound output*.

    .. image:: images/set_default.png
       :align: center   

#. Audio can now be played and recorded via the endpoints.

.. note::
   Note: Volume and sample rate control of AVB audio devices is not currently available via Audio MIDI Setup
