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
   - The maximum ethernet packet size that can be received by the
     endpoint. Packets larger than this are truncated (default 1518).
 * - ``NUM_MII_RX_BUF``
   - Number of packet buffers for incoming packets.
 * - ``NUM_MII_TX_BUF``
   - Number of packet buffers for outgoing packets.
 * - ``MAX_ETHERNET_CLIENTS``
   - Maximum number of ethernet clients (i.e. threads connected to the
     ethernet server).

.. list-table::
 :header-rows: 1
 :widths: 11 15
 
 * - Define
   - Description
 * - ``AVB_MAX_NAME_LEN``
   - The maximum length in characters of any text name used in the
     endpoint (e.g. the name of a source).
 * - ``AVB_MAX_CHANNELS_PER_STREAM``
   - The maximum allowed number of channels per AVB stream (incoming
     or outgoing).
 * - ``AVB_MAX_SAMPLE_RATE``
   - The maximum allowed sample rate in the system. This is used to 
     set the audio fifo buffer sizes.

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

