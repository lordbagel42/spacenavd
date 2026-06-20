#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "dev_sim.h"
#include "logger.h"
#include "event.h"

static void close_sim(struct device *dev);
static int read_sim(struct device *dev, struct dev_input *inp);
static void set_led_sim(struct device *dev, int state);
static void set_lcd_sim(struct device *dev, int offset, int len, unsigned char *data);

int open_dev_sim(struct device *dev)
{
	int s;
	struct sockaddr_in addr;

	if((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(9999);

	if(bind(s, (struct sockaddr*)&addr, sizeof addr) == -1) {
		logmsg(LOG_ERR, "Sim: failed to bind: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	fcntl(s, F_SETFL, O_NONBLOCK);

	dev->fd = s;
	dev->num_axes = 6;
	dev->num_buttons = 33;
	strcpy(dev->name, "Simulated SpaceMouse Enterprise");

	dev->close = close_sim;
	dev->read = read_sim;
	dev->set_led = set_led_sim;
	dev->set_lcd = set_lcd_sim;

	logmsg(LOG_INFO, "Simulated device started on UDP port 9999\n");
	return 0;
}

static void close_sim(struct device *dev)
{
	if(dev->fd != -1) {
		close(dev->fd);
		dev->fd = -1;
	}
}

static int read_sim(struct device *dev, struct dev_input *inp)
{
	unsigned char buf[64];

	int n = recv(dev->fd, buf, sizeof buf, 0);
	if(n <= 0) return -1;

	if(buf[0] == 0x01) {
		inp->type = INP_MOTION;
		inp->idx = buf[1];
		inp->val = (int16_t)(buf[2] | (buf[3] << 8));
	} else if(buf[0] == 0x02) {
		inp->type = INP_BUTTON;
		inp->idx = buf[1];
		inp->val = buf[2];
	} else if(buf[0] == 0x03) {
		inp->type = INP_FLUSH;
	} else {
		return -1;
	}

	return 0;
}

static void set_led_sim(struct device *dev, int state)
{
	logmsg(LOG_INFO, "Sim LED set to %d\n", state);
}

static void set_lcd_sim(struct device *dev, int offset, int len, unsigned char *data)
{
	logmsg(LOG_INFO, "Sim LCD update: offset=%d len=%d\n", offset, len);
}
