System Architecture
-------------------

The following diagram shows the overall structure of an XMOS AVB endpoint.

.. only:: html

  .. figure:: images/avbsystem.png
     :align: center

     AVB System Software Architecture

.. only:: latex

  .. figure:: images/avbsystem.pdf
     :figwidth: 70%
     :align: center

     AVB System Software Architecture

An endpoint consists of five main interacting components:


  * The ethernet MAC
  * The precise timing engine (PTP)
  * Audio streaming components
  * The media clock server
  * Configuration and other application components

Demo Applications
~~~~~~~~~~~~~~~~~

The software platform comes with four demo applications:

.. list-table:: 
 :header-rows: 1

 * - Directory
   - Platform
 * - app_xr_avb_lc_demo
   - XR-AVB-LC-BRD reference design kit
     (including a demo control protocol implementation that can be 
     controlled by a Windows configuration application)
 * - app_xdk_avb_demo
   - XDK development kit
 * - app_xdk_xai_avb_demo
   - XDK development kit with XAI audio
     interface board
 * - app_xc2_avb_demo
   - XC-2 development kit
