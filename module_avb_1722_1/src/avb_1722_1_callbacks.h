#ifndef _avb_1722_1_callbacks_h_
#define _avb_1722_1_callbacks_h_

#ifdef __XC__
interface avb_1722_1_control_callbacks {
  
  unsigned char get_control_value(unsigned short control_type,
                                  unsigned short control_index,
                                  unsigned short &values_length,
                                  unsigned char values[508]);

  unsigned char set_control_value(unsigned short control_type,
                                  unsigned short control_index,
                                  unsigned short values_length,
                                  unsigned char values[508]);

};
#endif

#endif // _api_h_
