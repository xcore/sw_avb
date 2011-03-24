#ifndef _CONTROL_API_SERVER_H_
#define _CONTROL_API_SERVER_H_
//=============================================================================
//  File Name: control_api_server.h
//
//  (c) Copyright [2010] Attero Tech, LLC. All rights reserved
//
//  This source code is Attero Tech, LLC. proprietary and confidential
//      information.
//
//  Description:
//      This file contains the public prototypes and structure defintions
//      for implementing the control_api_server.
//
// Modification History:
//     $Id: control_api_server.h,v 1.1 2010/11/12 21:00:41 afoster Exp $
//     $Log: control_api_server.h,v $
//     Revision 1.1  2010/11/12 21:00:41  afoster
//     Initial
//
//
//=============================================================================

#define ATTERO_CFG_PORT (40404)

void c_api_xtcp_handler(chanend tcp_svr,
                       REFERENCE_PARAM(xtcp_connection_t, conn));

void c_api_server_init(
     chanend tcp_svr // chan for the xtcp server
 );


#endif
