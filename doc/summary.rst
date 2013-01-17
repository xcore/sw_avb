Summary
=======


The XMOS Audio Video Bridging (AVB) endpoint is a reference design that provides time-synchronized, low latency streaming services through IEEE 802 networks. The solution is firmware that is implemented on the XMOS xCORE architecture and can be deployed on a number of different xCORE parts depending on system requirements such as stream and channel count.

.. only:: latex

 .. image:: images/avb_xmos_overview.pdf

.. only:: html

 .. image:: images/avb_xmos_overview.png

XMOS AVB Key Features
---------------------

* 100 Mbit/s full duplex Ethernet interface via MII
* Support for 1722.1 discovery, enumeration, command and control: ADP, AECP (AEM) and ACMP Draft 21
* Simultaneous 1722 Talker and Listener support for sourcing and sinking audio
* 1722 MAAP support for Talker stream MAC address allocation
* 802.1Q Stream Reservation Protocols for QoS including MSRP, MMRP and MVRP
* 802.1AS Precision Time Protocol server for synchronization
* I2S, TDM and other audio interfaces for connection to external codecs and DSPs
* Media clock recovery and interface to a PLL clock source for high quality audio reproduction