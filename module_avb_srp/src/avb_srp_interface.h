#ifndef _avb_srp_interface_h_
#define _avb_srp_interface_h_
#include <xccompat.h>
#include "xc2compat.h"
#include "avb_srp_pdu.h"
#include "avb_1722_talker.h"
#include "avb_mrp.h"
#include "avb_control_types.h"
#include "avb_stream.h"
#include "avb_api.h"

#ifdef __XC__
interface srp_interface {
  void register_stream_request(avb_srp_info_t stream_info);
  void deregister_stream_request(unsigned stream_id[2]);
  void register_attach_request(unsigned stream_id[2]);
  void deregister_attach_request(unsigned stream_id[2]);
};

#endif


#endif // _avb_srp_interface_h_
