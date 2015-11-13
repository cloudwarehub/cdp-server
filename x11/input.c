#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xtest.h>
#include <xcb/xcb_keysyms.h>
#include "cdp_defs.h"

extern xcb_connection_t *xconn;

void cdp_x11_input_mousemove(u32 wid, i16 x, i16 y)
{
    xcb_warp_pointer(xconn, XCB_NONE, wid, 0, 0, 0, 0, x, y);
}

void cdp_x11_input_mousedown(u32 wid, u8 code)
{
    xcb_test_fake_input(xconn, XCB_BUTTON_PRESS, code, XCB_CURRENT_TIME, wid, 0, 0, 0);
}

void cdp_x11_input_mouseup(u32 wid, u8 code)
{
    xcb_test_fake_input(xconn, XCB_BUTTON_RELEASE, code, XCB_CURRENT_TIME, wid, 0, 0, 0);
}

void cdp_x11_input_keydown(u32 wid, u8 code)
{
    // xcb_key_symbols_t *syms = NULL;
    // xcb_keycode_t kc;
    // xcb_keysym_t ks = code;
    // kc = xcb_key_symbols_get_keycode(syms, ks);
    xcb_test_fake_input(xconn, XCB_KEY_PRESS, code, XCB_CURRENT_TIME, wid, 0, 0, 0 );
}

void cdp_x11_input_keyup(u32 wid, u8 code)
{
    // xcb_key_symbols_t *syms = NULL;
    // xcb_keycode_t kc;
    // xcb_keysym_t ks = code;
    // kc = xcb_key_symbols_get_keycode(syms, ks);
    xcb_test_fake_input(xconn, XCB_KEY_RELEASE, code, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0 );
}