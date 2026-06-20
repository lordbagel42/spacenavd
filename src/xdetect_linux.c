#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include "xdetect.h"
#include "logger.h"

int xdet_get_fd(void) { return -1; }
int xdet_start(void) {
    logmsg(LOG_INFO, "X detection started (Wayland/Hyprland support enabled via WebSocket)\n");
    return 0;
}
void xdet_stop(void) {}
int handle_xdet_events(fd_set *rset) { return 0; }
