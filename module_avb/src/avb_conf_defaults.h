/*****************************************************
 * This file documents the options that can be altered in avb_conf.h 
 * along with the default values.
 * The majority of these options can be used to tune memory usage.
 *
 */ 

/*****************************************************/
/*   Ethernet Options                                */
/*****************************************************/

/** The maximum ethernet packet size that the MAC will accept.
 */
#define MAX_ETHERNET_PACKET_SIZE (1518)

/** The number of packet buffers available for receive in the MAC.
 */
#define NUM_MII_RX_BUF 16

/** The number of packet buffers available for transmit in the MAC.
 */
#define NUM_MII_TX_BUF 4

/** The maximum number of clients that can be attached to the MAC.
 */
#define MAX_ETHERNET_CLIENTS   4

/*****************************************************/
/*   General Audio Stream Options                    */
/*****************************************************/

/** Maximum number of channels in any IEEE 1722 stream (incoming or outgoing).
 */ 
#define MAX_CHANNELS_PER_AVB_STREAM 16

/*****************************************************/
/*   Talker Options                                  */
/*****************************************************/

/** Maximum number of local talker streams per XCore. */
#define MAX_LOCAL_TALKER_STREAMS_PER_CORE 4

/** Buffer size for each local talker stream (in samples) */
#define LOCAL_TALKER_STREAM_SAMPLE_FIFO_SIZE  64

/** Maximum number of local talker streams that can be combined into 
 *  an IEEE 1722 AVB stream. 
 */
#define MAX_LOCAL_TALKER_STREAMS_PER_AVB_STREAM 8

/** Maximum number of IEEE 1722 AVB streams a talker component 
 *  can transmit.
 */
#define MAX_AVB_STREAMS_PER_TALKER 8

/*****************************************************/
/*   Listener Options                                */
/*****************************************************/

/** Maximum number of incoming IEEE 1722 streams into the system.
 */
#define MAX_INCOMING_AVB_STREAMS 16

/** Maximum number of channels that can be attached to the 1722 router 
 *  component. 
 */
#define MAX_AVB_1722_ROUTER_LINKS 4

/** Buffer size of router packet buffer (in words). */
#define AVB_1722_ROUTER_BUFFER_SIZE 2048

/** Table size of router table.
 *  This should be about 3-4 times the total number fo incoming streams */
#define AVB_1722_ROUTER_TABLE_SIZE 256


/** Maximum number of local listener streams in the whole system.
 */
#define MAX_TOTAL_LOCAL_LISTENER_STREAMS 16

/** Maximum number of local listener streams on each Xcore */
#define MAX_LOCAL_LISTENER_STREAMS_PER_CORE 4

/** Sample buffer size for each local listener stream (in samples).
 *  This should be enough to hold  at least 2.5ms of audio 
 */
#define LOCAL_LISTENER_STREAM_SAMPLE_FIFO_SIZE  (512)

/** Maximum number of IEEE 1722 streams each listener component can
 *  receive.
 */
#define MAX_AVB_STREAMS_PER_LISTENER 12

/** Maximum number of local listener streams that an IEEE 1722 
 *  stream can be split into.
 */
#define MAX_LOCAL_LISTENER_STREAMS_PER_AVB_STREAM (8)

/*****************************************************/
/*   PTP Options                                     */
/*****************************************************/

/** PTP packet buffer size (in bytes).
 */
#define PTP_RX_FIFO_SIZE 8192

/*****************************************************/
/*   Media clock options                             */
/*****************************************************/

/** The maximum number of media clocks in the system
 */
#define MAX_NUM_MEDIA_CLOCKS 2

/** The maximum number of clk_ctl clients connected to the
 *  media clock server.
 */
#define MAX_CLK_CTL_CLIENTS 8

