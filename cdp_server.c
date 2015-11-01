#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/composite.h>
#include <x264.h>
#include "list.h"
#include "cdp.h"
#include "libyuv/convert.h"

#define PORT 1234

struct client_node{
    struct list_head list_node;
    struct sockaddr_in sockaddr;
};

struct window_node{
    struct list_head list_node;
    xcb_window_t wid;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    char override_redirect;
    char viewable;
    pthread_t sthread; // stream_thread id
    x264_param_t param;
};

struct client_node client_list;
struct window_node window_list;

xcb_connection_t *xconn; // xorg conn
int sockfd; // UDP sockfd

// when set to 1, x264 output keyframe to all clients
int force_keyframe = 0;

void *stream_thread(void *data)
{
start:
    struct window_node *window = (struct window_node*)data;
    if(window->height == 0 || window->width == 0){
        sleep(2);
        goto start;
    }
    x264_param_t param;
	x264_picture_t pic, pic_out;
	x264_t *h;
	int i_frame = 0;
	int i_frame_size;
	x264_nal_t *nal;
	int i_nal;
	x264_param_default_preset(&param, "veryfast", "zerolatency");
	param.i_csp = X264_CSP_I420;
	param.i_width = window->width;
	param.i_height = window->height;
	param.i_slice_max_size = 1490; // size less than MTU(1500) for udp
	
	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.rc.f_rf_constant = 30;

	if (x264_param_apply_profile(&param, "baseline" ) < 0) {
	    printf("[Error] x264_param_apply_profile\n");
	    return;
	}

	if (x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height) < 0) {
	    printf("[Error] x264_picture_alloc\n");
	    return;
	}

	h = x264_encoder_open(&param);
	if (!h){
	    printf("[Error] x264_encoder_open\n");
		goto fail2;
	}

	int	luma_size = window->width * window->height;
	int	chroma_size	= luma_size / 4;
	xcb_get_image_reply_t	*img;
	for (;; i_frame++ ) {
	    usleep(60000);
	    if(!window->viewable){
	        continue;
	    }
	    if(list_empty(&client_list.list_node)){
	        printf("client_list empty\n");
	        continue;
	    }
		img = xcb_get_image_reply(xconn,
					   xcb_get_image(xconn, XCB_IMAGE_FORMAT_Z_PIXMAP, window->wid,
							  0, 0, window->width, window->height, ~0 ), NULL);
		if ( img == NULL ) {// often coursed by GL window
			printf("get image error\n");
			sleep( 5 );
			continue;
		}

		ARGBToI420(xcb_get_image_data(img), window->width * 4,
			    pic.img.plane[0], window->width,
			    pic.img.plane[1], window->width / 2,
			    pic.img.plane[2], window->width / 2,
			    window->width, window->height );
		free(img);
	    pic.i_pts = i_frame;
    	i_frame_size = x264_encoder_encode(h, &nal, &i_nal, &pic, &pic_out);
    	if (i_frame_size < 0) {
    	    printf("[Error] x264_encoder_encode\n");
    		goto fail3;
    	}
    	if (i_frame_size){
    	    int i;
        	struct client_node *iter; 
        	list_for_each_entry(iter, &client_list.list_node, list_node) {
        	    for (i = 0; i < i_nal; ++i) {
                    printf("nal size: %d\n", nal[i].i_payload);
                    int length = 12 + nal[i].i_payload;
                    char _buf[length];
                    _buf[0] = 1;
                    _buf[4] = length;
                    _buf[8] = window->wid;
                    memcpy(&_buf[12], nal[i].p_payload, nal[i].i_payload);
                    sendto(sockfd, _buf, length, 0, (struct sockaddr *)&iter->sockaddr, sizeof(struct sockaddr_in));
                }
        	}
		}
	}
	
fail3:
	x264_encoder_close(h);
fail2:
	x264_picture_clean(&pic);
}

struct client_node *add_client(struct sockaddr_in sockaddr)
{
    struct client_node *client = (struct client_node*)malloc(sizeof(struct client_node));
    client->sockaddr = sockaddr;
    list_add_tail(&client->list_node, &client_list.list_node);
    return client;
}

struct window_node *add_window(xcb_window_t wid)
{
    struct window_node *window;
    window = (struct window_node*)malloc(sizeof(struct window_node));
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, wid), NULL);
    xcb_get_window_attributes_reply_t  *attr = xcb_get_window_attributes_reply(xconn, xcb_get_window_attributes(xconn, wid), NULL);

    window->wid = wid;
    window->x = geo->x;
    window->y = geo->y;
    window->width = geo->width;
    window->height = geo->height;
    window->override_redirect = attr->override_redirect;
    window->viewable = attr->map_state == XCB_MAP_STATE_VIEWABLE?1:0;
    pthread_create(&window->sthread, NULL, stream_thread, &window);
    list_add_tail(&window->list_node, &window_list.list_node);
    free(geo);
    free(attr);
    return window;
}

void handle_message(char *buf, int sockfd, struct sockaddr_in sockaddr)
{
    switch(buf[0]) {
        case CDP_REQUEST_RECOVER:
        break;
        case CDP_REQUEST_LISTEN:
            add_client(sockaddr);
        break;
    }
}

void *xorg_thread()
{
    xcb_screen_t *screen;
    xcb_window_t root;
    xconn = xcb_connect(NULL, NULL);
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
    //xcb_composite_redirect_subwindows(c, root, XCB_COMPOSITE_REDIRECT_AUTOMATIC);
    xcb_query_tree_reply_t *tree = xcb_query_tree_reply(xconn, xcb_query_tree(xconn, root), NULL);
    xcb_window_t *children = xcb_query_tree_children(tree);
    int i = 0;
    for (i; i < xcb_query_tree_children_length(tree); i++) {
        add_window(children[i]);
    }
    free(tree);
    
    xcb_generic_event_t *event;
    xcb_create_notify_event_t *cne;
    xcb_map_notify_event_t *mne;
    xcb_unmap_notify_event_t *umne;
    xcb_destroy_notify_event_t *dne;
    struct window_node *iter; 
    while (event = xcb_wait_for_event (xconn)) {
        switch (event->response_type & ~0x80) {
            case XCB_CREATE_NOTIFY:
                cne = (xcb_create_notify_event_t*)event;
                add_window(cne->window);
            break;
            case XCB_MAP_NOTIFY:
                mne = (xcb_map_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->wid == mne->window){
            	        iter->viewable = 1;
            	        break;
            	    }
            	}
            break;
            case XCB_UNMAP_NOTIFY:
                umne = (xcb_unmap_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->wid == mne->window){
            	        iter->viewable = 0;
            	        break;
            	    }
            	}
            break;
            case XCB_DESTROY_NOTIFY:
                dne = (xcb_destroy_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->wid == mne->window){
            	        list_del(&iter->list_node);
            	        free(iter);
            	        break;
            	    }
            	}
            break;
        }
    }
    xcb_disconnect(xconn);
}

int main(int argc, char *argv[])
{
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
    server.sin_port = htons(PORT);
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