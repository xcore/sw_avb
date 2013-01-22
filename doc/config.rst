.. _sec_config:

Device Discovery, Connection Management and Control
---------------------------------------------------

The Control Task
++++++++++++++++

In addition to components described in previous sections, an AVB
endpoint application requires a task to control and configure the
system. This control task varies across applications but the protocol to provide device discovery, connection management and control services has been standardised by the IEEE in 1722.1.

1722.1
++++++

The 1722.1 standard defines four independent steps that can be used to connect end stations that use 1722 streams to transport media across a LAN. The steps are:

a) Discovery
b) Enumeration
c) Connection Management
d) Control

These steps can be used together to form a system of end stations that interoperate with each other in a standards compliant way. The application that will use these individual steps is called a Controller and is the third member in the Talker, Listener and Controller device relationship.

A Controller may exist within a Talker, a Listener, or exist remotely within the network in a separate endpoint or general purpose computer.

The Controller can use the individual steps to find, connect and control entities on the network but it may choose to not use all of the steps if the Controller already knows some of the information (e.g. hard coded values assigned by user/hardware switch or values from previous session establishment) that can be gained in using the steps. The only required step is connection management because this is the step that establishes the bandwidth usage and reservations across the AVB cloud.

The four steps are broken down as follows:

 * Discovery is the process of finding AVB endpoints on the LAN that have services that are useful to the other
   AVB endpoints on the network. The discovery process also covers the termination of the publication of those
   services on the network.
 * Enumeration is the process of the collection of information from the AVB endpoint that could help an
   1722.1 Controller to use the capabilities of the AVB endpoint. This information can be used for connection
   management.
 * Connection management is the process of connecting or disconnecting one or more streams between two or more
   AVB endpoint.
 * Control is the process of adjusting a parameter on the endpoint from another Entity. There are a number of standard
   types of controls used in media devices like volume control, mute control and so on. A framework of basic
   commands allows the control process to be extended by the endpoint.

.. note:: 
   The XMOS endpoint provides full support for Talker and Listener 1722.1 services. Basic 1722.1 Controller functionality is available to allow 'plug and play' connection between two XMOS endpoints, however, it is expected that GUI Controller software will be available on the network for setting up larger topologies.

To assist in this task a unified control API is presented in Section :ref:`sec_avb_api`.