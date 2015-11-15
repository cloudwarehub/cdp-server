#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x264.h>
#include <xcb/xcb.h>
#include <pthread.h>
#include <signal.h>
#include "cdp_defs.h"
#include "list.h"
#include "cdp_server.h"
#include "cdp_stream.h"

extern struct client_node client_list;
extern xcb_connection_t *xconn;
extern struct window_node window_list;

void *stream_thread(void *data)
{
    struct window_node *windownode = (struct window_node*)data;
    cdp_window_t *window = windownode->window;
    uint16_t width;
    uint16_t height;
    
	pthread_mutex_lock(&window->lock);
    width = window->nwidth;
    height = window->nheight;
	pthread_mutex_unlock(&window->lock);
    
    if(height <= 4 || width <= 4){
        return;
    }
    printf("%d %d\n", window->width, window->height);
    
    x264_param_t *param = &windownode->param;
	x264_picture_t pic, pic_out;
	x264_t *h;
	int i_frame = 0;
	int i_frame_size;
	x264_nal_t *nal;
	int i_nal;
	x264_param_default_preset(param, "veryfast", "zerolatency");
	param->i_csp = X264_CSP_I420;
	param->i_width = width;
	param->i_height = height;
	param->i_slice_max_size = 149000; // size less than MTU(1500) for udp
	//param.i_keyint_max = 60;
	
	param->b_vfr_input = 0;
	param->b_repeat_headers = 1;
	param->b_annexb = 1;
	param->rc.f_rf_constant = 30;
	

	if (x264_param_apply_profile(param, "baseline" ) < 0) {
	    printf("[Error] x264_param_apply_profile\n");
	    return;
	}

	if (x264_picture_alloc(&pic, param->i_csp, param->i_width, param->i_height) < 0) {
	    printf("[Error] x264_picture_alloc\n");
	    return;
	}

	h = x264_encoder_open(param);
	if (!h){
	    printf("[Error] x264_encoder_open\n");
		goto fail2;
	}

	xcb_get_image_reply_t *img;
	int interval = 60000;
	if (windownode->window->override) {
		interval = 200000;
	}
	for (;; i_frame++) {
	    usleep(interval);
	    if (windownode->refresh) {
			pthread_mutex_lock(&window->lock);
	    	windownode->refresh = 0;
	    	pthread_create(&windownode->sthread, NULL, stream_thread, windownode);
			pthread_mutex_unlock(&window->lock);
	    	break;
	    	
	    	
	    	width = window->nwidth;
	    	height = window->nheight;
	    	param->i_csp = X264_CSP_I420;
			param->i_width = windownode->nwidth;
			param->i_height = windownode->nheight;
			x264_param_apply_profile(param, "baseline");
			x264_picture_clean(&pic);
			x264_picture_alloc(&pic, param->i_csp, param->i_width, param->i_height);
			x264_encoder_close(h);
			h = x264_encoder_open(param);
			windownode->refresh = 0;
	    }
	    if (!window->viewable) { //should use wait
	        continue;
	    }
	    if (list_empty(&client_list.list_node)) { //should use wait
	        //printf("client_list empty\n");
	        continue;
	    }
		img = xcb_get_image_reply(xconn,
					   xcb_get_image(xconn, XCB_IMAGE_FORMAT_Z_PIXMAP, window->id,
							  0, 0, width, height, ~0 ), NULL);
		if ( img == NULL ) {// often coursed by GL window
			printf("get image error\n");
			sleep( 5 );
			continue;
		}

		ARGBToI420(xcb_get_image_data(img), width * 4,
			    pic.img.plane[0], width,
			    pic.img.plane[1], width / 2,
			    pic.img.plane[2], width / 2,
			    width, height );
		free(img);
		if (windownode->force_keyframe) {
			pic.i_type = X264_TYPE_KEYFRAME;
			windownode->force_keyframe = 0;
		}else{
			pic.i_type = X264_TYPE_AUTO;
		}
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
                //printf("nal size: %d\n", nal[i].i_payload);
            }
		}
	}
	
fail3:
	x264_encoder_close(h);
fail2:
	x264_picture_clean(&pic);
}

void cdp_stream_resize(u32 wid, u16 width, u16 height)
{
	struct window_node *iter;
	if(width % 2){
    	width = width--;
    }
    if(height % 2){
    	height = height--;
    }
    list_for_each_entry(iter, &window_list.list_node, list_node) {
	    if(iter->window->id == wid){
			pthread_mutex_lock(&iter->window->lock);
	    	iter->window->nwidth = width;
	    	iter->window->nheight = height;
	    	iter->refresh = 1;
			pthread_mutex_unlock(&iter->window->lock);
	        break;
	    }
	}
}
