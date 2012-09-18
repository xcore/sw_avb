//=============================================================================
//  File Name: control_api.xc
//
//  (c) Copyright [2010] Attero Tech, LLC. All rights reserved
//
//  This source code is Attero Tech, LLC. proprietary and confidential
//      information.
//
//  Description:
//      This file contains the main control api thread and all hw_access
//      functions.
//
//      //ARF TODO add better description once we determine what all is going in
//        this file.
//
// Modification History:
//     $Id: control_api.xc,v 1.1 2010/11/12 21:00:41 afoster Exp $
//     $Log: control_api.xc,v $
//     Revision 1.1  2010/11/12 21:00:41  afoster
//     Initial
//
//
//=============================================================================
#include <stdio.h>
#include <xccompat.h>
#include <string.h>
#include <platform.h>
#include <xs1.h>
#include "xtcp_client.h"
#include "control_api_server.h"


//-----------------------------------------------------------------------------
// Global objects
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
void c_api_handle_udp_event(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t &conn, // reference to the connection
     unsigned int t           // event timestamp in units of core frequency
);
//-----------------------------------------------------------------------------
// Local definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local objects
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function: c_api_xtcp_handler
//
// Description:
//  This function is used to handle events from the xtcp connection. It will
//  access the local time in units of core frequency ticks and pass the
//  event down to the control api event handler.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_xtcp_handler(
    chanend tcp_svr,
    xtcp_connection_t &conn,
    timer tmr
)
{
  unsigned t;
  tmr :> t;
  c_api_handle_udp_event(tcp_svr, conn, t);
}// end <c_api_xtcp_handler>





