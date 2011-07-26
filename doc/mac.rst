Ethernet MAC Component
----------------------

The MAC component provides ethernet connectivity to the AVB
solution. To use the component, a physical interface must be attached
to the XCore ports to provide an 100 Mbps MII interface. The XS1
device is also capable of implementing a dual 100 Mbps interface and
a gigabit GMII interface [#]_.

.. [#] Dual MII and gigabit GMII code is not included with the 5v1
       software release. Contact XMOS for more information about the 
       device capability and software.

The MAC component supports two features that are necessary to
implement  AVB standards with precise timing and quality constraints. 

  * *Timestamping* - allows receipt and transmission of ethernet frames to be timestamped with respect to a clock (for example a 100 MHz reference clock can provide a resolution of 10 ns). 

  * *Bandwidth control* - allows different channels to have different
     priorities and bandwidth restrictions to allow steady flow of
     outgoing media stream packets. The implementation provides flow
     control to satisfy the requirements of an AVB endpoint as
     specified in the IEEE *802.1Qav* standard.

The single port 100 MBit component consists of five threads (each
running at 50 MIPS or more) that must be run on the same core. These threads handle both the receiving and transmission of
ethernet frames. The MAC component can be linked (via channels) to other components/threads in the system. Each link can set a filter to
control which packets are conveyed to it via that channel. 

.. only:: latex

  .. image:: images/single-100-mac.pdf
     :width: 70%
     :align: center

.. only:: html

  .. image:: images/single-100-mac.png
     :align: center

All configuration of the channel is managed by a client C/XC API, which
configures and registers the filters. Details of the API used to
configure MAC channels can be found in the `ethernet MAC component design guide <http://github.xcore.com/sc_ethernet/index.html>`_. This API is used for direct (layer-2) access to the
MAC. For AVB applications it is more likely that interaction with the
ethernet stack will be via the main AVB API (see Section
:ref:`sec_avb_api`).

1722 Packet Routing
~~~~~~~~~~~~~~~~~~~

In the AVB stack the MAC also includes a *IEEE P1722* packet router
that routes audio packets to the listener components in the system. 
It controls the routing by stream ID. This requires no configuration
and is controlled implicitly via the AVB API described in Section 
:ref:`sec_avb_api`.
