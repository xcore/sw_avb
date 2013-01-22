Ethernet AVB Standards
======================

Ethernet AVB consists of a collection of different standards that together allow audio and video to be streamed over Ethernet. The standards provide synchronized, uninterrupted streaming with multiple talkers and listeners on a switched network infrastructure. 

.. index:: ptp, 802.1as

802.1AS
-------

*802.1AS* defines a Precision Timing Protocol based on the *IEEE 1558v2* protocol. It allows every device connected to the network to share a common global clock. The protocol allows devices to have a synchronized view of this clock to within microseconds of each other, aiding media stream clock recovery to phase align audio clocks.

The `IEEE 802.1AS-2011 standard document`_ is available to download free of charge via the IEEE Get Program.

.. _`IEEE 802.1AS-2011 standard document`: http://standards.ieee.org/getieee802/download/802.1AS-2011.pdf

802.1Qav
--------

*802.1Qav* defines a standard for buffering and forwarding of traffic through the network using particular flow control algorithms. It gives predictable latency control on media streams flowing through the network.

The XMOS AVB solution implements the requirements for endpoints defined by *802.1Qav*. This is done by traffic flow control in the transmit arbiter of the Ethernet MAC component.

The 802.1Qav specification is available as a section in the `IEEE 802.1Q-2011 standard document`_  and is available to download free of charge via the IEEE Get Program.

.. _`IEEE 802.1Q-2011 standard document`: http://standards.ieee.org/getieee802/download/802.1AS-2011.pdf

802.1Qat
--------

*802.1Qat* defines a stream reservation protocol that provides end-to-end reservation of bandwidth across an AVB network. 

The 802.1Qat specification is available as a section in the `IEEE 802.1Q-2011 standard document`_.

IEC 61883-6
-----------

*IEC 61883-6* defines an audio data format that is contained in *IEEE P1722* streams. The XMOS AVB solution uses *IEC 61883-6* to convey audio sample streams.

The `IEC 61883-6:2005 standard document`_ is available for purchase from the IEC website.

.. _`IEC 61883-6:2005 standard document`: http://webstore.iec.ch/webstore/webstore.nsf/ArtNum_PK/46793


IEEE P1722
----------

*IEEE P1722* defines an encapsulation protocol to transport audio streams over Ethernet. It is complementary to the AVB standards and in particular allows timestamping of a stream based on the *802.1AS* global clock. 

The XMOS AVB solution handles both transmission and receipt of audio streams using *IEEE P1722*. In addition it can use the *802.1AS* timestamps to accurately recover the audio master clock from an input stream.

The `IEEE 1722-2011 standard document`_ is available for purchase from the IEEE website.

.. _`IEEE 1722-2011 standard document`: http://standards.ieee.org/findstds/standard/1722-2011.html

IEEE P1722.1
------------

*IEEE P1722.1* is a system control protocol, used for device discovery, connection management and enumeration and control of parameters exposed by the AVB endpoints.

The IEEE 1722.1 standard is currently in final draft and available to members of the `1722.1 Working Group`_

.. _`1722.1 Working Group`: http://grouper.ieee.org/groups/1722/1/AVB-DECC/IEEE-1722.1_Working_Group.html