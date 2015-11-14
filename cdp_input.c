#include "cdp_protocol.h"

void cdp_input_mousemove(cdp_request_mousemove_t *req)
{
    cdp_x11_input_mousemove(req->wid, req->x, req->y);
}

void cdp_input_mousedown(cdp_request_mousedown_t *req)
{
    cdp_x11_input_mousedown(req->wid, req->code);
}

void cdp_input_mouseup(cdp_request_mouseup_t *req)
{
    cdp_x11_input_mouseup(req->wid, req->code);
}

void cdp_input_keydown(cdp_request_keydown_t *req)
{
    cdp_x11_input_keydown(req->wid, req->code);
}

void cdp_input_keyup(cdp_request_keyup_t *req)
{
    cdp_x11_input_keyup(req->wid, req->code);
}

void cdp_input_keypress(cdp_request_keypress_t *req)
{
    cdp_x11_input_keypress(req->wid, req->code);
}