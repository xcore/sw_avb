#ifndef _avb_1722_stream_table_h_
#define _avb_1722_stream_table_h_

void xdk_manage_streams(chanend listener_config,
                        chanend txt_svr,
                        int display_result,
                        unsigned int &change_stream);

void xdk_stream_manager_display_table(chanend txt_svr);

#endif // _avb_1722_stream_table_h_
