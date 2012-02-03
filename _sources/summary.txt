Summary
=======


The XMOS Audio Visual Bridging (AVB) reference design can be used to stream synchronized audio over an ethernet 
network. The XMOS solution is based on event-driven programmable devices, which can transmit and receive multiple audio streams, and implement both talker and listener functionality.

.. only:: latex

 .. image:: images/avb_xmos.pdf

.. only:: html

 .. image:: images/avb_xmos.png

XMOS AVB Features
-----------------

 * Supports all endpoint requirements: ethernet interface, packet processing, timing synchronization, configuration/stream setup and media rate recovery can all be handled on device.
 * Supports emerging AVB standards such as *IEEE 802.1as*, *IEEE 801.1Qav* and *IEEE P1722*.
 * Flexible software based design implements both hardware interfaces and protocol layers in the same environment allowing flexibility in system design and easy modification.
 * Multiple audio hardware interfaces supported, for instance *I2S*, *TDM* and *S/PDIF*.
