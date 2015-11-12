#include <xcb/xcb.h>
#include "cdp_defs.h"

extern xcb_connection_t *xconn;

void cdp_x11_request_window_move(u32 wid, i16 x, i16 y)
{
    u32 values[] = {x, y};
    xcb_configure_window(xconn, wid, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    xcb_flush(xconn); 
}