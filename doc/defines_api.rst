.. _sec_defines_api:

Configuration Defines
=====================

Demo and hardware specific
--------------------------

Demo parameters and hardware port definitions are set in a header configuration file named ``app_config.h`` within the ``src/`` directory
of the application.

.. doxygendefine:: AVB_DEMO_ENABLE_TALKER
.. doxygendefine:: AVB_DEMO_ENABLE_LISTENER
.. doxygendefine:: AVB_DEMO_NUM_CHANNELS

Core AVB parameters
-------------------
  
Each application using the AVB modules must include a header configuration file named
``avb_conf.h`` within the ``src/`` directory of the application and this file must set the following values with #defines.

See the demo application for a realistic example.

.. note:: 

  Defaults for these #defines are assigned in their absence, but may cause compilation failure or unpredictable/erroneous behaviour.

Ethernet
--------
See the Ethernet documentation for detailed information on its parameters.

Audio subsystem
---------------

.. doxygendefine:: AVB_MAX_AUDIO_SAMPLE_RATE

.. doxygendefine:: AVB_NUM_SOURCES
.. doxygendefine:: AVB_NUM_TALKER_UNITS
.. doxygendefine:: AVB_MAX_CHANNELS_PER_TALKER_STREAM
.. doxygendefine:: AVB_NUM_MEDIA_INPUTS

.. doxygendefine:: AVB_NUM_SINKS
.. doxygendefine:: AVB_NUM_LISTENER_UNITS
.. doxygendefine:: AVB_MAX_CHANNELS_PER_LISTENER_STREAM
.. doxygendefine:: AVB_NUM_MEDIA_OUTPUTS

.. doxygendefine:: AVB_NUM_MEDIA_UNITS
.. doxygendefine:: AVB_NUM_MEDIA_CLOCKS

1722.1
------

.. doxygendefine:: AVB_ENABLE_1722_1
.. doxygendefine:: AVB_1722_1_TALKER_ENABLED
.. doxygendefine:: AVB_1722_1_LISTENER_ENABLED
.. doxygendefine:: AVB_1722_1_CONTROLLER_ENABLED

Descriptor specific strings can be modified in a header configuration file named
``aem_entity_strings.h.in`` within the ``src/`` directory. It is post-processed by a script
in the build stage to expand strings to 64 octet padded with zeros.

.. list-table::
 :header-rows: 1
 :widths: 11 15

 * - Define
   - Description
 * - ``AVB_1722_1_ENTITY_NAME_STRING``
   - A string (64 octet max) containing an Entity name
 * - ``AVB_1722_1_FIRMWARE_VERSION_STRING``
   - A string (64 octet max) containing the firmware version of the Entity
 * - ``AVB_1722_1_GROUP_NAME_STRING``
   - A string (64 octet max) containing the group name of the Entity
 * - ``AVB_1722_1_SERIAL_NUMBER_STRING``
   - A string (64 octet max) containing the serial number of the Entity
 * - ``AVB_1722_1_VENDOR_NAME_STRING``
   - A string (64 octet max) containing the vendor name of the Entity 
 * - ``AVB_1722_1_MODEL_NAME_STRING``
   - A string (64 octet max) containing the model name of the Entity