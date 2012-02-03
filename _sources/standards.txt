Ethernet AVB Standards
======================

Ethernet AVB consists of a collection of different standards that together allow audio and video to be streamed over ethernet. The standards allow synchronized, uninterrupted streaming with multiple talkers and listeners on a switched network infrastructure. 

.. index:: ptp, 802.1as

802.1as
-------

*802.1as* defines a precise timing protocol based on the *IEEE 1558v2* protocol. It allows every device connected to the network to share a common global clock. The protocol allows devices to have a synchronized view of this clock to within microseconds of each other, aiding media stream clock recovery and coordinated AVB traffic control. 

802.1Qav
--------

*802.1Qav* defines a standard for buffering and forwarding of traffic through the network using particular flow control algorithms. It uses the global clock provided by *802.1as* to synchronize traffic forwarding and gives predictable latency control on media streams going through the network.

The XMOS AVB solution implements the requirements for endpoints defined by *802.1Qav*. This is done by traffic flow control in the transmit arbiter of the ethernet MAC component.

802.1Qat
--------

*802.1Qat* defines a stream reservation protocol that provides end-to-end reservation of bandwidth across an AVB network. 


IEC 61883-6
-----------

*IEC 61883-6* defines an audio data format that is contained in *IEEE P1722* streams.

The XMOS AVB solution uses *IEC 61883-6* to convey audio sample streams.  Alternatively, the solution can use the *Simple Audio Format*.

IEEE P1722
----------

*IEEE P1722* defines an encapsulation protocol to transport audio streams over ethernet. It is complementary to the AVB standards and in particular allows timestamping of a stream based on the *802.1as* global clock. 

The XMOS AVB solution handles both transmission and receipt of audio streams using *IEEE P1722*. In addition it can use the *802.1as* timestamps to accurately recover the sample rate clock of the audio to match on the listener side.

IEEE P1722.1
------------

*IEEE P1722.1* is a system control protocol, used for discovery of AVB endpoints, connection management between endpoints, and enumeration and control of parameters exposed by the endpoints.

The XMOS AVB solution supports this emerging standard.

