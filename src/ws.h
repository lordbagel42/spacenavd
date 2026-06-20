#ifndef WS_H_
#define WS_H_

#include <sys/select.h>
#include "event.h"

int ws_init(int port);
void ws_shutdown(void);
int ws_add_fds(fd_set *rset, int *maxfd);
void ws_handle_events(fd_set *rset);
void ws_broadcast_event(const char *event_json);
void ws_broadcast_spnav_event(spnav_event *ev);

#endif
