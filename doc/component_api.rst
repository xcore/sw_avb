.. _sec_component_api:

Component functions
===================

The following functions provide components that can be combined in the
top-level main. For details on the ethernet and TCP/IP components see [XEth10]_ and [XTCP10]_.

Core Components
~~~~~~~~~~~~~~~

.. doxygenfunction:: ptp_server

.. doxygenfunction:: media_clock_server

.. doxygenfunction:: avb_1722_listener

.. doxygenfunction:: avb_1722_talker

Audio Components
~~~~~~~~~~~~~~~~

The following types are used by the AVB audio components:

.. doxygentypedef:: media_output_fifo_t

.. doxygentypedef:: media_output_fifo_data_t

.. doxygentypedef:: media_input_fifo_t

.. doxygentypedef:: media_input_fifo_data_t

The following functions implement AVB audio components:

.. doxygenfunction:: init_media_input_fifos

.. doxygenfunction:: init_media_output_fifos

.. doxygenfunction:: i2s_master

.. doxygenfunction:: media_output_fifo_to_xc_channel

.. doxygenfunction:: media_output_fifo_to_xc_channel_split_lr


