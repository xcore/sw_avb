Ethernet MAC Component
----------------------

The MAC component provides Ethernet connectivity to the AVB
solution. To use the component, a Ethernet PHY must be attached
to the XCore ports via MII. The XS1
device is also capable of implementing a dual 100 Mbps interface. [#]_.

.. [#] Dual MII is not included with the current
       software release. Contact XMOS for more information about the 
       device capability and software availability.

The XMOS Ethernet MAC component supports two features that are necessary to
implement AVB standards with precise timing and quality constraints:

  * *Timestamping* - allows receipt and transmission of Ethernet frames to be timestamped with respect to a clock (for example a 100 MHz reference clock can provide a resolution of 10 ns). 

  * *Time sensitive traffic shaping* - allows traffic bandwidth to be reserved and shaped on egress to provide a steady and guaranteed flow of outgoing media stream packets. The implementation provides flow control to satisfy the requirements of an AVB endpoint as specified in the IEEE *802.1Qav* standard.

The single port 100 Mbps component consists of five logcial cores, each
running at 50 MIPS or more, that must be run on the same tile. These logcial cores handle both the receipt and transmission of
Ethernet frames. The MAC component can be linked via channels to other components/logcial cores in the system. Each link can set a filter to
control which packets are conveyed to it via that channel. 

.. only:: latex

  .. image:: images/single-100-mac.pdf
     :align: center

.. only:: html

  .. image:: images/single-100-mac.png
     :align: center

All configuration of the channel is managed by a client C/XC API, which
configures and registers the filters. Details of the API used to
configure MAC channels can be found in the `Ethernet MAC component documentation <https://www.xmos.com/resources/xsoftip?component=module_ethernet>`_. This API is used for direct (layer-2) access to the
MAC. For AVB applications it is more likely that interaction with the
ethernet stack will be via the main AVB API (see Section
:ref:`sec_avb_api`).

1722 Packet Routing
~~~~~~~~~~~~~~~~~~~

The AVB enabled Ethernet MAC also includes a *IEEE 1722* packet router
that routes audio packets to the listener components in the system. 
It controls the routing by stream ID. This requires no configuration
and is controlled implicitly via the AVB API described in Section 
:ref:`sec_avb_api`.
