#ifndef _CONTROL_API_DEFS_H_
#define _CONTROL_API_DEFS_H_
//=============================================================================
//  File Name: control_apidefs.h
//
//  (c) Copyright [2010] Attero Tech, LLC. All rights reserved
//
//  This source code is Attero Tech, LLC. proprietary and confidential
//      information.
//
//  Description:
//      This file contains all shared c_api defintions used in the
//      application.
//
// Modification History:
//     $Id: control_api_defs.h,v 1.1 2010/11/12 21:00:41 afoster Exp $
//     $Log: control_api_defs.h,v $
//     Revision 1.1  2010/11/12 21:00:41  afoster
//     Initial
//
//
//=============================================================================

#ifdef __cplusplus
extern "C"
{
#endif
//=============================================================================
//
// The following values define the 4-bit field for the hw_shared_out 
// argument to several of the control APIs.
//
//=============================================================================
#define HW_CONT_OUT_MUTEn        0x01  // MUTEn      pin 0
#define HW_CONT_OUT_LED_REMOTE   0x02  // LED_REMOTE pin 1
#define HW_CONT_OUT_RESERVED_2   0x04  // RESERVED_2 pin 2
#define HW_CONT_OUT_RESERVED_3   0x08  // RESERVED_3 pin 3

#define HW_CONT_SHARED_OUT_PINS  0x03

//=============================================================================
//
// The following values define the 4-bit field for the hw_port_leds 
// argument to several of the control APIs.
//
//=============================================================================                                  
#define HW_CONT_PORT_LED_1_2     0x01  // LED_1_2    pin 0
#define HW_CONT_PORT_LED_3_4     0x02  // LED_3_4    pin 1
#define HW_CONT_PORT_LED_5_6     0x04  // LED_5_6    pin 2
#define HW_CONT_PORT_LED_7_8     0x08  // LED_7_8    pin 3                                    

#define HW_CONT_PORT_LED_PINS    0x0F 
//=============================================================================
//
// The following values define the 4-bit field for the hw_shared_in argument
// to several of the control APIs.
//
//=============================================================================
#define HW_CONT_IN_STREAM_SEL    0x01  // STREAM_SEL    pin 0
#define HW_CONT_IN_REMOTE        0x02  // REMOTE        pin 1
#define HW_CONT_IN_CHANNEL_SEL   0x04  // CHANNEL_SEL   pin 2
#define HW_CONT_IN_RESERVED_3    0x08  // RESERVED_3    pin 3

#define HW_CONT_IN_PINS          0x07

#ifdef __cplusplus
}
#endif

#endif

