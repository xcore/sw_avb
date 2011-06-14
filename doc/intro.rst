XMOS AVB Specification
======================

 +-------------------------------------------------------------------+
 |                        **Functionality**                          |
 +-------------------------------------------------------------------+
 |  Provides hardware interfaces, audio transport,                   |
 |  precise timing protocol clock synchronization and media clock    |
 |  recovery to streamed audio over ethernet.                        |
 +-------------------------------------------------------------------+
 |                       **Supported Standards**                     |
 +---------------------------------+---------------------------------+
 | | Ethernet                      | | IEEE 802.3 (via MII)          |
 | | AVB QoS                       | | IEEE 802.1Qav, 802.1Qat       |
 | | Precise Timing Protocol       | | IEEE 802.1as                  |
 | | AVB Audio Over Ethernet       | | IEEE P1722                    |
 | | Audio Streaming               | | IEC 61883-6                   |
 | | Control Protocol              | | IEEE P1722.1                  |
 +---------------------------------+---------------------------------+
 |                       **Supported Devices**                       |
 +---------------------------------+---------------------------------+
 | XMOS Devices                    | | XS1-G4                        |
 |                                 | | XS1-L2                        |
 +---------------------------------+---------------------------------+
 |                       **Requirements**                            |
 +---------------------------------+---------------------------------+
 | Development Tools               | XMOS Desktop Tools v11.2 or     |
 |                                 | later                           |
 +---------------------------------+---------------------------------+
 | Ethernet                        | | 1 x MII compatible 100Mbit PHY|  
 +---------------------------------+---------------------------------+ 
 | Audio                           | | Audio input/output device     |
 |                                 |   (e.g. ADC/DAC audio CODEC)    |
 |                                 | | PLL/Frequency synthesizer     |
 |                                 |   chip to generate CODEC system |
 |                                 |   clock                         |
 +---------------------------------+---------------------------------+ 
 | Boot/Storage                    | Compatible SPI Flash Device     |
 +---------------------------------+---------------------------------+
 |                       **Licensing and Support**                   |
 +-------------------------------------------------------------------+
 | | Reference code provided without charge under license from XMOS. |
 | | Contact support@xmos.com for details.                           |
 | | Reference code is maintained by XMOS Limited.                   |
 +-------------------------------------------------------------------+

