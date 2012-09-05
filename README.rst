AVB Software Stack
..................

:Stable release:  5.2

:Status:  Reference Software

:Maintainer:  andy@xmos.com

:Description:  Audio Video Bridging Software (AVB)


Key Features
============

* 1722 Packet listener and talker
* 1722 MAAP multicast reservation protocol
* MRP,MMRP,MVRP,SRP 802.1 protocols
* PTP server and protocol
* Audio interface for I2S and TDM
* Clock recovery and interface to PLL clock source
* Support for 1722.1 ADP, AECP (AEM) and ACMP Draft 21


Firmware Overview
=================

This is an implementation of the AVB transport stream protocol carrying audio data. It includes a PTP time
server to provide a stable wallclock reference and clock recovery to synchronize listener audio to talker audio
codecs.  It also includes implementations of the Stream Reservation Protocol for conveying AVB stream reservation
information through 802.1 network infrastructure.

Known Issues
============

* The listener does not respond to the withdrawal of a talker advertise message

Required Repositories
================

* sc_ethernet git\@github.com:xcore/sc_ethernet.git
* sc_xtcp git\@github.com:xcore/sc_xtcp.git
* sc_util git\@github.com:xcore/sc_util.git
* sc_i2c git\@github.com:xcore/sc_i2c.git

To clone (read only):

::

  git clone git://github.com/xcore/sw_avb.git
  git clone git://github.com/xcore/sc_ethernet.git
  git clone git://github.com/xcore/sc_xtcp.git
  git clone git://github.com/xcore/sc_util.git
  git clone git://github.com/xcore/sc_i2c.git

Support
=======

Supported by XMOS Ltd.
