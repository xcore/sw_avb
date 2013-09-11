#ifndef __media_clock_client_h__
#define __media_clock_client_h__

#define CLK_CTL_SET_RATE 0x1
#define CLK_CTL_STOP     0x2

#define WC_FRACTIONAL_BITS 16

/** Commands that can be sent to the buffer */
#define BUF_CTL_GET_INCOMING_COUNT 1
#define BUF_CTL_GET_OUTGOING_COUNT 2
#define BUF_CTL_GET_COUNTS 3
#define BUF_CTL_RESET_COUNTS 4
#define BUF_CTL_RESET_LOCKED 5
#define BUF_CTL_RESET_UNLOCKED 6
#define BUF_CTL_RESET_COUNTS_NO_NOTIFY 7
#define BUF_CTL_GET_INCOMING_TIMESTAMP 8
#define BUF_CTL_GET_OUTGOING_TIMESTAMP 9
#define BUF_CTL_SYNC 10
#define BUF_CTL_FILL_LEVEL 11
#define BUF_CTL_REQUEST_INFO 12
#define BUF_CTL_ADJUST_FILL 13
#define BUF_CTL_ACK 14
#define BUF_CTL_GOT_INFO 15
#define BUF_CTL_RESET 16
#define BUF_CTL_NEW_STREAM 17
#define BUF_CTL_REQUEST_NEW_STREAM_INFO 18

void notify_buf_ctl_of_info(chanend buf_ctl, int stream_num);
void notify_buf_ctl_of_new_stream(chanend buf_ctl, int stream_num);
#endif


void buf_ctl_ack(chanend buf_ctl);
int get_buf_ctl_adjust(chanend buf_ctl);
int get_buf_ctl_cmd(chanend buf_ctl);
void send_buf_ctl_info(chanend buf_ctl,
                       int active,
                       unsigned int ptp_ts,
                       unsigned int local_ts,
                       unsigned int rdptr,
                       unsigned int wrptr,
                       timer tmr);

void send_buf_ctl_new_stream_info(chanend buf_ctl,
                                  int media_clock);
