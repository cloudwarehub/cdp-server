#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cdp_protocol.h"
#include "cdp_server.h"

extern int sockfd;
extern struct client_node client_list;

/**
 * cast message to all listened clients
 **/
void cdp_cast_message(void *buf, uint32_t length)
{
    struct client_node *iter;
    list_for_each_entry(iter, &client_list.list_node, list_node) {
        sendto(sockfd, buf, length, 0, (struct sockaddr *)&iter->sockaddr, sizeof(struct sockaddr_in));
    }
}

void cdp_message_create_window(cdp_window_t *window)
{
    cdp_message_create_window_t msg;
    msg.msgtype = CDP_MESSAGE_CREATE_WINDOW;
    msg.window = window->id;
    msg.x = window->x;
    msg.y = window->y;
    msg.width = window->width;
    msg.height = window->height;
    msg.override = window->override;
    msg.viewable = window->viewable;
    cdp_cast_message(&msg, sizeof(msg));
}

void cdp_message_destroy_window(u32 id)
{
    cdp_message_destroy_window_t msg;
    msg.msgtype = CDP_MESSAGE_DESTROY_WINDOW;
    msg.window = id;
    cdp_cast_message(&msg, sizeof(msg));
}

void cdp_message_show_window(u32 id)
{
    cdp_message_show_window_t msg;
    msg.msgtype = CDP_MESSAGE_SHOW_WINDOW;
    msg.window = id;
    cdp_cast_message(&msg, sizeof(msg));
}

void cdp_message_hide_window(u32 id)
{
    cdp_message_hide_window_t msg;
    msg.msgtype = CDP_MESSAGE_HIDE_WINDOW;
    msg.window = id;
    cdp_cast_message(&msg, sizeof(msg));
}

void cdp_message_window_frame(u32 wid, char *frame_data, int size)
{
    int length = sizeof(cdp_message_window_frame_t) + size;
    char buf[length];
    cdp_message_window_frame_t *msg = (cdp_message_window_frame_t*)buf;
    msg->msgtype = CDP_MESSAGE_WINDOW_FRAME;
    msg->length = length;
    msg->window = wid;
    memcpy(&buf[sizeof(cdp_message_window_frame_t)], frame_data, size);
    printf("%d, %d", length, sizeof(cdp_message_window_frame_t))
    cdp_cast_message(buf, length);
}