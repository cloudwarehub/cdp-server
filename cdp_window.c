#include <stdlib.h>
#include <xcb/xcb.h>
#include <pthread.h>
#include "cdp_server.h"
#include "cdp_protocol.h"
#include "list.h"

extern struct window_node window_list;

extern xcb_connection_t *xconn; // xorg conn

struct window_node *add_window(cdp_window_t *window)
{
    struct window_node *windownode;
    windownode = (struct window_node*)malloc(sizeof(struct window_node));
    windownode->window = window;
    
    pthread_create(&windownode->sthread, NULL, stream_thread, windownode);
    list_add_tail(&windownode->list_node, &window_list.list_node);
    return windownode;
}

struct window_node *add_window_byid(u32 wid)
{
    cdp_window_t *window;
    window = (cdp_window_t*)malloc(sizeof(cdp_window_t));
    
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, wid), NULL);
    xcb_get_window_attributes_reply_t  *attr = xcb_get_window_attributes_reply(xconn, xcb_get_window_attributes(xconn, wid), NULL);

    window->id = wid;
    window->x = geo->x;
    window->y = geo->y;
    window->width = geo->width;
    window->height = geo->height;
    window->override = attr->override_redirect;
    window->viewable = attr->map_state == XCB_MAP_STATE_VIEWABLE?1:0;
    free(geo);
    free(attr);
    return add_window(window);
}

void remove_window(xcb_window_t wid)
{
    struct window_node *iter;
    list_for_each_entry(iter, &window_list.list_node, list_node) {
	    if(iter->window->id == wid){
	        list_del(&iter->list_node);
	        free(iter);
	        break;
	    }
	}
}

void show_window(u32 wid)
{
    struct window_node *iter;
    list_for_each_entry(iter, &window_list.list_node, list_node) {
	    if(iter->window->id == wid){
	        iter->window->viewable = 1;
	        break;
	    }
	}
}

void hide_window(u32 wid)
{
    struct window_node *iter;
    list_for_each_entry(iter, &window_list.list_node, list_node) {
	    if(iter->window->id == wid){
	        iter->window->viewable = 0;
	        break;
	    }
	}
}