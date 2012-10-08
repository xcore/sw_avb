AVB Software Stack
..................

:Stable release:  5.2


:Maintainer:  andy@xmos.com



Key Features
============

* 1722 Talker and Listener (simultaneous) support
* 1722 MAAP support for Talkers
* 802.1Q MRP, MMRP, MVRP, SRP protocols
* gPTP server and protocol
* Audio interface for I2S and TDM
* Media clock recovery and interface to PLL clock source
* Support for 1722.1 AVDECC: ADP, AECP (AEM) and ACMP Draft 21

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

* sc_ethernet git\@github.com:xcore/sc_ethernet.git
* sc_xtcp git\@github.com:xcore/sc_xtcp.git
* sc_util git\@github.com:xcore/sc_util.git
* sc_i2c git\@github.com:xcore/sc_i2c.git
* sc_otp git\@github.com:xcore/sc_otp.git

To clone (read only):

::

  git clone git://github.com/xcore/sw_avb.git
  git clone git://github.com/xcore/sc_ethernet.git
  git clone git://github.com/xcore/sc_xtcp.git
  git clone git://github.com/xcore/sc_util.git
  git clone git://github.com/xcore/sc_i2c.git
  git clone git://github.com/xcore/sc_otp.git
