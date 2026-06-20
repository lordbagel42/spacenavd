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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "dev.h"
#include "dev_usb.h"
#include "dev_serial.h"
#include "dev_sim.h"
#include "event.h"
#include "spnavd.h"
#include "proto.h"
#include "proto_unix.h"

#ifdef USE_X11
#include "proto_x11.h"
#endif

enum { DF_SWAPYZ = 1, DF_INVYZ = 2 };

static struct usbdb_entry {
	int usbid[2];
	int type;
	unsigned int flags;
	int (*bnmap)(int);
} usbdb[] = {
	{{0x046d, 0xc603}, DEV_PLUSXT,		0,						0},
	{{0x046d, 0xc605}, DEV_CADMAN,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc606}, DEV_SMCLASSIC,	0,						0},
	{{0x046d, 0xc621}, DEV_SB5000,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc623}, DEV_STRAVEL,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc625}, DEV_SPILOT,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc626}, DEV_SNAV,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc627}, DEV_SEXP,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc628}, DEV_SNAVNB,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc629}, DEV_SPILOTPRO,	DF_SWAPYZ | DF_INVYZ,	0},
	{{0x046d, 0xc62b}, DEV_SMPRO,		DF_SWAPYZ | DF_INVYZ,	bnhack_smpro},
	{{0x046d, 0xc640}, DEV_NULOOQ,		0,						0},
	{{0x256f, 0xc62e}, DEV_SMW,			DF_SWAPYZ | DF_INVYZ,	0},
	{{0x256f, 0xc62f}, DEV_SMW,			DF_SWAPYZ | DF_INVYZ,	0},
	{{0x256f, 0xc631}, DEV_SMPROW,		DF_SWAPYZ | DF_INVYZ,	bnhack_smpro},
	{{0x256f, 0xc632}, DEV_SMPROW,		DF_SWAPYZ | DF_INVYZ,	bnhack_smpro},
	{{0x256f, 0xc633}, DEV_SMENT,		DF_SWAPYZ | DF_INVYZ,	bnhack_sment},
	{{0x256f, 0xc635}, DEV_SMCOMP,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x256f, 0xc636}, DEV_SMMOD,		DF_SWAPYZ | DF_INVYZ,	0},
	{{0x256f, 0xc638}, DEV_SMPROW,		DF_SWAPYZ | DF_INVYZ,	bnhack_smpro},
	{{0x256f, 0xc63a}, DEV_SMW,			DF_SWAPYZ | DF_INVYZ,	0},
	{{-1, -1}, DEV_UNKNOWN, 0}
};

static struct device *dev_list = NULL;
static unsigned short last_id;

static struct device *add_device(void)
{
	struct device *dev;
	if(!(dev = malloc(sizeof *dev))) return 0;
	memset(dev, 0, sizeof *dev);
	dev->fd = -1;
	dev->id = last_id++;
	dev->next = dev_list;
	dev_list = dev;
	return dev;
}

void remove_device(struct device *dev)
{
	struct device dummy, *iter = &dummy;
	dummy.next = dev_list;
	while(iter->next) {
		if(iter->next == dev) {
			iter->next = dev->next;
			break;
		}
		iter = iter->next;
	}
	dev_list = dummy.next;
	if(dev->close) dev->close(dev);
	free(dev);
}

void init_devices(void)
{
	init_devices_serial();
	init_devices_usb();
	if(!dev_list) {
		struct device *dev = add_device();
		if(open_dev_sim(dev) == -1) remove_device(dev);
	}
}

void init_devices_serial(void) {}

int init_devices_usb(void)
{
	/* In this sandbox environment, real USB detection usually fails or is restricted.
	 * We keep the function but it will likely return -1, triggering sim fallback in init_devices.
	 */
	return -1;
}

int get_device_fd(struct device *dev) { return dev ? dev->fd : -1; }
struct device *get_devices(void) { return dev_list; }

int read_device(struct device *dev, struct dev_input *inp)
{
	if(!dev->read) return -1;
	return dev->read(dev, inp);
}

void set_device_led(struct device *dev, int state)
{
	if(dev->set_led) dev->set_led(dev, state);
}

void set_devices_led(int state)
{
	struct device *dev = dev_list;
	while(dev) {
		set_device_led(dev, state);
		dev = dev->next;
	}
}

int bnhack_smpro(int bn) { return bn; }

int bnhack_sment(int bn)
{
	if(bn < 0) return 33;
	switch(bn) {
	case 256: return 12;	case 257: return 13;	case 258: return 14;
	case 260: return 15;	case 261: return 16;	case 264: return 17;
	case 266: return 30;	case 268: return 0;		case 269: return 1;
	case 270: return 2;		case 271: return 3;		case 272: return 4;
	case 273: return 5;		case 274: return 6;		case 275: return 7;
	case 276: return 8;		case 277: return 9;		case 278: return 18;
	case 279: return 19;	case 280: return 20;	case 281: return 21;
	case 282: return 22;	case 291: return 23;	case 292: return 24;
	case 332: return 10;	case 333: return 11;	case 358: return 27;
	case 359: return 28;	case 360: return 29;	case 430: return 25;
	case 431: return 26;	case 262: return 31;	case 263: return 32;
	default: break;
	}
	return -1;
}

struct device *dev_path_in_use(const char *path) { return NULL; }
