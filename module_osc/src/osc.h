#ifndef _osc_h_
#define _osc_h_

#include <xccompat.h>
#include "xtcp_client.h"
#include "osc_types.h"

/** Initializes osc. 
 *
 * \param tcp_svr channel linking to the TCP server
 * \param ocs_port TCP port to run the OSC server on
 *  Must be called at beginning of program.
 *
 */
void osc_init(chanend tcp_svr, unsigned int osc_port);

/** Handle an OSC TCP event. 
 *
 * This routine will handle a TCP event contained in
 * connection conn. If the event is on the OSC listening port it will handle
 * the request.
 */
void osc_xtcp_handler(chanend tcp_svr, REFERENCE_PARAM(xtcp_connection_t,conn));


#ifndef __XC__
void osc_get(osc_node *n, int h[], osc_val a[]);
void osc_set(osc_node *n, int h[], osc_val a[]);
osc_node *osc_match(char *query);
#endif

#endif
