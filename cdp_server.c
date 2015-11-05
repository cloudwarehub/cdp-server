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
#include "libyuv/convert.h"
#include "cdp_server.h"

#define PORT 1234

struct client_node client_list;
struct window_node window_list;

xcb_connection_t *xconn; // xorg conn
int sockfd; // UDP sockfd

// when set to 1, x264 output keyframe to all clients
int force_keyframe = 0;

void *stream_thread(void *data)
{
    struct window_node *windownode = (struct window_node*)data;
    cdp_window_t *window = windownode->window;
    
    if(window->height == 0 || window->width == 0){
        return;
    }
    printf("%d %d\n", window->width, window->height);
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
					   xcb_get_image(xconn, XCB_IMAGE_FORMAT_Z_PIXMAP, window->id,
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
    	    for (i = 0; i < i_nal; ++i) {
    	        cdp_message_window_frame(window->id, nal[i].p_payload, nal[i].i_payload);
                printf("nal size: %d\n", nal[i].i_payload);
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

void handle_message(char *buf, int sockfd, struct sockaddr_in sockaddr)
{
    switch(buf[0]) {
        case CDP_REQUEST_LISTEN:
            printf("new listen client\n");
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
                cdp_window_t *window;
                window = (cdp_window_t*)malloc(sizeof(cdp_window_t));
                
                xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(xconn, xcb_get_geometry(xconn, cne->window), NULL);
                xcb_get_window_attributes_reply_t  *attr = xcb_get_window_attributes_reply(xconn, xcb_get_window_attributes(xconn, cne->window), NULL);
            
                window->id = cne->window;
                window->x = geo->x;
                window->y = geo->y;
                window->width = geo->width;
                window->height = geo->height;
                window->override = attr->override_redirect;
                window->viewable = attr->map_state == XCB_MAP_STATE_VIEWABLE?1:0;
                free(geo);
                free(attr);
                cdp_message_create_window(window);
                add_window(window);
            break;
            case XCB_MAP_NOTIFY:
                mne = (xcb_map_notify_event_t*)event;
            	list_for_each_entry(iter, &window_list.list_node, list_node) {
            	    if(iter->window->id == mne->window){
            	        iter->window->viewable = 1;
            	        break;
            	    }
            	}
            	cdp_message_show_window(mne->window);
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