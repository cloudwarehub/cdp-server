#include <stdint.h>
#include <netinet/in.h>
#include <x264.h>
#include "list.h"

enum CDP_REQUEST {
    CDP_REQUEST_GET_WINDOWS,
    CDP_REQUEST_LISTEN,
};

typedef struct {
    uint32_t id;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    char override;
    char viewable;
} cdp_window_t;

struct client_node{
    struct list_head list_node;
    struct sockaddr_in sockaddr;
};

struct window_node{
    struct list_head list_node;
    cdp_window_t window;
    pthread_t sthread; // stream_thread id
    x264_param_t param;
};