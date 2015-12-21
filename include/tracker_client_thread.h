#ifndef __TRACKER_CLIENT_THREAD
#define __TRACKER_CLIENT_THREAD

#include "tracker_types.h"

extern int tracker_report_init();
extern int tracker_report_destroy();
extern int tracker_report_thread_start();

extern int tracker_report_join(TrackerServerInfo *pTrackerServer);
#endif