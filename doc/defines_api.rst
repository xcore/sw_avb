.. _sec_defines_api:

Configuration Defines
=====================

.. note:: 
  
  Each application using the AVB modules must include a file called
  ``avb_conf.h`` and this file must set the following values with    
  #defines.

.. list-table::
 :header-rows: 1
 :widths: 11 15
 
 * - Define
   - Description
 * - ``MAX_ETHERNET_PACKET_SIZE``
   - The maximum Ethernet packet size that can be received by the
     endpoint. Packets larger than this are truncated (default 1518).
 * - ``NUM_MII_RX_BUF``
   - Number of packet buffers for incoming packets.
 * - ``NUM_MII_TX_BUF``
   - Number of packet buffers for outgoing packets.
 * - ``MAX_ETHERNET_CLIENTS``
   - Maximum number of Ethernet clients (i.e. threads connected to the
     Ethernet server).

.. list-table::
 :header-rows: 1
 :widths: 11 15
 
 * - Define
   - Description
 * - ``AVB_MAX_CHANNELS_PER_STREAM``
   - The maximum allowed number of channels per AVB stream (incoming
     or outgoing).
 * - ``AVB_MAX_AUDIO_SAMPLE_RATE``
   - The maximum sample rate in Hz of audio that is to be input or output

.. list-table::
 :header-rows: 1
 :widths: 11 15

 * - Define
   - Description
 * - ``AVB_NUM_SINKS``
   - The total number of AVB sinks (incoming streams that can be
     listened to).
 * - ``AVB_NUM_LISTENER_UNITS``
   - The total number or listener components (i.e. the number of
     threads running the  :c:func:`avb_1722_listener` function).
 * - ``AVB_NUM_SOURCES``
   - The total number of AVB sources (streams that are to be transmitted).
 * - ``AVB_NUM_TALKER_UNITS``
   - The total number or talker components (i.e. the number of
     threads running the  :c:func:`avb_1722_talker` function).

.. list-table::
 :header-rows: 1
 :widths: 11 15

 * - Define
   - Description
 * - ``AVB_NUM_MEDIA_OUTPUTS``
   - The total number of media outputs (i.e. the number of media 
     output FIFOs).
 * - ``AVB_NUM_MEDIA_INPUTS``
   - The total number of media inputs (i.e. the number of media 
     input FIFOs).
 * - ``AVB_NUM_MEDIA_UNITS``
   - The number of components in the endpoint that will register and 
     initialize media FIFOs (e.g. an audio interface component).
 * - ``AVB_NUM_MEDIA_CLOCKS`` 
   - The number of media clocks in the endpoint.

.. list-table::
 :header-rows: 1
 :widths: 11 15

 * - Define
   - Description
 * - ``AVB_ENABLE_1722_1``
   - Enable 1722.1 capability on the endpoint
 * - ``AVB_1722_1_CONTROLLER_ENABLED``
   - Enable 1722.1 Controller capability on the endpoint
 * - ``AVB_1722_1_TALKER_ENABLED``
   - Enable 1722.1 Talker capability on the endpoint
 * - ``AVB_1722_1_LISTENER_ENABLED``
   - Enable 1722.1 Listener capability on the endpoint

Defaults for these #defines are assigned in their absence, but may cause compilation failure or unpredictable/erroneous behaviour.
