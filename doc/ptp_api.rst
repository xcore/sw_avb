.. _sec_ptp_api:

PTP Client API
==============

The PTP client API can be used if you want extra information about the PTP
time domain. An application does not need to directly use this to
control the AVB endpoint since the talker, listener and media clock
server units communicate with the PTP server directly.


Time Data Structures
~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: ptp_timestamp

Getting PTP Time Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygentypedef:: ptp_time_info
.. doxygentypedef:: ptp_time_info_mod64

.. doxygenfunction:: ptp_get_time_info
.. doxygenfunction:: ptp_get_time_info_mod64

.. doxygenfunction:: ptp_request_time_info
.. doxygenfunction:: ptp_request_time_info_mod64

.. doxygenfunction:: ptp_get_requested_time_info
.. doxygenfunction:: ptp_get_requested_time_info_mod64

Converting Timestamps
~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: local_timestamp_to_ptp

.. doxygenfunction:: local_timestamp_to_ptp_mod32

.. doxygenfunction:: ptp_timestamp_to_local


