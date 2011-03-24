#ifndef __avnu_observability_h__
#define __avnu_observability_h__
#include "xccompat.h"
#include "gptp.h"



enum avnu_testpoint_type_t {
  AVNU_TESTPOINT_GM,
  AVNU_TESTPOINT_LM,
  AVNU_TESTPOINT_SYNC,
  AVNU_TESTPOINT_ANRX,
  AVNU_TESTPOINT_ANTX,
  AVNU_TESTPOINT_TA,
  AVNU_TESTPOINT_TAF,
  AVNU_TESTPOINT_LR,
  AVNU_TESTPOINT_LRF,
  AVNU_TESTPOINT_LAF,
  AVNU_TESTPOINT_PDELAY,
  AVNU_TESTPOINT_RR,
  AVNU_TESTPOINT_TEST,
  // Always have this as the last enum
  AVNU_NUM_TESTPOINTS
};

#define MAX_TESTPOINT_NAME_SIZE 6

struct testpoint_info_t {
  int console_out;
  char name[MAX_TESTPOINT_NAME_SIZE];
};

// The following define should match the above
#define TESTPOINT_INFO { \
  {1,"GM"},             \
  {0,"LM"},          \
  {0,"SYNC"},        \
  {0,"ANRX"},        \
  {0,"ANTX"},        \
  {0,"TA"},          \
  {0,"TAF"},         \
  {0,"LR"},          \
  {0,"LRF"},         \
  {0,"LAF"},         \
  {1,"PDELAY"},      \
  {0,"RR"},          \
  {0,"TEST"},        \
} \

void avnu_observability_init(chanend tcp_svr);


void init_avnu_trace_for_core(chanend c);

#ifdef __XC__
void avnu_log(enum avnu_testpoint_type_t testpoint,
              ptp_timestamp &?ts,
              char msg[]);
#else
void avnu_log(enum avnu_testpoint_type_t testpoint,
              ptp_timestamp *ts,
              char msg[]);
#endif

void avnu_observability_handler(chanend tcp_svr, chanend c);

#endif // __avnu_observability_h__
