AVB Audio Module
================

This module provides audio input and output functionality to an AVB
endpoint. It consists of three parts:

  * `Interfaces`_: Digital audio interfaces
  * `Codecs`_ : Codec configuration and controls
  * `Media Fifos`_: Code for controlling the media input/output fifos that connect to IEEE 1722 listener/talker units

Media Fifos
-----------

.. include:: media_fifo.h, media_input_fifo.h, media_output_fifo.h
 
Interfaces
----------

.. include:: i2s.h, spdif.h, tdm.h, synth.h

Codecs
------

.. include:: audio_codec_CS42448.h, audio_codec_TLV320AIC23B.h
