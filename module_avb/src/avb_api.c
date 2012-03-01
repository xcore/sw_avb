#ifndef _avb_api_h_
#define _avb_api_h_
#include <xccompat.h>
#include "avb_control_types.h"

#ifdef __XC__
#define REFERENCE_TO
#else
#define REFERENCE_TO &
#endif

#ifndef __OSC_IMPL

int get_osc_version(char a0[]);
int get_osc_type_reports(char a0[]);
int get_osc_type_accepts(char a0[]);
int getset_avb_source_presentation(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_source_presentation(int h0,int a0)
{return getset_avb_source_presentation(1, h0,REFERENCE_TO a0);}

int get_avb_source_presentation(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_source_presentation(0, h0,a0);}

int getset_avb_source_map(int set, int h0,int a0[], REFERENCE_PARAM(int, a0_len));

int set_avb_source_map(int h0,int a0[], int a0_len)
{return getset_avb_source_map(1, h0,a0, REFERENCE_TO a0_len);}

int get_avb_source_map(int h0,int a0[], REFERENCE_PARAM(int, a0_len))
 {return getset_avb_source_map(0, h0,a0, a0_len);}

int getset_avb_source_dest(int set, int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len));

int set_avb_source_dest(int h0,unsigned char a0[], int a0_len)
{return getset_avb_source_dest(1, h0,a0, REFERENCE_TO a0_len);}

int get_avb_source_dest(int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len))
 {return getset_avb_source_dest(0, h0,a0, a0_len);}

int getset_avb_source_format(int set, int h0,REFERENCE_PARAM(enum avb_source_format_t, a0),REFERENCE_PARAM(int, a1));

int set_avb_source_format(int h0,enum avb_source_format_t a0,int a1)
{return getset_avb_source_format(1, h0,REFERENCE_TO a0,REFERENCE_TO a1);}

int get_avb_source_format(int h0,REFERENCE_PARAM(enum avb_source_format_t, a0),REFERENCE_PARAM(int, a1))
 {return getset_avb_source_format(0, h0,a0,a1);}

int getset_avb_source_channels(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_source_channels(int h0,int a0)
{return getset_avb_source_channels(1, h0,REFERENCE_TO a0);}

int get_avb_source_channels(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_source_channels(0, h0,a0);}

int getset_avb_source_sync(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_source_sync(int h0,int a0)
{return getset_avb_source_sync(1, h0,REFERENCE_TO a0);}

int get_avb_source_sync(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_source_sync(0, h0,a0);}

int getset_avb_source_name(int set, int h0,char a0[]);

int set_avb_source_name(int h0,char a0[])
{return getset_avb_source_name(1, h0,a0);}

int get_avb_source_name(int h0,char a0[])
 {return getset_avb_source_name(0, h0,a0);}

int get_avb_source_id(int h0,unsigned int a0[2]);
int getset_avb_source_vlan(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_source_vlan(int h0,int a0)
{return getset_avb_source_vlan(1, h0,REFERENCE_TO a0);}

int get_avb_source_vlan(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_source_vlan(0, h0,a0);}

int getset_avb_source_state(int set, int h0,REFERENCE_PARAM(enum avb_source_state_t, a0));

int set_avb_source_state(int h0,enum avb_source_state_t a0)
{return getset_avb_source_state(1, h0,REFERENCE_TO a0);}

int get_avb_source_state(int h0,REFERENCE_PARAM(enum avb_source_state_t, a0))
 {return getset_avb_source_state(0, h0,a0);}

int get_avb_sources(REFERENCE_PARAM(int, a0));
int get_avb_sinks(REFERENCE_PARAM(int, a0));
int get_avb_ptp_gm(unsigned char a0[], REFERENCE_PARAM(int, a0_len));
int get_avb_ptp_ports(REFERENCE_PARAM(int, a0));
int get_avb_ptp_rateratio(REFERENCE_PARAM(int, a0));
int get_avb_ptp_port_pdelay(int h0,REFERENCE_PARAM(int, a0));
int getset_avb_sink_channels(int set, int h0,REFERENCE_PARAM(int, a0));


int set_avb_sink_channels(int h0,int a0)
{return getset_avb_sink_channels(1, h0,REFERENCE_TO a0);}

int get_avb_sink_channels(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_sink_channels(0, h0,a0);}

int getset_avb_sink_sync(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_sink_sync(int h0,int a0)
{return getset_avb_sink_sync(1, h0,REFERENCE_TO a0);}

int get_avb_sink_sync(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_sink_sync(0, h0,a0);}

int getset_avb_sink_name(int set, int h0,char a0[]);

int set_avb_sink_name(int h0,char a0[])
{return getset_avb_sink_name(1, h0,a0);}

int get_avb_sink_name(int h0,char a0[])
 {return getset_avb_sink_name(0, h0,a0);}

int getset_avb_sink_vlan(int set, int h0,REFERENCE_PARAM(int, a0));

int set_avb_sink_vlan(int h0,int a0)
{return getset_avb_sink_vlan(1, h0,REFERENCE_TO a0);}

int get_avb_sink_vlan(int h0,REFERENCE_PARAM(int, a0))
 {return getset_avb_sink_vlan(0, h0,a0);}

int getset_avb_sink_state(int set, int h0,REFERENCE_PARAM(enum avb_sink_state_t, a0));

int set_avb_sink_state(int h0,enum avb_sink_state_t a0)
{return getset_avb_sink_state(1, h0,REFERENCE_TO a0);}

int get_avb_sink_state(int h0,REFERENCE_PARAM(enum avb_sink_state_t, a0))
 {return getset_avb_sink_state(0, h0,a0);}

int getset_avb_sink_map(int set, int h0,int a0[], REFERENCE_PARAM(int, a0_len));

int set_avb_sink_map(int h0,int a0[], int a0_len)
{return getset_avb_sink_map(1, h0,a0, REFERENCE_TO a0_len);}

int get_avb_sink_map(int h0,int a0[], REFERENCE_PARAM(int, a0_len))
 {return getset_avb_sink_map(0, h0,a0, a0_len);}

int getset_avb_sink_id(int set, int h0,unsigned int a0[2]);

int set_avb_sink_id(int h0,unsigned int a0[2])
{return getset_avb_sink_id(1, h0,a0);}

int get_avb_sink_id(int h0,unsigned int a0[2])
 {return getset_avb_sink_id(0, h0,a0);}

int getset_avb_sink_addr(int set, int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len));

int set_avb_sink_addr(int h0,unsigned char a0[], int a0_len)
{return getset_avb_sink_addr(1, h0,a0, REFERENCE_TO a0_len);}

int get_avb_sink_addr(int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len))
 {return getset_avb_sink_addr(0, h0,a0, a0_len);}

int getset_media_out_name(int set, int h0,char a0[]);

int set_media_out_name(int h0,char a0[])
{return getset_media_out_name(1, h0,a0);}

int get_media_out_name(int h0,char a0[])
 {return getset_media_out_name(0, h0,a0);}

int get_media_out_type(int h0,char a0[]);
int get_media_outs(REFERENCE_PARAM(int, a0));
int get_media_ins(REFERENCE_PARAM(int, a0));
int getset_media_in_name(int set, int h0,char a0[]);

int set_media_in_name(int h0,char a0[])
{return getset_media_in_name(1, h0,a0);}

int get_media_in_name(int h0,char a0[])
 {return getset_media_in_name(0, h0,a0);}

int get_media_in_type(int h0,char a0[]);
int getset_device_media_clock_source(int set, int h0,REFERENCE_PARAM(int, a0));

int set_device_media_clock_source(int h0,int a0)
{return getset_device_media_clock_source(1, h0,REFERENCE_TO a0);}

int get_device_media_clock_source(int h0,REFERENCE_PARAM(int, a0))
 {return getset_device_media_clock_source(0, h0,a0);}

int getset_device_media_clock_rate(int set, int h0,REFERENCE_PARAM(int, a0));

int set_device_media_clock_rate(int h0,int a0)
{return getset_device_media_clock_rate(1, h0,REFERENCE_TO a0);}

int get_device_media_clock_rate(int h0,REFERENCE_PARAM(int, a0))
 {return getset_device_media_clock_rate(0, h0,a0);}

int getset_device_media_clock_type(int set, int h0,REFERENCE_PARAM(enum device_media_clock_type_t, a0));

int set_device_media_clock_type(int h0,enum device_media_clock_type_t a0)
{return getset_device_media_clock_type(1, h0,REFERENCE_TO a0);}

int get_device_media_clock_type(int h0,REFERENCE_PARAM(enum device_media_clock_type_t, a0))
 {return getset_device_media_clock_type(0, h0,a0);}

int getset_device_media_clock_state(int set, int h0,REFERENCE_PARAM(enum device_media_clock_state_t, a0));

int set_device_media_clock_state(int h0,enum device_media_clock_state_t a0)
{return getset_device_media_clock_state(1, h0,REFERENCE_TO a0);}

int get_device_media_clock_state(int h0,REFERENCE_PARAM(enum device_media_clock_state_t, a0))
 {return getset_device_media_clock_state(0, h0,a0);}

int get_device_system(char a0[]);
int get_device_identity_serial(char a0[]);
int get_device_identity_version(char a0[]);
int get_device_identity_vendor_id(char a0[]);
int get_device_identity_product(char a0[]);
int get_device_identity_vendor(char a0[]);
int get_device_name(char a0[]);
int get_device_media_clocks(REFERENCE_PARAM(int, a0));

#endif // __OSC_IMPL


#endif // _avb_api_h_
