AVB Software Stack
..................

:Latest release: 5.1.2rc2
:Maintainer: ajwlucas
:Description: AVB software stack





Key Features
============

* 1722 Packet listener and talker
* 1722 MAAP multicast reservation protocol
* MRP,MMRP,MVRP,SRP 802.1 protocols
* PTP server and protocol
* Audio interface for I2S and TDM
* Clock recovery and interface to PLL clock source
* Support for 1722.1 ADP and ACMP draft D15A

Firmware Overview
=================

This is an implementation of the AVB transport stream protocol carrying audio data. It includes a PTP time
server to provide a stable wallclock reference and clock recovery to synchronize listener audio to talker audio
codecs.  It also includes implementations of the Stream Reservation Protocol for conveying AVB stream reservation
information through 802.1 network infrastructure.

Known Issues
============

* The listener does not respond to the withdrawal of a talker advertise message

Support
=======

Supported by XMOS Ltd.

Required software (dependencies)
================================

  * sc_xlog
  * sc_i2c
  * sc_xtcp
  * xcommon (if using development tools earlier than 11.11.0)
  * sc_ethernet

To clone (read only):

::

  git clone git://github.com/xcore/sw_avb.git
  git clone git://github.com/xcore/sc_ethernet.git
  git clone git://github.com/xcore/sc_xtcp.git
  git clone git://github.com/xcore/sc_xlog.git
  git clone git://github.com/xcore/sc_i2c.git
  git clone git://github.com/xcore/xcommon.git

