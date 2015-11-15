#ifndef CDP_PROTOCOL_H
#define CDP_PROTOCOL_H

#include <stdint.h>
#include <cdp_defs.h>

enum CDP_MESSAGE {
    CDP_MESSAGE_CREATE_WINDOW = 0,
    CDP_MESSAGE_DESTROY_WINDOW,
    CDP_MESSAGE_SHOW_WINDOW,
    CDP_MESSAGE_HIDE_WINDOW,
    CDP_MESSAGE_WINDOW_FRAME = 4,
    CDP_MESSAGE_CONFIGURE_WINDOW,
    CDP_MESSAGE_LISTEN_REPLY
};

enum CDP_REQUEST {
    CDP_REQUEST_GET_WINDOWS = 1,
    CDP_REQUEST_LISTEN = 2,
    CDP_REQUEST_UNLISTEN = 3,
    CDP_REQUEST_MOUSEMOVE = 4,
    CDP_REQUEST_MOUSEDOWN = 5,
    CDP_REQUEST_MOUSEUP = 6,
    CDP_REQUEST_KEYDOWN = 7,
    CDP_REQUEST_KEYUP = 8,
    CDP_REQUEST_WINDOW_CONFIGURE = 9,
    CDP_REQUEST_WINDOW_MOVE = 10,
    CDP_REQUEST_WINDOW_RESIZE = 11,
    CDP_REQUEST_KEYPRESS = 12,
};

/**
 * requests
 **/
 
typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
} cdp_request_listen_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 resource;
} cdp_request_unlisten_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    i16 x;
    i16 y;
} cdp_request_mousemove_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u8 code;
    u8 _pad1[3];
} cdp_request_mousedown_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u8 code;
    u8 _pad1[3];
} cdp_request_mouseup_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u8 code;
    u8 _pad1[3];
} cdp_request_keydown_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u8 code;
    u8 _pad1[3];
} cdp_request_keyup_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u8 code;
    u8 _pad1[3];
} cdp_request_keypress_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    i16 x;
    i16 y;
} cdp_request_window_move_t;

typedef struct {
    u8 reqtype;
    u8 _pad;
    u16 length;
    u32 wid;
    u16 width;
    u16 height;
} cdp_request_window_resize_t;

/**
 * messages
 **/
typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 resource;
} cdp_message_listen_reply_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
    i16 x;
    i16 y;
    u16 width;
    u16 height;
    u8 override;
    u8 viewable;
    u16 _pad1;
} cdp_message_create_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_destroy_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
    u8 override;
    u8 _pad1[3];
} cdp_message_show_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_hide_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
    u32 frame_size;
    //data
} cdp_message_window_frame_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
    i16 x;
    i16 y;
    u16 width;
    u16 height;
    u32 above;
    u8 override;
    u8 _pad1[3];
} cdp_message_configure_window_t;

#endif