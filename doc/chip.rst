Choosing the right chip
-----------------------

An XMOS AVB endpoint provides the ability to take IEEE 1722 audio
streams from ethernet and output the audio data.

The number of audio channels the device can handle depends on the
XMOS device used. The XS1 platform is very flexible and can provide
other functions alongside audio (depending on how much audio is
used) including DSP functionality, controlling inputs and
displays on a device or controlling non-AVB ethernet
communication.

The amount of audio available for the XS1-G4 and XS1-L2 devices is
detailed in the following sections. See Section :ref:`sec_resource`
for more information on chip resource usage and 
how these figures were determined. 

.. note:: 

   Please note
   that in a final application the exact capability depends on the
   type of digital audio interface, the mapping between 1722 and local
   streams, the complexity of the routing etc. and these figures are
   meant only as a rough guide.

-  The maximum channel count figures assume that more than two
   channels are used per AVB stream to maximize channel count.

-  The maximum channel count assumes I2S or similar (e.g
   you cannot get maximum number of channels if all are S/PDIF).

-  At 100MBit/s, the capability on the XS1 for higher bit-rates are
   bounded by bandwidth and buffering in line with the AVB standard.

-  For very high channel counts, it is assumed that a multi-channel
   multiplexed 1-wire protocol (*e.g.* TDM) is used to reduce the
   required pin count.

There exist combinations for using the AVB software with higher
channel counts and a gigabit interface on a G4 (or multi-G4)
configuration. For details of these configurations please contact
XMOS. 

XS1-G4 Device - 100Mbit/s
~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1

  * - Sample Rate (kHz)
    - AVB Streams
    - Audio Channels
  * - 48
    - 9in/9out
    - 32 in/32 out
  * - 96
    - 6in/6out
    - 16 in/16 out

XS1-L2 Device - 100Mbit/s
~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1

  * - Sample Rate (kHz)
    - AVB Streams
    - Audio Channels
  * - 48
    - 4in/4out
    - 8 in/8 out
  * - 96
    - 2in/2out
    - 4 in/4 out



.. XS1-G4 Device - Gigabit
.. ~~~~~~~~~~~~~~~~~~~~~~~

..     {\\tabcolsep}{0.2cm}{\\arraystretch}{1.25}

..         {Sample Rate} & {AVB Streams} & {Audio channels}
..         48Khz & 20 in/20 out & 72 in/72 out
..         96Khz & 10 in/10 out & 36 in/36 out



.. 2 x XS1-G4 Device - Gigabit
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..     {\\tabcolsep}{0.2cm}{\\arraystretch}{1.25}

..         {Sample Rate} & {AVB Streams} & {Audio channels}
..         48Khz & 40 in/40 out & 120 in/120 out
..         96Khz & 20 in/20 out & 60 in/60 out


