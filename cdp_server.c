#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/composite.h>
#include <x264.h>
#include "list.h"
#include "libyuv/convert.h"
#include "cdp_defs.h"
#include "cdp_server.h"
#include "cdp_protocol.h"
#include "cdp_stream.h"
#include "cdp_window.h"

int port = 5999;
char display[20] = ":0";

struct client_node client_list;
struct window_node window_list;

xcb_connection_t *xconn; // xorg conn
int sockfd; // UDP sockfd

// when set to 1, x264 output keyframe to all clients
int force_keyframe = 0;

struct client_node *add_client(struct sockaddr_in sockaddr, int resource)
{
    struct client_node *client = (struct client_node*)malloc(sizeof(struct client_node));
    client->sockaddr = sockaddr;
    client->resource = resource;
    list_add_tail(&client->list_node, &client_list.list_node);
    struct window_node *iter;
    list_for_each_entry(iter, &window_list.list_node, list_node) {
        iter->force_keyframe = 1;
    }
    return client;
}

void remove_client(int resource)
{
    struct client_node *iter;
    list_for_each_entry(iter, &client_list.list_node, list_node) {
        if (iter->resource == resource) {
            list_del(&iter->list_node);
            break;
        }
    }
}

void recover(struct sockaddr_in client)
{
    struct window_node *iter;
    list_for_each_entry(iter, &window_list.list_node, list_node) {
        cdp_message_create_window(iter->window);
    }
}

int resource = 1; // global client id, used for unlisten

void handle_message(char *buf, int sockfd, struct sockaddr_in sockaddr)
{
    switch(buf[0]) {
        case CDP_REQUEST_LISTEN:
            printf("new listen client\n");
            struct client_node *node = add_client(sockaddr, resource++);
            cdp_message_listen_reply_t msg;
            msg.msgtype = CDP_MESSAGE_LISTEN_REPLY;
            msg.resource = node->resource;
            cdp_cast_message(&msg, sizeof(msg));
            recover(sockaddr);
            break;
        case CDP_REQUEST_UNLISTEN:
            printf("unlisten client\n");
            remove_client(((cdp_request_unlisten_t*)buf)->resource);
            break;
        case CDP_REQUEST_MOUSEMOVE: ;
            cdp_request_mousemove_t *req = (cdp_request_mousemove_t*)buf;
            cdp_input_mousemove(req);
            break;
        case CDP_REQUEST_MOUSEDOWN: ;
            cdp_input_mousedown((cdp_request_mousedown_t*)buf);
            break;
        case CDP_REQUEST_MOUSEUP: ;
            cdp_input_mouseup((cdp_request_mouseup_t*)buf);
            break;
        case CDP_REQUEST_KEYDOWN:
            cdp_input_keydown((cdp_request_keydown_t*)buf);
            break;
        case CDP_REQUEST_KEYUP:
            cdp_input_keyup((cdp_request_keyup_t*)buf);
            break;
        case CDP_REQUEST_KEYPRESS:
            cdp_input_keypress((cdp_request_keypress_t*)buf);
            break;
        case CDP_REQUEST_WINDOW_MOVE: ;
            printf("move it\n");
            cdp_request_window_move_t *crwm = (cdp_request_window_move_t*)buf;
            cdp_window_move(crwm->wid, crwm->x, crwm->y);
            cdp_x11_request_window_move(crwm->wid, crwm->x, crwm->y);
            break;
        case CDP_REQUEST_WINDOW_RESIZE:
            cdp_request_window_resize((cdp_request_window_resize_t*)buf);
            break;
    }
}

void *xorg_thread()
{
    xcb_screen_t *screen;
    xcb_window_t root;
    xconn = xcb_connect(display, NULL);
	if (xcb_connection_has_error(xconn))
	{
		printf("cannot connect display\n");
		xcb_disconnect(xconn);
		exit(-1);
	}
	screen = xcb_setup_roots_iterator(xcb_get_setup(xconn)).data;
	root = screen->root;
	uint32_t mask[] = { XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };
	xcb_change_window_attributes(xconn, root, XCB_CW_EVENT_MASK, mask);
    xcb_composite_redirect_subwindows(xconn, root, XCB_COMPOSITE_REDIRECT_AUTOMATIC);
    
    xcb_query_tree_reply_t *tree = xcb_query_tree_reply(xconn, xcb_query_tree(xconn, root), NULL);
    xcb_window_t *children = xcb_query_tree_children(tree);
    int i = 0;
    for (i; i < xcb_query_tree_children_length(tree); i++) {
        add_window_byid(children[i]);
    }
    free(tree);
    
    xcb_generic_event_t *event;
    xcb_create_notify_event_t *cne;
    xcb_map_notify_event_t *mne;
    xcb_unmap_notify_event_t *umne;
    xcb_destroy_notify_event_t *dne;
    xcb_configure_notify_event_t *cone;
    struct window_node *iter;
    while (event = xcb_wait_for_event (xconn)) {
        printf("new event: %d\n", event->response_type);
        switch (event->response_type & ~0x80) {
            case XCB_CREATE_NOTIFY:
                cne = (xcb_create_notify_event_t*)event;
                cdp_window_t *window;
                window = (cdp_window_t*)malloc(sizeof(cdp_window_t));
                
                window->id = cne->window;
                window->x = cne->x;
                window->y = cne->y;
                window->width = cne->width;
                window->height = cne->height;
                window->override = cne->override_redirect;
                window->viewable = 0;
                cdp_message_create_window(window);
                add_window(window);
            break;
            case XCB_MAP_NOTIFY:
                mne = (xcb_map_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->window->id == mne->window){
            	        iter->window->viewable = 1;
            	        iter->window->override = mne->override_redirect;
            	        if(!iter->sthread)
                            pthread_create(&iter->sthread, NULL, stream_thread, iter);
            	        cdp_message_show_window(iter->window);
            	        iter->force_keyframe = 1;
            	        break;
            	    }
            	}
            break;
            case XCB_UNMAP_NOTIFY:
                umne = (xcb_unmap_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->window->id == umne->window){
            	        iter->window->viewable = 0;
            	        break;
            	    }
            	}
            	cdp_message_hide_window(umne->window);
            break;
            case XCB_DESTROY_NOTIFY:
                dne = (xcb_destroy_notify_event_t*)event;
            	cdp_message_destroy_window(dne->window);
            	remove_window(dne->window);
            break;
            case XCB_CONFIGURE_NOTIFY:
                cone = (xcb_configure_notify_event_t*)event;
                cdp_window_t *w = cdp_window_configure(cone->window, cone->x, cone->y, cone->width, cone->height, cone->override_redirect, cone->above_sibling);
                cdp_message_configure_window(w);
                printf("conf notify %d %d %d %d %d\n", cone->window, cone->x, cone->y, cone->width, cone->height);
                fflush(stdout);
            break;
        }
    }
    xcb_disconnect(xconn);
}

void usage(char *exe)
{
    printf("Usage: %s -p [port(5999)] -d [display(:0)]\n", exe);
}

int main(int argc, char *argv[])
{
    printf("welcome to cdp-server\n");
    int ch;
    while ((ch = getopt(argc, argv, "p:d:h")) != -1) {
        switch (ch) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                strcpy(display, optarg);
                break;
            case 'h':
            case '?':
                usage(argv[0]);
                exit(-1);
                break;
            default:
                break;
        }
    }
    INIT_LIST_HEAD(&window_list.list_node);
    INIT_LIST_HEAD(&client_list.list_node);
    pthread_t xthread;
    pthread_create(&xthread, NULL, xorg_thread, NULL);
    
    struct sockaddr_in server;
    struct sockaddr_in client;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Creating socket failed.");
        exit(1);
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("Bind error.");
        exit(1);
    }
    socklen_t sin_size = sizeof(struct sockaddr_in);
    
    char buf[128];
    while(1){
        recvfrom(sockfd, buf, 128, 0, (struct sockaddr *)&client, &sin_size);
        handle_message(buf, sockfd, client);
    }
    return 0;
}