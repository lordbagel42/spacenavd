#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "ws.h"
#include "logger.h"
#include "event.h"
#include "spnavd.h"
#include "dev.h"
#include "cfgfile.h"

#define MAX_WS_CLIENTS 16

static int listen_fd = -1;
static int client_fds[MAX_WS_CLIENTS];

int ws_init(int port)
{
	int i;
	struct sockaddr_in addr;
	for(i=0; i<MAX_WS_CLIENTS; i++) client_fds[i] = -1;
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;
	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if(bind(listen_fd, (struct sockaddr*)&addr, sizeof addr) == -1) return -1;
	if(listen(listen_fd, 5) == -1) return -1;
	logmsg(LOG_INFO, "WS: server started on port %d\n", port);
	return 0;
}

void ws_shutdown(void)
{
	int i;
	if(listen_fd != -1) close(listen_fd);
	for(i=0; i<MAX_WS_CLIENTS; i++) {
		if(client_fds[i] != -1) close(client_fds[i]);
	}
}

int ws_add_fds(fd_set *rset, int *maxfd)
{
	int i;
	if(listen_fd != -1) {
		FD_SET(listen_fd, rset);
		if(listen_fd > *maxfd) *maxfd = listen_fd;
	}
	for(i=0; i<MAX_WS_CLIENTS; i++) {
		if(client_fds[i] != -1) {
			FD_SET(client_fds[i], rset);
			if(client_fds[i] > *maxfd) *maxfd = client_fds[i];
		}
	}
	return 0;
}

static void base64_encode(const unsigned char *input, int length, char *output)
{
    EVP_EncodeBlock((unsigned char *)output, input, length);
}

static void handle_handshake(int fd, char *req)
{
	char *key_start = strstr(req, "Sec-WebSocket-Key: ");
	if(!key_start) return;
	key_start += 19;
	char *key_end = strstr(key_start, "\r\n");
	if(!key_end) return;
	char key[128];
	int key_len = key_end - key_start;
	memcpy(key, key_start, key_len);
	key[key_len] = '\0';
	strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1((unsigned char*)key, strlen(key), hash);
	char accept_key[128];
	base64_encode(hash, SHA_DIGEST_LENGTH, accept_key);
	char response[512];
	sprintf(response, "HTTP/1.1 101 Switching Protocols\r\n"
	                  "Upgrade: websocket\r\n"
	                  "Connection: Upgrade\r\n"
	                  "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);
	write(fd, response, strlen(response));
}

void ws_handle_client_data(int fd, unsigned char *data, int len)
{
	if(data[0] == 0x81) {
		int payload_len = data[1] & 0x7F;
		int mask_offset = 2;
		if(payload_len == 126) mask_offset = 4;
		unsigned char *mask = data + mask_offset;
		unsigned char *payload = data + mask_offset + 4;
		for(int i=0; i<payload_len; i++) payload[i] ^= mask[i % 4];
		payload[payload_len] = '\0';
		logmsg(LOG_DEBUG, "WS received: %s\n", payload);

		if(strstr((char*)payload, "set_led")) {
			set_devices_led(strstr((char*)payload, "true") ? 1 : 0);
		} else if(strstr((char*)payload, "set_lcd")) {
			char *text = strstr((char*)payload, "text\": \"");
			if(text) {
				text += 8;
				char *end = strchr(text, '"');
				if(end) *end = '\0';
				struct device *dev = get_devices();
				if(dev && dev->set_lcd) dev->set_lcd(dev, 0, strlen(text), (unsigned char*)text);
			}
		} else if(strstr((char*)payload, "set_sens")) {
			/* Mock sensitivity adjustment for now */
			logmsg(LOG_INFO, "WS: requested sensitivity change\n");
		}
	}
}

void ws_handle_events(fd_set *rset)
{
	int i;
	if(listen_fd != -1 && FD_ISSET(listen_fd, rset)) {
		int new_fd = accept(listen_fd, NULL, NULL);
		if(new_fd != -1) {
			for(i=0; i<MAX_WS_CLIENTS; i++) {
				if(client_fds[i] == -1) {
					client_fds[i] = new_fd;
					break;
				}
			}
			if(i == MAX_WS_CLIENTS) close(new_fd);
		}
	}
	for(i=0; i<MAX_WS_CLIENTS; i++) {
		if(client_fds[i] != -1 && FD_ISSET(client_fds[i], rset)) {
			unsigned char buf[4096];
			int n = read(client_fds[i], buf, sizeof buf);
			if(n <= 0) {
				close(client_fds[i]);
				client_fds[i] = -1;
			} else {
				if(strstr((char*)buf, "GET /")) handle_handshake(client_fds[i], (char*)buf);
				else ws_handle_client_data(client_fds[i], buf, n);
			}
		}
	}
}

void ws_broadcast_event(const char *event_json)
{
	int i;
	unsigned char frame[4096];
	int len = strlen(event_json);
	frame[0] = 0x81;
	int head_len = 2;
	if(len <= 125) frame[1] = len;
	else if(len <= 65535) {
		frame[1] = 126;
		frame[2] = (len >> 8) & 0xFF;
		frame[3] = len & 0xFF; head_len = 4;
	} else return;
	memcpy(frame + head_len, event_json, len);
	for(i=0; i<MAX_WS_CLIENTS; i++) {
		if(client_fds[i] != -1) write(client_fds[i], frame, head_len + len);
	}
}

void ws_broadcast_spnav_event(spnav_event *ev)
{
	char json[1024];
	if(ev->type == EVENT_MOTION) {
		sprintf(json, "{\"type\": \"motion\", \"data\": [%d, %d, %d, %d, %d, %d]}",
				ev->motion.x, ev->motion.y, ev->motion.z,
				ev->motion.rx, ev->motion.ry, ev->motion.rz);
	} else if(ev->type == EVENT_BUTTON) {
		sprintf(json, "{\"type\": \"button\", \"press\": %d, \"button\": %d}",
				ev->button.press, ev->button.bnum);
	} else return;
	logmsg(LOG_INFO, "WS Broadcast: %s\n", json);
	ws_broadcast_event(json);
}
