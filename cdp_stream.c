#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x264.h>
#include <xcb/xcb.h>
#include "list.h"
#include "cdp_server.h"
#include "cdp_stream.h"

extern struct client_node client_list;
extern xcb_connection_t *xconn;

void *stream_thread(void *data)
{
    struct window_node *windownode = (struct window_node*)data;
    cdp_window_t *window = windownode->window;
    uint16_t width;
    uint16_t height;
    
    if(window->height <= 4 || window->width <= 4){
        return;
    }
    width = window->width;
    height = window->height;
    printf("%d %d\n", window->width, window->height);
    if(width % 2){
    	width--;
    }
    if(height % 2){
    	height--;
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
	param.i_width = width;
	param.i_height = height;
	param.i_slice_max_size = 14900; // size less than MTU(1500) for udp
	//param.i_keyint_max = 60;
	
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
	xcb_get_image_reply_t *img;
	int interval = 60000;
	if (windownode->window->override) {
		interval = 200000;
	}
	for (;; i_frame++) {
	    usleep(interval);
	    if(!window->viewable){ //should use wait
	        continue;
	    }
	    if(list_empty(&client_list.list_node)){ //should use wait
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
