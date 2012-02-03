.. _sec_config:

Configuration and application threads
-------------------------------------

The Control Thread
++++++++++++++++++

As well as the components described in previous sections, an AVB
endpoint application needs a thread to control and configure the
system. This control thread varies across application and must
be implemented by the application designer. To assist in this task a
unified control API is presented in Section :ref:`sec_avb_api`. The
control thread can pass XC channels connected to other components into
the :c:func:`avb_init` function and subsequently use the control API calls to
configure the other components.

For an example of how an application can use this API, see the example
code walkthrough presented in Section :ref:`sec_app_tutorial`.

Legacy Mode
~~~~~~~~~~~

AVB protocols require an AVB compatible switch to function correctly. 
For testing/demonstration purposes the software platform provides 
a "legacy" mode
which alters the protocols to be non standard but function through
non-AVB switches. In this mode the destination addresses of the
protocols change to become legacy traffic and some of the PTP protocol
behavior changes. In this case:

  * The protocols are non-longer AVB standard so do not work with
    any other AVB hardware. They will work with other endpoints 
    set to this mode.
  * There is no longer any quality of service guarantee so audio        
    may be disrupted. Furthermore the traffic from the endpoint may
    disrupt other non-AVB ethernet devices on the network.

Remote Configuration
++++++++++++++++++++

The AVB standard protocols control the time synchronization, streaming
and routing of audio data. However, it is likely that a higher level
configuration protocol will be required to configure an AVB endpoint.

Such a protocol needs two parts: discovery and control. The
discovery part must determine higher level information about other
endpoints on the network. For example:

 * Discover which other Talkers/Listeners are on the network.
 * Discover which streams are available and meta-information about
   them (sample rate, description, global clock synchronization
   participation *etc.*).

The XMOS TCP/IP implementation canbe used with the AVB solution to
provide services such as MDNS.

The control part controls the device. For example it controls:

 * The streams the Talker outputs/Listener inputs.
 * Audio input/output (including sample rate, gain, dsp).
 * Other non-audio aspects.

A control stack will link into the control thread to provide a bridge
to the local control API. To aid implementation of this, a the XMOS TCP/IP
stack can be used. A demonstration control API is provided for
the XR-AVB-LC-BRD, for more details see the `AtteroTech/XMOS XR-AVB-LC-BRD Quickstart Guide <http://www.atterotech.com/cobranet-oem-products/xmos-avb-module/>`_.

TCP/IP Stack
++++++++++++

The AVB software solution includes a port of the uIP protocol stack
which is a small memory footprint stack that can be used for UDP/TCP
communication to aid implementation of an upper layer configuration
protocol. For details on this stack see the `XTCP Component Guide <http://github.xcore.com/sc_xtcp/index.html>`_.

Zeroconf
++++++++

The Zeroconf stack (sometimes known as "Bonjour") provides:

    * **Multicast DNS** - Allows endpoints to have a local name
      related to their IP address.
    * **DNS Service Discovery** - Allows endpoints to advertize a 
      particular service (such as a configuration/control service over
      TCP/IP) for other entities on the network to discover.

The stack is configured via the API described in Section :ref:`sec_mdns_api`.
      

