#include <stdlib.h>
#include "cdp_protocol.h"
#include "cdp_server.h"

extern int sockfd;
extern struct client_node client_list;

void cdp_message_create_window(cdp_window_t *window)
{
    cdp_message_create_window_t msg;
    msg.msgtype = 1;
    msg.window = window->id;
    msg.x = window->x;
    msg.y = window->y;
    msg.width = window->width;
    msg.height = window->height;
    msg.override = window->override;
    msg.viewable = window->viewable;
    struct client_node *iter;
    list_for_each_entry(iter, &client_list.list_node, list_node) {
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&iter->sockaddr, sizeof(struct sockaddr_in));
    }
}