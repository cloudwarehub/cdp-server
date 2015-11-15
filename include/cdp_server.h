#ifndef CDP_SERVER_H
#define CDP_SERVER_H

#include <stdint.h>
#include <netinet/in.h>
#include <x264.h>
#include "list.h"
#include "cdp_defs.h"

typedef struct {
    uint32_t id;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    char override;
    char viewable;
    uint32_t above;
} cdp_window_t;

struct client_node{
    struct list_head list_node;
    struct sockaddr_in sockaddr;
    int resource;
};

struct window_node{
    struct list_head list_node;
    cdp_window_t *window;
    pthread_t sthread; // stream_thread id
    x264_param_t param;
    int force_keyframe;
    u16 nwidth;
    u16 nheight;
    int refresh;
};

#endif