#ifndef _demo_stream_manager_h_
#define _demo_stream_manager_h_

// This function manages listener streams.
// Paramaters:
//   change_stream - a flag that indicates the manager should try and change to 
//                   the next stream
//   selected_chan - the selected channel pair for output (0,1,2 or 3)
void demo_manage_listener_stream(unsigned int &change_stream,
                                 int selected_chan);

#endif // _demo_stream_manager_h_
