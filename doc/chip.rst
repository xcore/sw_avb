Choosing the right chip
-----------------------

The number of audio channels the device can handle depends on the
XMOS device used. The XS1 platform is very flexible and can provide
other functions alongside audio (depending on how much audio is
used) including DSP functionality, controlling inputs and
displays on a device or controlling non-AVB ethernet
communication.

The amount of audio available for the XS1-L devices is
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

-  At 100MBit/s, the capability on the XS1 for higher bit-rates are
   bounded by bandwidth and buffering in line with the AVB standard.

-  For greater than 8 in/ 8 out channels, it is assumed that a multi-channel
   multiplexed protocol such as TDM is used to reduce the
   required pin count.

XS1-L16 Device
~~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1

  * - Sample Rate (kHz)
    - AVB Streams
    - Audio Channels
  * - 48
    - 4 in/4 out
    - *16 in/16 out (TDM)*
  * - 48
    - 4 in/4 out
    - 8 in/8 out
  * - 96
    - 2 in/2 out
    - 4 in/4 out

XS1-L10 Device
~~~~~~~~~~~~~~

.. list-table::
  :header-rows: 1

  * - Sample Rate (kHz)
    - AVB Streams
    - Audio Channels
  * - 48
    - 1 in/1 out
    - 4 in/4 out
  * - 96
    - 1 in/1 out
    - 2 in/2 out


