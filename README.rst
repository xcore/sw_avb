AVB Software Stack
..................

:Latest release: 5.2.0beta1
:Maintainer: ajwlucas
:Description: AVB software stack


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

This firmware is a reference endpoint implementation of Audio Video Bridging protocols for XMOS silicon. It includes a PTP time
server to provide a stable wallclock reference and clock recovery to synchronise listener audio to talker audio
codecs. The Stream Reservation Protocol is used to reserve bandwidth through 802.1 network infrastructure.

Known Issues
============

Support
=======

The HEAD of this repository is a work in progress. It may or may not compile from time to time, and modules, code and features may be incomplete. For a stable, supported release please see the reference designs section at www.xmos.com.

Required software (dependencies)
================================

  * sc_i2c
  * sc_ethernet
  * sc_otp (https://github.com/xcore/sc_otp.git)
  * sc_util (https://github.com/xcore/sc_util.git)

