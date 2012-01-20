// Copyright (c) 2011, XMOS Ltd, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>


#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include "miiDriver.h"
#include "mii.h"
#include "miiClient.h"

enum {
    THREAD_NONE = -1,
    THREAD_REST = 2,
    THREAD_PTP = 0,
    THREAD_AUDIO = 1,
};

static int calcDestinationThread(int addr) {
    int b3, result;
    asm("ldw %0, %1[3]" : "=r" (b3) : "r" (addr));
    if ((b3 & 0xffff) == 0x0081) {
        asm("ldw %0, %1[4]" : "=r" (b3) : "r" (addr));
    }
   

    switch (b3 & 0xFFFF) {
    case 0xF788:
        result = THREAD_PTP;
        break;
    case 0xEA22:
    case 0xF588:
    case 0xF688:
        result = THREAD_REST;
        break;
//    case 0x0608:
//    case 0x0008:
//        result = MAC_FILTER_ARPIP;
//        break;
    case 0xF022:
        result = ((b3 >> 23) & 1) ? THREAD_REST : THREAD_AUDIO;
        break;
    default:
        result = THREAD_NONE;
        break;
    }

    return result;
}
#define mask 3

#pragma unsafe arrays
static void theServer(chanend cIn, chanend cOut, chanend cNotifications, chanend appIn[3], chanend appOut[3]) {
    int outBytes;
    int b[3200];
    int txbuf[400];
    int timeRequired, t;
    int dest;
    int count, count2;
    struct miiData miiData;

    struct {
        struct {
            int addr;
            int nBytes;
            int time;
        } elements[mask+1];
        int len, rd, wr;
    } packetStore[3];
    
        count = 0; count2 = 0;
    
    for (int i=0; i < 3; i++)
    {
        packetStore[i].rd = 0;
        packetStore[i].wr = 0;
        packetStore[i].len = 0;
    }

    miiBufferInit(miiData, cIn, cNotifications, b, 3200);
    miiOutInit(cOut);
    
    while (1) {
        select {
        case inuchar_byref(cNotifications, miiData.notifySeen):
            break;
        case (int i = 0; i < 3; i++) 
            packetStore[i].len != 0 => appIn[i] :> int _:
            appIn[i] <: packetStore[i].elements[packetStore[i].rd].nBytes;
            for(int j = 0; j < ((packetStore[i].elements[packetStore[i].rd].nBytes + 3) >>2); j++) {
                int val;
                asm("ldw %0, %1[%2]" : "=r" (val) : "r" (packetStore[i].elements[packetStore[i].rd].addr) , "r" (j));
                appIn[i] <: val;
            }
            appIn[i] <: packetStore[i].elements[packetStore[i].rd].time;
            // printintln(packetStore[i].elements[packetStore[i].rd].time);
            miiFreeInBuffer(packetStore[i].elements[packetStore[i].rd].addr);
            miiRestartBuffer();
                packetStore[i].len--;
                packetStore[i].rd = (packetStore[i].rd + 1)& mask;
                if (packetStore[i].len != 0) {
                    outuint(appIn[i], 0);
                }
            break;
        case (int i = 0; i < 3; i++) 
            appOut[i] :> outBytes:
            appOut[i] :> timeRequired;

            for(int j = 0; j < ((outBytes + 3) >>2); j++) {
                appOut[i] :> txbuf[j];
            }
            t = miiOutPacket(cOut, txbuf, 0, outBytes);
            if (timeRequired) {
                appOut[i] <: t;
            }
            miiOutPacketDone(cOut);
            break;
        }
        while (1) {
            int a, n, dest, t;
            {a,n,t} = miiGetInBuffer(miiData);
            if (a == 0) {
                break; // no packets available.
            }
            dest = calcDestinationThread(a);
            if (dest == THREAD_NONE) {
                miiFreeInBuffer(miiData, a);
                miiRestartBuffer(miiData);
                continue;
            }
            if (packetStore[dest].len == mask+1) {
                miiFreeInBuffer(packetStore[dest].elements[packetStore[dest].rd].addr);
                packetStore[dest].len--;
                packetStore[dest].rd = (packetStore[dest].rd + 1)& mask;
                miiRestartBuffer(miiData);
            }
            else 
            {
                if (packetStore[dest].len == 0) {
                    outuint(appIn[dest], 0);
                }
            }
            
            packetStore[dest].elements[packetStore[dest].wr].addr = a;
            packetStore[dest].elements[packetStore[dest].wr].nBytes = n;
            packetStore[dest].elements[packetStore[dest].wr].time = t;
                packetStore[dest].len++;
                packetStore[dest].wr = (packetStore[dest].wr + 1)& mask;
                miiRestartBuffer();

        }
    } 
}

void miiAVBListenerServer(clock clk_smi, out port ?p_mii_resetn, smi_interface_t &smi,
                            mii_interface_t &m, chanend appIn[3], chanend appOut[3], chanend ?server) {
    chan cIn, cOut;
    chan notifications;
    par {
    	{
    	miiInitialise(clk_smi, p_mii_resetn, smi, m);
    	miiDriver(m, cIn, cOut);
    	}
        theServer(cIn, cOut, notifications, appIn, appOut);
    }
}
