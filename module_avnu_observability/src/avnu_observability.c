#include "telnetd.h"
#include <print.h>
#include "xlog_client.h"
#include <string.h>
#include "gptp.h"
#include "avb.h"
#include "avb_internal.h"
#include "avb_util.h"
#include "misc_timer.h"
#include "swlock.h"
#include "xccompat.h"
#include "avnu_observability.h"
#include "xtcp_client.h"
static  ptp_time_info timeinfo;

// These are going to be "core local" - i.e. have different values on each
// core
static swlock_t chan_lock = INITIAL_SWLOCK_VALUE;
static chanend c_client = 0;
static int is_my_core = 0;
static int my_thread_id = -1;
///


static struct testpoint_info_t testpoint_info[] = TESTPOINT_INFO;

static char msg[160] = "AVnu xxxxxxxxxx.yyyyyyyyy aa:bb:cc:dd:ee:ff ";

static chanend tcp_svr=0;


typedef struct avnu_conn_t {
  int active;
  int dcount[AVNU_NUM_TESTPOINTS];
} avnu_conn_t;


static avnu_conn_t conns[NUM_TELNETD_CONNECTIONS];


void init_avnu_trace_for_core(chanend c) {
  swlock_acquire(&chan_lock);
  c_client = c;
  swlock_release(&chan_lock);
}


static void avnu_do_msg(int testpoint, chanend tcp_svr, int valid_ts, ptp_timestamp *ts) {
  int t;
  if (!valid_ts) {
    t = get_local_time();
    local_timestamp_to_ptp(ts, t, &timeinfo);
  }
  
  avb_itoa(ts->seconds[0],&msg[5],10,10);
  avb_itoa(ts->nanoseconds,&msg[16],10,9);
  
  
  if (testpoint_info[testpoint].console_out) {
    printstrln(msg);
  }

  for (int i=0;i<NUM_TELNETD_CONNECTIONS;i++) {
    if (conns[i].active && conns[i].dcount[testpoint]) {      
      telnetd_send_line(tcp_svr, i, msg);
      conns[i].dcount[testpoint]--;
    }
  }
}

void avnu_log_xc(chanend c,
                 enum avnu_testpoint_type_t testpoint,
                 ptp_timestamp *ts,
                 char msg[]);

void avnu_log(enum avnu_testpoint_type_t testpoint,
              ptp_timestamp *ts,
              char m[])
{
  if (is_my_core && get_thread_id() == my_thread_id) {
    ptp_timestamp ts0;
    int namelen;
    namelen = strlen(testpoint_info[testpoint].name);
    memcpy(&msg[44], testpoint_info[testpoint].name, namelen);
    msg[44+namelen] = ' ';
    memcpy(&msg[44+namelen+1], m, strlen(m)+1);
    if (ts != NULL)
      memcpy(&ts0, ts, sizeof(ptp_timestamp));
    avnu_do_msg(testpoint, tcp_svr, ts != NULL, &ts0);                              
  }
  else {
    swlock_acquire(&chan_lock);
    if (c_client) {
      avnu_log_xc(c_client, testpoint, ts, m);
    }
    swlock_release(&chan_lock);
  }
}             

void avnu_update_ptp_timeinfo_xc(chanend c);

void avnu_update_ptp_timeinfo() 
{
  swlock_acquire(&chan_lock);
  if (c_client) {
    avnu_update_ptp_timeinfo_xc(c_client);
  }
  swlock_release(&chan_lock);
}

char *skip_whitespace(char *line) {
  while (*line==' ')
    line++;
  return line;
}

#define UNRECOGNIZED_TESTPOINT -1
#define ALL_TESTPOINTS -2

char * parse_testpoint(char *line, int *testpoint)
{
  if (strncmp(line, "ALL", 3)==0) {
    *testpoint = ALL_TESTPOINTS;
    return (line+3);
  }
  for (int i=0;i<AVNU_NUM_TESTPOINTS;i++) {
    int len = strlen(testpoint_info[i].name);
    if (strncmp(line, testpoint_info[i].name, len)==0) {
      *testpoint = i;
      return (line + len);
    }
  }
  *testpoint = UNRECOGNIZED_TESTPOINT;
  return line;
}

int avnu_parse_command(int id,
                       char line[])
{
  int testpoint;
  if (strncmp(line, "avnu", 4) != 0)
    return 0;

  line += 4;

  line = skip_whitespace(line);
  
  if (strncmp(line,"on",2)==0) {
    int count=1;
    line += 2;
    line = skip_whitespace(line);
    line = parse_testpoint(line, &testpoint); 

    if (testpoint == UNRECOGNIZED_TESTPOINT)
      return 0;

    line = skip_whitespace(line);

    line = avb_atoi(line, &count);
    printintln(count);
    if (!count)
      count = 1;

    if (testpoint == ALL_TESTPOINTS) {
      for (int i=0;i<AVNU_NUM_TESTPOINTS;i++) {
        conns[id].dcount[i] = count;
      }
    }
    else {
      conns[id].dcount[testpoint] = count;
    }         

  }
  else if (strncmp(line,"off",3)==0) {
    line += 3;
    line = skip_whitespace(line);
    line = parse_testpoint(line, &testpoint); 
    if (testpoint == ALL_TESTPOINTS) {
      for (int i=0;i<AVNU_NUM_TESTPOINTS;i++) {
        conns[id].dcount[i] = 0;
      }
    }
    else {
      conns[id].dcount[testpoint] = 0;
    }         
  }
  else if (strncmp(line,"for",3)==0) {
    line +=3;
    line = skip_whitespace(line);
    line = parse_testpoint(line, &testpoint);
    if (testpoint == UNRECOGNIZED_TESTPOINT)
      return 0;
  }
  else
    return 0;

  return 1;
}


void telnetd_recv_line(chanend tcp_svr,
                       int id,
                       char line[],
                       int len)
{
  int res=1;

  if (len != 0) 
    res = avnu_parse_command(id, line);

  if (!res)
    telnetd_send_line(tcp_svr, id, "Unrecognized command");    
  telnetd_send(tcp_svr, id, "> ");
}
                       
void telnetd_sent_line(chanend tcp_svr, int id)
{
  // do nothing
  return;
}

void telnetd_new_connection(chanend tcp_svr, int id)
{
  telnetd_send_line(tcp_svr, id, "*****************************************");
  telnetd_send_line(tcp_svr, id, "XMOS AVNu testpoint observability server.");
  telnetd_send_line(tcp_svr, id, "*****************************************");
  telnetd_send(tcp_svr, id, "> ");
  conns[id].active = 1;
}

void telnetd_connection_closed(chanend tcp_svr, int id)
{
  conns[id].active = 0;
}




void avnu_observability_init(chanend c_tcp)
{
  unsigned char mac_addr[6];
  xtcp_get_mac_address(c_tcp, mac_addr);
  tcp_svr = c_tcp;
  is_my_core = 1;
  my_thread_id = get_thread_id();
  int j=26;
  for (int i=0;i<6;i++) {
    avb_itoa(mac_addr[i],&msg[j],16,2);
    j+=3;
  }
  return;

}


void avnu_get_log_hdr(chanend c,
                      enum avnu_testpoint_type_t *testpoint,
                      ptp_timestamp *ts,
                      int *valid_ts);

void avnu_get_log_msg(chanend c,
                      char msg[]);



int avnu_msg_type(chanend);
void avnu_receive_timeinfo_update(chanend c, ptp_time_info *timeinfo);

void avnu_observability_handler(chanend tcp_svr, chanend c)
{
  ptp_timestamp ts;
  int valid_ts;
  enum avnu_testpoint_type_t testpoint;
  int namelen;
  int typ;

  typ = avnu_msg_type(c);

  switch (typ) 
    {
    case 0:
      avnu_get_log_hdr(c, &testpoint, &ts, &valid_ts);

      namelen = strlen(testpoint_info[testpoint].name);      
      memcpy(&msg[44], &testpoint_info[testpoint].name, namelen);

      msg[44+namelen] = ' ';
      avnu_get_log_msg(c, &msg[44+namelen+1]);
      
      avnu_do_msg(testpoint, tcp_svr, valid_ts, &ts);
      break;
    case 1:
      avnu_receive_timeinfo_update(c, &timeinfo);
      break;
    }

  return;
}

