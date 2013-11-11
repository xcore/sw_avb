#ifndef __gptp_h__
#define __gptp_h__

#include <xccompat.h>

#define PTP_ADJUST_PREC 30

/** This type represents a timestamp in the gptp clock domain.
 *
 **/
typedef struct ptp_timestamp {
  unsigned int seconds[2];
  unsigned int nanoseconds;
} ptp_timestamp;

/**
 *  The ptp_ts field stores the seconds and nanoseconds fields separately
 *  so the nanoseconds field is always in the range 0-999999999
 */
struct ptp_time_info {
  unsigned int local_ts; /*!< A local timestamp based on the 100MHz
                              XCore reference clock */
  ptp_timestamp ptp_ts;  /*!< A PTP timestamp in the gptp clock domain
                           that matches the local timestamp */

  int ptp_adjust; /*!< The adjustment required to convert from
                       local time to PTP time */
  int inv_ptp_adjust; /*!< The adjustment required to convert from
                                PTP time to local time */
};


/** This type is used to relate local XCore time with gptp time.
 *  It can be retrieved from the PTP server using the ptp_get_time_info()
 *  function.
 **/
typedef struct ptp_time_info ptp_time_info;

/**
 *  The time stored in the PTP low and high words is the PTP time in
 *  nanoseconds.
 */
struct ptp_time_info_mod64 {
  unsigned int local_ts;
  unsigned int ptp_ts_hi;
  unsigned int ptp_ts_lo;
  int ptp_adjust;
  int inv_ptp_adjust;
};

/** This structure is used to relate local XCore time with the least
 *  significant 64 bits of gptp time. The 64 bits of time is the PTP
 *  time in nanoseconds from the epoch.
 *
 *  It can be retrieved from the PTP server using the ptp_get_time_info_mod64()
 *  function.
 **/
typedef struct ptp_time_info_mod64 ptp_time_info_mod64;

/** The type of a PTP server. Can be passed into the ptp_server() function.
 **/
enum ptp_server_type {
  PTP_GRANDMASTER_CAPABLE,
  PTP_SLAVE_ONLY
};

typedef enum ptp_port_role_t {
  PTP_MASTER,
  PTP_UNCERTAIN,
  PTP_SLAVE,
  PTP_DISABLED
} ptp_port_role_t;

typedef struct ptp_port_info_t {
  ptp_port_role_t role_state;
} ptp_port_info_t;

/** This function runs the PTP server. It takes one thread and runs
    indefinitely

    \param mac_rx       chanend connected to the ethernet server (receive)
    \param mac_tx       chanend connected to the ethernet server (transmit)
    \param client       an array of chanends to connect to clients
                        of the ptp server
    \param num_clients  The number of clients attached
    \param server_type The type of the server (``PTP_GRANDMASTER_CAPABLE``
                       or ``PTP_SLAVE_ONLY``)
 **/
void ptp_server(chanend mac_rx, chanend mac_tx,
                chanend ptp_clients[], int num_clients,
                enum ptp_server_type server_type);


// Synchronous PTP client functions
// --------------------------------

ptp_port_role_t ptp_get_state(chanend ptp_server);


/** Retrieve time information from the ptp server
 *
 *  This function gets an up-to-date structure of type `ptp_time_info` to use
 *  to convert local time to PTP time.
 *
 *  \param ptp_server chanend connected to the ptp_server
 *  \param info       structure to be filled with time information
 *
 **/
void ptp_get_propagation_delay(chanend ptp_server, unsigned *pdelay);

/** Retrieve port progatation delay from the ptp server
 *
 *
 *  \param ptp_server chanend connected to the ptp_server
 *  \param pdelay     unsigned int with delay in ns
 *
 **/
void ptp_get_time_info(chanend ptp_server,
                        REFERENCE_PARAM(ptp_time_info, info));
/** Retrieve time information from the ptp server
 *
 *  This function gets an up-to-date structure of type `ptp_time_info_mod64`
 *  to use to convert local time to ptp time (modulo 64 bits).
 *
 *  \param ptp_server chanend connected to the ptp_server
 *  \param info       structure to be filled with time information
 *
 **/
void ptp_get_time_info_mod64(NULLABLE_RESOURCE(chanend,ptp_server),
                              REFERENCE_PARAM(ptp_time_info_mod64, info));

// Asynchronous PTP client functions
// --------------------------------

/** This function requests a `ptp_time_info` structure from the
    PTP server. This is an asynchronous call so needs to be completed
    later with a call to ptp_get_requested_time_info().

    \param ptp_server chanend connecting to the ptp server

 **/
void ptp_request_time_info(chanend ptp_server);

/** This function receives a `ptp_time_info` structure from the
    PTP server. This completes an asynchronous transaction initiated with a call
    to ptp_request_time_info(). The function can be placed in a select case
    which will activate when the PTP server is ready to send.

    \param ptp_server      chanend connecting to the PTP server
    \param info            a reference parameter to be filled with the time
                           information structure
**/
#ifdef __XC__
#pragma select handler
#endif
void ptp_get_requested_time_info(chanend ptp_server,
                                  REFERENCE_PARAM(ptp_time_info, info));


/** This function requests a `ptp_time_info_mod64` structure from the
    PTP server. This is an asynchronous call so needs to be completed
    later with a call to ptp_get_requested_time_info_mod64().

    \param ptp_server chanend connecting to the PTP server

 **/
void ptp_request_time_info_mod64(chanend ptp_server);


/** This function receives a `ptp_time_info_mod64` structure from the
    PTP server. This completes an asynchronous transaction initiated with a call
    to ptp_request_time_info_mod64().
    The function can be placed in a select case
    which will activate when the PTP server is ready to send.

    \param ptp_server      chanend connecting to the PTP server
    \param info            a reference parameter to be filled with the time
                           information structure
**/
#ifdef __XC__
#pragma select handler
#endif
void ptp_get_requested_time_info_mod64(chanend ptp_server,
                                        REFERENCE_PARAM(ptp_time_info_mod64, info));

#ifdef __XC__
#pragma select handler
#endif
void ptp_get_requested_time_info_mod64_use_timer(chanend c,
                                                 REFERENCE_PARAM(ptp_time_info_mod64, info),
                                                 timer tmr);


/** Convert a timestamp from the local XCore timer to PTP time.
 *
 *  This function takes a 32-bit timestamp taken from an XCore timer and
 *  converts it to PTP time.
 *
 *  \param ptp_ts         the PTP timestamp structure to be filled with the
 *                        converted time
 *  \param local_ts       the local timestamp to be converted
 *  \param info           a time information structure retrieved from the ptp
 *                        server
 **/
void local_timestamp_to_ptp(REFERENCE_PARAM(ptp_timestamp, ptp_ts),
                            unsigned local_ts,
                            REFERENCE_PARAM(ptp_time_info, info));

/** Convert a timestamp from the local XCore timer to the least significant
 *  32 bits of PTP time.
 *
 *  This function takes a 32-bit timestamp taken from an XCore timer and
 *  converts it to the least significant 32 bits of global PTP time.
 *
 *  \param local_ts       the local timestamp to be converted
 *  \param info           a time information structure retrieved from the PTP
 *                        server
 *  \returns              the least significant 32-bits of ptp time in
 *                        nanoseconds
 **/
unsigned local_timestamp_to_ptp_mod32(unsigned local_ts,
                                      REFERENCE_PARAM(ptp_time_info_mod64, info));

/** Convert a PTP timestamp to a local XCore timestamp.
 *
 *  This function takes a PTP timestamp and converts it to a local
 *  32-bit timestamp that is related to the XCore timer.
 *
 *  \param ts             the PTP timestamp to convert
 *  \param info           a time information structure retrieved from the PTP
 *                        server.
 *  \returns              the local timestamp
 **/
unsigned ptp_timestamp_to_local(REFERENCE_PARAM(ptp_timestamp, ts),
                                REFERENCE_PARAM(ptp_time_info, info));

/** Convert a PTP timestamp to a local XCore timestamp.
 *
 *  This function takes a PTP timestamp and converts it to a local
 *  32-bit timestamp that is related to the XCore timer.
 *
 *  \param ts             the least significant 32 bits of a PTP timestamp to convert
 *  \param info           a time information structure retrieved from the PTP
 *                        server.
 *  \returns              the local timestamp
 **/
unsigned ptp_mod32_timestamp_to_local(unsigned ts, REFERENCE_PARAM(ptp_time_info_mod64, info));

/** Calculate an offset to a PTP timestamp.
 *
 *  This function adds and offset to a timestamp.
 *
 *  \param ptp_timestamp the timestamp to be offset; this argument is modified
 *                       by adding the offset
 *  \param offset        the offset to add in nanoseconds
 *
 */
void ptp_timestamp_offset(REFERENCE_PARAM(ptp_timestamp, ts), int offset);


void ptp_get_current_grandmaster(chanend ptp_server, unsigned char grandmaster[8]);


/** Initialize the inline ptp server.
 *
 *  \param mac_rx       chanend connected to the ethernet server (receive)
 *  \param mac_tx       chanend connected to the ethernet server (transmit)
 *  \param server_type The type of the server (``PTP_GRANDMASTER_CAPABLE``
 *                     or ``PTP_SLAVE_ONLY``)
 *
 *  This function initializes the ptp server when you want to use it inline
 *  combined with other event handling functions (i.e. share the resource in
 *  the ptp thread).
 *  It needs to be called in conjunction with do_ptp_server().
 *  Here is an example usage::
 *
 *     ptp_server_init(c_rx, c_tx, PTP_GRANDMASTER_CAPABLE);
 *     while (1) {
 *         select {
 *             do_ptp_server(c_tx, c_tx, ptp_client, num_clients);
 *             // Add your own cases here
 *         }
 *
 *     }
 *
 *  \sa do_ptp_server
 **/
void ptp_server_init(chanend mac_rx, chanend mac_tx,
                     enum ptp_server_type server_type,
                     timer ptp_timer,
                     REFERENCE_PARAM(int, ptp_timeout));


#ifdef __XC__
#pragma select handler
#endif
void ptp_recv_and_process_packet(chanend c_rx, chanend c_tx);
#ifdef __XC__
#pragma select handler
#endif
void ptp_process_client_request(chanend c, timer ptp_timer);
void ptp_periodic(chanend, unsigned);
#define PTP_PERIODIC_TIME (10000)  // 0.tfp1 milliseconds





#define do_ptp_server(c_rx, c_tx, client, num_clients, ptp_timer, ptp_timeout)      \
  case ptp_recv_and_process_packet(c_rx, c_tx): \
       break;                     \
 case (int i=0;i<num_clients;i++) ptp_process_client_request(client[i], ptp_timer): \
       break; \
  case ptp_timer when timerafter(ptp_timeout) :> void: \
       ptp_periodic(c_tx, ptp_timeout); \
       ptp_timeout += PTP_PERIODIC_TIME; \
       break




void ptp_output_test_clock(chanend ptp_link,
                           port test_clock_port,
                           int period);
#endif //__gptp_h__
