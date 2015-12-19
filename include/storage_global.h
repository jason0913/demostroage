#ifndef __CLIENT_GLOBAL_H
#define __CLIENT_GLOBAL_H

#include "tracker_types.h"

extern int g_tracker_server_count;
extern int g_tracker_server_index;  //server index for roundrobin
extern TrackerServerInfo *g_tracker_server;

#endif