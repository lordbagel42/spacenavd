/*
spacenavd - a free software replacement driver for 6dof space-mice.
Copyright (C) 2007-2025 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ws.h"
#include "event.h"
#include "client.h"
#include "proto_unix.h"
#include "spnavd.h"
#include "kbemu.h"
#ifdef USE_X11
#include "proto_x11.h"
#endif

struct dev_event {
	spnav_event event;
	struct timeval timeval;
	struct device *dev;
	int pending;
	struct dev_event *next;
};

static struct dev_event *dev_ev_list = NULL;

static struct dev_event *add_dev_event(struct device *dev)
{
	struct dev_event *dev_ev, *iter;
	if(!(dev_ev = malloc(sizeof *dev_ev))) return NULL;
	dev_ev->event.motion.data = (int*)&dev_ev->event.motion.x;
	for(int i=0; i<6; i++) dev_ev->event.motion.data[i] = 0;
	gettimeofday(&dev_ev->timeval, 0);
	dev_ev->dev = dev; dev_ev->next = NULL;
	if(!dev_ev_list) return dev_ev_list = dev_ev;
	iter = dev_ev_list; while(iter->next) iter = iter->next;
	iter->next = dev_ev; return dev_ev;
}

void remove_dev_event(struct device *dev)
{
	struct dev_event dummy, *iter = &dummy;
	dummy.next = dev_ev_list;
	while(iter->next) {
		if(iter->next->dev == dev) {
			struct dev_event *ev = iter->next;
			iter->next = ev->next; free(ev);
		} else iter = iter->next;
	}
	dev_ev_list = dummy.next;
}

static struct dev_event *device_event_in_use(struct device *dev)
{
	struct dev_event *iter = dev_ev_list;
	while(iter) { if(iter->dev == dev) return iter; iter = iter->next; }
	return NULL;
}

static void send_event(spnav_event *ev, struct client *c)
{
	switch(get_client_type(c)) {
#ifdef USE_X11
	case CLIENT_X11: send_xevent(ev, c); break;
#endif
	case CLIENT_UNIX: send_uevent(ev, c); break;
	}
}

void broadcast_event(spnav_event *ev)
{
	struct client *c = first_client();
	while(c) { send_event(ev, c); c = c->next; }
	ws_broadcast_spnav_event(ev);
}

static void dispatch_event(struct dev_event *dev_ev)
{
	struct client *c = first_client();
	while(c) {
		struct device *client_dev = get_client_device(c);
		if(!client_dev || client_dev == dev_ev->dev) send_event(&dev_ev->event, c);
		c = c->next;
	}
	ws_broadcast_spnav_event(&dev_ev->event);
}

void process_input(struct device *dev, struct dev_input *inp)
{
	struct dev_event *dev_ev;
	spnav_event ev;
	switch(inp->type) {
	case INP_MOTION:
		dev_ev = device_event_in_use(dev);
		if(!dev_ev) dev_ev = add_dev_event(dev);
		if(!dev_ev) break;
		dev_ev->event.type = EVENT_MOTION;
		dev_ev->event.motion.data = (int*)&dev_ev->event.motion.x;
		dev_ev->event.motion.data[inp->idx] = inp->val;
		dev_ev->pending = 1;
		break;
	case INP_BUTTON:
		ev.type = EVENT_BUTTON;
		ev.button.press = inp->val;
		ev.button.bnum = inp->idx;
		broadcast_event(&ev);
		break;
	case INP_FLUSH:
		dev_ev = device_event_in_use(dev);
		if(dev_ev && dev_ev->pending) {
			dispatch_event(dev_ev);
			dev_ev->pending = 0;
			for(int i=0; i<6; i++) dev_ev->event.motion.data[i] = 0;
		}
		break;
	}
}

int in_deadzone(struct device *dev) {
	struct dev_event *dev_ev = device_event_in_use(dev);
	if(!dev_ev) return 1;
	for(int i=0; i<6; i++) if(dev_ev->event.motion.data[i] != 0) return 0;
	return 1;
}
void repeat_last_event(struct device *dev) {
	struct dev_event *dev_ev = device_event_in_use(dev);
	if(dev_ev) dispatch_event(dev_ev);
}
void broadcast_cfg_event(int cfg_id, int val) {
	spnav_event ev = {0};
	ev.type = EVENT_CFG;
	ev.cfg.cfg = cfg_id;
	ev.cfg.data[0] = val;
	broadcast_event(&ev);
}
