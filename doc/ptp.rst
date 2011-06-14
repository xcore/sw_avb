.. index:: ptp, 802.1as

Precise Timing Protocol Component
---------------------------------

The precise timing protocol component (PTP) provides a system with a
notion of global time on a network. The component supports the *AVB
802.1as* timing protocol. It allows synchronization of the
presentation and playback rate of media streams across a network.

.. only:: latex

 .. image:: images/ptp-crop.pdf
   :width: 70%
   :align: center

.. only:: html

 .. image:: images/ptp-crop.png
   :align: center

The timing component consists of two threads. It connects to the ethernet MAC component and provides channel ends for clients to query for timing information. The component interprets PTP packets from the MAC and maintains a notion of global time. The maintenance of global time requires no application interaction with the component.

The PTP component can be configured at runtime to be a *PTP grandmaster* or a *PTP slave*. If the component is configured as a grandmaster, it supplies a clock source to the network. If the network has several grandmasters, the potential grandmasters negotiate between themselves to select a single grandmaster. Once a single grandmaster is selected, all units on the network synchronize a global time from this source and the other grandmasters stop providing timing information. Depending on the intermediate network, this synchronization can be to sub-microsecond level resolution.

Client threads connect to the timing component via channels. The relationship between the local reference counter and global time is maintained across this channel, allowing a client to timestamp with a local timer very accurately and then convert it to global time, giving highly accurate global timestamps.

Client threads can communicate with the server using the API described
in Section :ref:`sec_ptp_api`.

 * The PTP system in the endpoint is self-configuring, it runs
   automatically and gives each endpoint an accurate notion of a global clock.
 * The global clock is *not* the same as the sample rate clock
   used to time audio (though it can be used to create the sample
   clock). An audio stream may be at a rate that is independent of the 
   PTP clock but will contain timestamps that use the global PTP clock
   domain as a reference domain.
